#!/usr/bin/env python3
"""
generate_directory_index.py

DevTools-friendly directory index generator for SOTS.

Outputs an LLM-friendly "tree" text file (similar to Windows `tree /F /A`) with:
- max depth / max items safety caps
- exclude by directory name and/or relative path prefixes
- optional folders-only mode
- optional chunking into part files
- JSON summary for quick ingestion

This script prints clear progress + writes a small run log in the output folder.
"""

from __future__ import annotations

import argparse
import os
import sys
import json
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable

from cli_utils import confirm_start, confirm_exit
from project_paths import get_project_root
from llm_log import print_llm_summary


DEFAULT_EXCLUDE = [
    "Binaries",
    "Intermediate",
    "Saved",
    "DerivedDataCache",
    ".vs",
    ".git",
    "ThirdParty",
]


def _now_stamp() -> str:
    return datetime.now().strftime("%Y-%m-%d_%H-%M-%S")


def _split_csv(s: str | None) -> list[str]:
    if not s:
        return []
    parts: list[str] = []
    for p in s.split(","):
        p = p.strip()
        if p:
            parts.append(p)
    return parts


def _norm_rel_posix(root: Path, path: Path) -> str:
    try:
        rel = path.resolve().relative_to(root.resolve())
    except Exception:
        rel = path
    return str(rel).replace("\\", "/").lstrip("/")


@dataclass
class ExcludeRules:
    dirnames_lower: set[str]
    path_prefixes_lower: list[str]

    @staticmethod
    def from_tokens(tokens: Iterable[str]) -> "ExcludeRules":
        dirnames: set[str] = set()
        prefixes: list[str] = []
        for t in tokens:
            t = t.strip()
            if not t:
                continue
            if "/" in t or "\\" in t:
                prefixes.append(t.replace("\\", "/").strip("/").lower())
            else:
                dirnames.add(t.lower())
        return ExcludeRules(dirnames_lower=dirnames, path_prefixes_lower=prefixes)

    def is_excluded_dir(self, name: str, rel_posix: str) -> bool:
        n = name.lower()
        if n in self.dirnames_lower:
            return True
        rel_l = rel_posix.lower()
        for pref in self.path_prefixes_lower:
            if rel_l.startswith(pref):
                return True
        return False


@dataclass
class Stats:
    total_dirs_seen: int = 0
    total_files_seen: int = 0
    total_excluded_dirs: int = 0
    total_lines_written: int = 0
    max_depth_seen: int = 0
    hit_max_items: bool = False
    errors: int = 0


class LineWriter:
    def __init__(self, out_dir: Path, base_name: str, chunk_lines: int):
        self.out_dir = out_dir
        self.base_name = base_name
        self.chunk_lines = max(0, int(chunk_lines))
        self.part_index = 1
        self.line_in_part = 0
        self.parts: list[Path] = []
        self.fp = None

    def _part_path(self, idx: int) -> Path:
        stem = Path(self.base_name).stem
        suffix = Path(self.base_name).suffix or ".txt"
        if self.chunk_lines > 0:
            return self.out_dir / f"{stem}.part{idx:02d}{suffix}"
        return self.out_dir / f"{stem}{suffix}"

    def open(self) -> None:
        self.out_dir.mkdir(parents=True, exist_ok=True)
        p = self._part_path(self.part_index)
        self.fp = p.open("w", encoding="utf-8", errors="replace", newline="\n")
        self.parts.append(p)

    def close(self) -> None:
        if self.fp:
            self.fp.close()
            self.fp = None

    def write_line(self, line: str, stats: Stats) -> None:
        if self.fp is None:
            self.open()

        # Chunk rollover
        if self.chunk_lines > 0 and self.line_in_part >= self.chunk_lines:
            self.close()
            self.part_index += 1
            self.line_in_part = 0
            self.open()

        assert self.fp is not None
        self.fp.write(line + "\n")
        self.line_in_part += 1
        stats.total_lines_written += 1

    def write_manifest_if_chunked(self, manifest_name: str, header_lines: list[str]) -> Path | None:
        if self.chunk_lines <= 0:
            return None

        manifest = self.out_dir / manifest_name
        with manifest.open("w", encoding="utf-8", errors="replace", newline="\n") as f:
            for h in header_lines:
                f.write(h + "\n")
            f.write("\nParts:\n")
            for p in self.parts:
                f.write(f" - {p.name}\n")
        return manifest


