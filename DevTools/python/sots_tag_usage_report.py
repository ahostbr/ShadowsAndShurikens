# sots_tag_usage_report.py
# Aggregate GameplayTag usage across plugins/files.
# Uses simple text search for 'SAS.' and 'GameplayTag' patterns.
#
# Noisy by design: prints progress and writes a small log + JSON.

import os
import re
import json
import argparse
from pathlib import Path
from datetime import datetime


def scan_file(path: Path, tag_root: str, tag_regex: re.Pattern):
    matches = []
    try:
        with path.open("r", encoding="utf-8", errors="ignore") as f:
            for lineno, line in enumerate(f, 1):
                if tag_root in line or tag_regex.search(line):
                    matches.append({
                        "line": lineno,
                        "text": line.strip(),
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


def build_report(root: Path, tag_root: str, exts, ignore_dirs):
    tag_regex = re.compile(re.escape(tag_root))
    report = {}
    total_files = 0
    total_matches = 0

    for fpath in walk_root(root, exts, ignore_dirs):
        total_files += 1
        rel = fpath.relative_to(root)
        text = str(rel)

        # crude plugin guess: parent of 'Source' or immediate parent under Plugins
        parts = rel.parts
        plugin_name = None
        if "Plugins" in parts:
            idx = parts.index("Plugins")
            if len(parts) > idx + 1:
                plugin_name = parts[idx + 1]
        if not plugin_name and len(parts) > 0:
            plugin_name = parts[0]

        matches = scan_file(fpath, tag_root, tag_regex)
        if not matches:
            continue

        total_matches += len(matches)

        r_plugin = report.setdefault(plugin_name, {})
        r_file = r_plugin.setdefault(str(rel), [])
        r_file.extend(matches)

    return report, total_files, total_matches


def print_summary(report, total_files, total_matches):
    print("[RESULT] Tag Usage Report Summary")
    print("================================")
    print(f"Files scanned: {total_files}")
    print(f"Total matches: {total_matches}")
    print("")
    for plugin, files in sorted(report.items()):
        count = sum(len(m) for m in files.values())
        print(f"- {plugin}: {count} matches in {len(files)} files")


def main():
    parser = argparse.ArgumentParser(description="SOTS GameplayTag usage report")
    parser.add_argument(
        "--root",
        type=str,
        default=".",
        help="Root folder to scan (project root).",
    )
    parser.add_argument(
        "--tag-root",
        type=str,
        default="SAS.",
        help="Tag root string to search for (e.g. 'SAS.').",
    )
    parser.add_argument(
        "--exts",
        type=str,
        default=".h,.cpp,.ini,.json,.txt",
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
        default="DevTools/reports/sots_tag_usage_report.json",
        help="Path to write JSON output.",
    )
    args = parser.parse_args()

    root = Path(args.root).resolve()
    exts = set(e.strip() for e in args.exts.split(",") if e.strip())
    ignore_dirs = set(d.strip() for d in args.ignore_dir.split(",") if d.strip())
    out_json = Path(args.output_json).resolve()
    out_log = out_json.with_suffix(".log")

    print(f"[INFO] sots_tag_usage_report.py starting at {datetime.now().isoformat()}")
    print(f"[INFO] Root: {root}")
    print(f"[INFO] Tag root: {args.tag_root}")

    report, total_files, total_matches = build_report(root, args.tag_root, exts, ignore_dirs)

    out_json.parent.mkdir(parents=True, exist_ok=True)
    with out_json.open("w", encoding="utf-8") as f:
        json.dump({
            "root": str(root),
            "tag_root": args.tag_root,
            "total_files": total_files,
            "total_matches": total_matches,
            "report": report,
        }, f, indent=2)

    print_summary(report, total_files, total_matches)

    with out_log.open("w", encoding="utf-8") as f:
        f.write(f"sots_tag_usage_report run at {datetime.now().isoformat()}\n")
        f.write(f"Root: {root}\n")
        f.write(f"Tag root: {args.tag_root}\n")
        f.write(f"Files scanned: {total_files}\n")
        f.write(f"Total matches: {total_matches}\n")

    print(f"[INFO] JSON written to: {out_json}")
    print(f"[INFO] Log written to: {out_log}")
    print("[INFO] Done.")


if __name__ == "__main__":
    main()
