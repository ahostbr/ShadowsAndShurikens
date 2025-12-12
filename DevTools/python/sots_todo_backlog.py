# sots_todo_backlog.py
# Collect TODO/FIXME/@Ryan/@Buddy style comments across the tree.
# Produces a backlog per plugin/file.

import os
import json
import argparse
from pathlib import Path
from datetime import datetime

TODO_PATTERNS = [
    "TODO",
    "FIXME",
    "@Ryan",
    "@Buddy",
]


def scan_file(path: Path):
    matches = []
    try:
        with path.open("r", encoding="utf-8", errors="ignore") as f:
            for lineno, line in enumerate(f, 1):
                if any(tok in line for tok in TODO_PATTERNS):
                    matches.append({
                        "line": lineno,
                        "text": line.rstrip(),
                    })
    except Exception as e:
        print(f"[WARN] Failed to read {path}: {e}")
    return matches


def walk_root(root: Path, exts, ignore_dirs):
    for dirpath, dirnames, filenames in os.walk(root):
        rel_parts = Path(dirpath).parts
        if any(ig in rel_parts for ig in ignore_dirs):
            continue
        for fname in filenames:
            p = Path(dirpath) / fname
            if p.suffix in exts:
                yield p


def build_backlog(root: Path, exts, ignore_dirs):
    backlog = {}
    total_matches = 0
    total_files = 0
    for fpath in walk_root(root, exts, ignore_dirs):
        total_files += 1
        rel = fpath.relative_to(root)
        parts = rel.parts
        plugin_name = None
        if "Plugins" in parts:
            idx = parts.index("Plugins")
            if len(parts) > idx + 1:
                plugin_name = parts[idx + 1]
        if not plugin_name and len(parts) > 0:
            plugin_name = parts[0]

        matches = scan_file(fpath)
        if not matches:
            continue
        total_matches += len(matches)

        p_data = backlog.setdefault(plugin_name, {})
        p_data[str(rel)] = matches

    return backlog, total_files, total_matches


def print_summary(backlog, total_files, total_matches):
    print("[RESULT] TODO / FIXME backlog")
    print("================================")
    print(f"Files scanned: {total_files}")
    print(f"Total matches: {total_matches}")
    print("")
    for plugin, files in sorted(backlog.items()):
        count = sum(len(m) for m in files.values())
        print(f"- {plugin}: {count} items in {len(files)} files")


def main():
    parser = argparse.ArgumentParser(description="SOTS TODO/FIXME backlog collector")
    parser.add_argument(
        "--root",
        type=str,
        default=".",
        help="Root folder to scan (project root).",
    )
    parser.add_argument(
        "--exts",
        type=str,
        default=".h,.cpp,.cs,.ini,.json,.txt",
        help="Comma-separated list of file extensions to scan.",
    )
    parser.add_argument(
        "--ignore-dir",
        type=str,
        default=".git,Intermediate,Binaries,Saved,.vs",
        help="Comma-separated list of directory names to ignore.",
    )
    parser.add_argument(
        "--output-json",
        type=str,
        default="DevTools/reports/sots_todo_backlog.json",
        help="Where to write JSON output.",
    )
    args = parser.parse_args()

    root = Path(args.root).resolve()
    exts = set(e.strip() for e in args.exts.split(",") if e.strip())
    ignore_dirs = set(d.strip() for d in args.ignore_dir.split(",") if d.strip())
    out_json = Path(args.output_json).resolve()
    out_log = out_json.with_suffix(".log")

    print(f"[INFO] sots_todo_backlog.py starting at {datetime.now().isoformat()}")
    print(f"[INFO] Root: {root}")

    backlog, total_files, total_matches = build_backlog(root, exts, ignore_dirs)

    out_json.parent.mkdir(parents=True, exist_ok=True)
    with out_json.open("w", encoding="utf-8") as f:
        json.dump({
            "root": str(root),
            "total_files": total_files,
            "total_matches": total_matches,
            "backlog": backlog,
        }, f, indent=2)

    with out_log.open("w", encoding="utf-8") as f:
        f.write(f"sots_todo_backlog run at {datetime.now().isoformat()}\n")
        f.write(f"Root: {root}\n")
        f.write(f"Files scanned: {total_files}\n")
        f.write(f"Total matches: {total_matches}\n")

    print_summary(backlog, total_files, total_matches)

    print(f"[INFO] JSON written to: {out_json}")
    print(f"[INFO] Log written to: {out_log}")
    print("[INFO] Done.")


if __name__ == "__main__":
    main()