def _scandir_sorted(path: Path):
    # Fast-ish sorted scandir for stable output.
    try:
        with os.scandir(path) as it:
            entries = list(it)
    except Exception:
        return [], []

    dirs = [e for e in entries if e.is_dir(follow_symlinks=False)]
    files = [e for e in entries if e.is_file(follow_symlinks=False)]
    dirs.sort(key=lambda e: e.name.lower())
    files.sort(key=lambda e: e.name.lower())
    return dirs, files


def generate_index(
    root: Path,
    writer: LineWriter,
    rules: ExcludeRules,
    *,
    out_name_for_manifest: str,
    folders_only: bool,
    max_depth: int,
    max_items: int,
    show_excluded_placeholders: bool,
) -> tuple[Stats, dict[str, dict[str, int]]]:
    stats = Stats()
    top_level: dict[str, dict[str, int]] = {}

    def bump_top(top: str, key: str, amount: int = 1) -> None:
        d = top_level.setdefault(top, {"dirs": 0, "files": 0, "excluded_dirs": 0})
        d[key] = int(d.get(key, 0)) + amount

    def walk(dir_path: Path, prefix: str, depth: int, top_name: str) -> None:
        if stats.total_dirs_seen + stats.total_files_seen >= max_items:
            stats.hit_max_items = True
            return
        stats.max_depth_seen = max(stats.max_depth_seen, depth)
        if depth > max_depth:
            return

        dirs, files = _scandir_sorted(dir_path)

        children = []
        children.extend(("dir", d) for d in dirs)
        if not folders_only:
            children.extend(("file", f) for f in files)

        for i, (kind, entry) in enumerate(children):
            if stats.total_dirs_seen + stats.total_files_seen >= max_items:
                stats.hit_max_items = True
                return

            is_last = (i == len(children) - 1)
            connector = "\\---" if is_last else "+---"

            name = entry.name
            child_path = Path(entry.path)

            if kind == "dir":
                rel = _norm_rel_posix(root, child_path)
                excluded = rules.is_excluded_dir(name, rel)

                if excluded:
                    stats.total_excluded_dirs += 1
                    bump_top(top_name, "excluded_dirs", 1)
                    if show_excluded_placeholders:
                        writer.write_line(f"{prefix}{connector}{name} [EXCLUDED]", stats)
                    continue

                writer.write_line(f"{prefix}{connector}{name}", stats)
                stats.total_dirs_seen += 1
                bump_top(top_name, "dirs", 1)

                next_prefix = (prefix + "    ") if is_last else (prefix + "|   ")
                walk(child_path, next_prefix, depth + 1, top_name)
            else:
                writer.write_line(f"{prefix}{connector}{name}", stats)
                stats.total_files_seen += 1
                bump_top(top_name, "files", 1)

    # Header at top of output
    header = [
        f'Directory index root: "{root}"',
        f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
        f"MaxDepth: {max_depth} | MaxItems: {max_items} | FoldersOnly: {folders_only}",
        f"ExcludeDirNames: {', '.join(sorted(rules.dirnames_lower)) if rules.dirnames_lower else '(none)'}",
        f"ExcludePathPrefixes: {', '.join(rules.path_prefixes_lower) if rules.path_prefixes_lower else '(none)'}",
        "",
    ]
    for h in header:
        writer.write_line(h, stats)

    # Root line
    writer.write_line(".", stats)

    # Root's immediate folders become the top-level buckets.
    top_bucket_for_root_files = "."
    top_level.setdefault(top_bucket_for_root_files, {"dirs": 0, "files": 0, "excluded_dirs": 0})

    try:
        dirs, files = _scandir_sorted(root)
    except Exception:
        dirs, files = [], []

    children = []
    children.extend(("dir", d) for d in dirs)
    if not folders_only:
        children.extend(("file", f) for f in files)

    for i, (kind, entry) in enumerate(children):
        if stats.total_dirs_seen + stats.total_files_seen >= max_items:
            stats.hit_max_items = True
            break

        is_last = (i == len(children) - 1)
        connector = "\\---" if is_last else "+---"
        name = entry.name
        child_path = Path(entry.path)

        if kind == "dir":
            rel = _norm_rel_posix(root, child_path)
            excluded = rules.is_excluded_dir(name, rel)
            if excluded:
                stats.total_excluded_dirs += 1
                bump_top(name, "excluded_dirs", 1)
                if show_excluded_placeholders:
                    writer.write_line(f"{connector}{name} [EXCLUDED]", stats)
                continue

            writer.write_line(f"{connector}{name}", stats)
            stats.total_dirs_seen += 1
            bump_top(name, "dirs", 1)

            next_prefix = ("    ") if is_last else ("|   ")
            walk(child_path, next_prefix, 1, name)
        else:
            writer.write_line(f"{connector}{name}", stats)
            stats.total_files_seen += 1
            bump_top(top_bucket_for_root_files, "files", 1)

    # Footer
    writer.write_line("", stats)
    writer.write_line(f"Total dirs written: {stats.total_dirs_seen}", stats)
    writer.write_line(f"Total files written: {stats.total_files_seen}", stats)
    writer.write_line(f"Excluded dirs: {stats.total_excluded_dirs}", stats)
    writer.write_line(f"Max depth seen: {stats.max_depth_seen}", stats)
    if stats.hit_max_items:
        writer.write_line(f"NOTE: Hit max-items cap ({max_items}). Output is truncated.", stats)

    # If chunked, create manifest file listing parts.
    manifest_header = header + [
        f"Output was chunked into {len(writer.parts)} part file(s).",
        f"Base name: {out_name_for_manifest}",
    ]
    writer.write_manifest_if_chunked(out_name_for_manifest, manifest_header)

    return stats, top_level


