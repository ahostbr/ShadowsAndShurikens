#!/usr/bin/env python3
"""quick_search.py - Simple literal + regex search for SOTS DevTools."""
from __future__ import annotations

import argparse
import re
from pathlib import Path
from typing import Iterator

from project_paths import get_project_root
from llm_log import print_llm_summary


def iter_files(root: Path, exts: set[str]) -> Iterator[Path]:
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if exts and path.suffix not in exts:
            continue
        yield path


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Quick literal + regex search across the SOTS project tree."
    )
    parser.add_argument(
        "--search",
        required=True,
        help="Literal substring to prefilter files (e.g. '#define GroundLeft').",
    )
    parser.add_argument(
        "--regex",
        default=None,
        help="Optional regex pattern. If omitted, uses a regex-escaped version of --search.",
    )
    parser.add_argument(
        "--root",
        default=None,
        help="Root directory to scan (default: SOTS project root).",
    )
    parser.add_argument(
        "--exts",
        default=".h,.cpp,.ini,.uplugin,.uproject,.cs,.py,.json,.txt",
        help="Comma-separated list of file extensions to include, e.g. '.h,.cpp'.",
    )
    parser.add_argument(
        "--ignore-case",
        action="store_true",
        help="Perform a case-insensitive search.",
    )

    args = parser.parse_args()

    if args.root:
        root = Path(args.root).expanduser().resolve()
    else:
        root = get_project_root()

    extset = {
        e.strip() for e in args.exts.split(",")
        if e.strip() and e.strip().startswith(".")
    }

    pattern = args.regex if args.regex else re.escape(args.search)
    flags = re.MULTILINE
    if args.ignore_case:
        flags |= re.IGNORECASE

    try:
        regex = re.compile(pattern, flags)
    except re.error as ex:
        print(f"[ERROR] Invalid regex pattern: {ex}")
        print_llm_summary(
            "quick_search",
            status="invalid_regex",
            ROOT=str(root),
            SEARCH=args.search,
            REGEX=pattern,
            EXTS=sorted(extset),
            IGNORE_CASE=args.ignore_case,
        )
        return

    print(f"[INFO] Root directory : {root}")
    print(f"[INFO] Extensions     : {sorted(extset) if extset else ['<all>']}")
    print(f"[INFO] Search literal : {args.search!r}")
    print(f"[INFO] Regex pattern  : {pattern!r}")
    print(f"[INFO] Ignore case    : {args.ignore_case}")
    print()

    total_files = 0
    matched_files = 0
    total_matches = 0

    for file_path in iter_files(root, extset):
        total_files += 1

        try:
            text = file_path.read_text(encoding="utf-8", errors="ignore")
        except Exception as ex:
            print(f"[WARN] Could not read {file_path}: {ex}")
            continue

        haystack = text.lower() if args.ignore_case else text
        needle = args.search.lower() if args.ignore_case else args.search

        if needle not in haystack:
            continue

        matches = list(regex.finditer(text))
        if not matches:
            continue

        matched_files += 1
        print(f"\n--- {file_path} ---")

        lines = text.splitlines()
        for m in matches:
            total_matches += 1
            line_index = text.count("\n", 0, m.start())
            line_no = line_index + 1
            line_text = lines[line_index] if 0 <= line_index < len(lines) else ""
            print(f"{line_no:5d}: {line_text}")

    print("\n=== quick_search summary ===")
    print(f"  Files scanned    : {total_files}")
    print(f"  Files with match : {matched_files}")
    print(f"  Total matches    : {total_matches}")

    status = "no_matches" if total_matches == 0 else "ok"

    print_llm_summary(
        "quick_search",
        status=status,
        ROOT=str(root),
        SEARCH=args.search,
        REGEX=pattern,
        EXTS=sorted(extset),
        IGNORE_CASE=args.ignore_case,
        FILES_SCANNED=total_files,
        FILES_MATCHED=matched_files,
        MATCH_COUNT=total_matches,
    )

    # no interactive exit prompt for automation


if __name__ == "__main__":
    main()
