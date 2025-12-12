#!/usr/bin/env python3
"""
ad_hoc_regex_search.py

Ad-hoc regex search tool for the SOTS DevTools suite.
"""
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
        description="Ad-hoc regex search across the SOTS project tree."
    )
    parser.add_argument("--pattern", type=str, default=None)
    parser.add_argument("--literal", type=str, default=None)
    parser.add_argument("--root", type=str, default=None)
    parser.add_argument(
        "--exts",
        type=str,
        default=".h,.cpp,.ini,.uplugin,.uproject,.cs,.py,.json,.txt",
    )
    parser.add_argument("--ignore-case", action="store_true")
    parser.add_argument("--context", type=int, default=0)

    args = parser.parse_args()

    if not args.pattern:
        print("[PROMPT] No --pattern supplied; entering interactive mode.")
        pattern_input = input("Regex pattern (required): ").strip()
        if not pattern_input:
            print("[ERROR] Empty pattern; aborting.")
            return
        args.pattern = pattern_input
        if args.literal is None:
            literal_input = input("Optional literal prefilter (press Enter to skip): ").strip()
            args.literal = literal_input or None

    if args.root:
        root = Path(args.root).expanduser().resolve()
    else:
        root = get_project_root()

    extset = {
        e.strip() for e in args.exts.split(",")
        if e.strip() and e.strip().startswith(".")
    }

    flags = re.MULTILINE
    if args.ignore_case:
        flags |= re.IGNORECASE

    try:
        regex = re.compile(args.pattern, flags)
    except re.error as ex:
        print(f"[ERROR] Invalid regex pattern: {ex}")
        print_llm_summary(
            "ad_hoc_regex_search",
            status="invalid_regex",
            ROOT=str(root),
            PATTERN=args.pattern,
            LITERAL=args.literal,
        )
        return

    print(f"[INFO] Root directory : {root}")
    print(f"[INFO] Extensions     : {sorted(extset) if extset else ['<all>']}")
    print(f"[INFO] Regex pattern  : {args.pattern!r}")
    print(f"[INFO] Literal filter : {args.literal!r}")
    print(f"[INFO] Ignore case    : {args.ignore_case}")
    print(f"[INFO] Context lines  : {args.context}")
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

        if args.literal and args.literal not in text:
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
            start_line = max(0, line_index - args.context)
            end_line = min(len(lines) - 1, line_index + args.context)

            for i in range(start_line, end_line + 1):
                prefix = ">" if i == line_index else " "
                print(f"{prefix}{i + 1:5d}: {lines[i]}")
            print()

    print("\n=== ad_hoc_regex_search summary ===")
    print(f"  Files scanned    : {total_files}")
    print(f"  Files with match : {matched_files}")
    print(f"  Total matches    : {total_matches}")

    status = "no_matches" if total_matches == 0 else "ok"

    print_llm_summary(
        "ad_hoc_regex_search",
        status=status,
        ROOT=str(root),
        PATTERN=args.pattern,
        LITERAL=args.literal,
        EXTS=sorted(extset),
        IGNORE_CASE=args.ignore_case,
        CONTEXT=args.context,
        FILES_SCANNED=total_files,
        FILES_MATCHED=matched_files,
        MATCH_COUNT=total_matches,
    )

    # no interactive exit prompt for automation


if __name__ == "__main__":
    main()