def main(argv: list[str] | None = None) -> int:
    tool_name = "generate_directory_index"
    confirm_start(tool_name)

    ap = argparse.ArgumentParser(description="Generate an LLM-friendly directory index text file.")
    ap.add_argument("--root", help="Root directory to index (defaults to project root).")
    ap.add_argument("--out-dir", help="Output directory (defaults to DevTools/reports/directory_index/<timestamp>).")
    ap.add_argument("--out-name", default="LLM_DIRECTORY_INDEX.txt", help="Base output file name.")
    ap.add_argument("--summary-name", default="LLM_DIRECTORY_INDEX.summary.json", help="JSON summary file name.")
    ap.add_argument("--max-depth", type=int, default=10, help="Maximum recursion depth.")
    ap.add_argument("--max-items", type=int, default=200000, help="Max total dirs+files to write before truncating.")
    ap.add_argument("--folders-only", action="store_true", help="Only list folders.")
    ap.add_argument("--exclude", help="Comma-separated exclude tokens (dir names or relative path prefixes).")
    ap.add_argument("--chunk-lines", type=int, default=0, help="Split output into part files of N lines (0 disables).")
    ap.add_argument("--no-excluded-placeholders", action="store_true", help="Do not write '[EXCLUDED]' placeholder lines.")

    args = ap.parse_args(argv)

    root = Path(args.root).expanduser().resolve() if args.root else get_project_root()

    if args.out_dir:
        out_dir = Path(args.out_dir).expanduser().resolve()
    else:
        out_dir = get_project_root() / "DevTools" / "reports" / "directory_index" / _now_stamp()

    out_dir.mkdir(parents=True, exist_ok=True)

    exclude_tokens = DEFAULT_EXCLUDE.copy()
    exclude_tokens.extend(_split_csv(args.exclude))
    rules = ExcludeRules.from_tokens(exclude_tokens)

    # Write a tiny run log (per your “never silent” law).
    run_log = out_dir / "directory_index_run.log"
    with run_log.open("w", encoding="utf-8", errors="replace", newline="\n") as f:
        f.write(f"Tool: {tool_name}\n")
        f.write(f"Root: {root}\n")
        f.write(f"OutDir: {out_dir}\n")
        f.write(f"OutName: {args.out_name}\n")
        f.write(f"SummaryName: {args.summary_name}\n")
        f.write(f"MaxDepth: {args.max_depth}\n")
        f.write(f"MaxItems: {args.max_items}\n")
        f.write(f"FoldersOnly: {args.folders_only}\n")
        f.write(f"ChunkLines: {args.chunk_lines}\n")
        f.write(f"ExcludeDirNames: {', '.join(sorted(rules.dirnames_lower))}\n")
        f.write(f"ExcludePathPrefixes: {', '.join(rules.path_prefixes_lower)}\n")

    writer = LineWriter(out_dir, args.out_name, args.chunk_lines)

    print(f"[INFO] Root: {root}")
    print(f"[INFO] Output dir: {out_dir}")
    print(f"[INFO] Writing: {args.out_name} (chunk_lines={args.chunk_lines})")
    print(f"[INFO] Writing summary: {args.summary_name}")
    print(f"[INFO] Excludes: {', '.join(exclude_tokens)}")

    stats, top_level = generate_index(
        root,
        writer,
        rules,
        out_name_for_manifest=args.out_name,
        folders_only=args.folders_only,
        max_depth=args.max_depth,
        max_items=args.max_items,
        show_excluded_placeholders=(not args.no_excluded_placeholders),
    )

    writer.close()

    summary = {
        "root": str(root),
        "generated_local": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "options": {
            "max_depth": args.max_depth,
            "max_items": args.max_items,
            "folders_only": bool(args.folders_only),
            "chunk_lines": int(args.chunk_lines),
            "excluded_placeholders": (not args.no_excluded_placeholders),
        },
        "totals": {
            "dirs_written": stats.total_dirs_seen,
            "files_written": stats.total_files_seen,
            "excluded_dirs": stats.total_excluded_dirs,
            "lines_written": stats.total_lines_written,
            "max_depth_seen": stats.max_depth_seen,
            "hit_max_items": stats.hit_max_items,
            "errors": stats.errors,
        },
        "top_level": top_level,
        "exclude_dirnames": sorted(rules.dirnames_lower),
        "exclude_path_prefixes": rules.path_prefixes_lower,
        "outputs": {
            "out_dir": str(out_dir),
            "parts": [p.name for p in writer.parts],
            "manifest_or_output": args.out_name,
            "run_log": run_log.name,
        },
    }

    summary_path = out_dir / args.summary_name
    summary_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")

    print("[INFO] Done.")
    print(f"[INFO] Parts written: {len(writer.parts)}")
    for p in writer.parts:
        print(f"  - {p}")
    print(f"[INFO] Run log: {run_log}")
    print(f"[INFO] Summary JSON: {summary_path}")

    print_llm_summary(
        tool=tool_name,
        status="OK",
        root=str(root),
        out_dir=str(out_dir),
        out_name=str(args.out_name),
        summary_name=str(args.summary_name),
        max_depth=args.max_depth,
        max_items=args.max_items,
        folders_only=bool(args.folders_only),
        chunk_lines=int(args.chunk_lines),
        dirs_written=stats.total_dirs_seen,
        files_written=stats.total_files_seen,
        excluded_dirs=stats.total_excluded_dirs,
        lines_written=stats.total_lines_written,
        max_depth_seen=stats.max_depth_seen,
        hit_max_items=stats.hit_max_items,
        parts_written=len(writer.parts),
    )

    confirm_exit()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
