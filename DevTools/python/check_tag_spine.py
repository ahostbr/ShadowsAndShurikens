#!/usr/bin/env python3
"""Heuristic scanner for ad-hoc AddTag/RemoveTag usage outside SOTS_TagManager.

This is a heuristic and will generate false positives (e.g., AddTag on arrays of FNames),
so manual review is recommended when it reports results.
"""

import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
PLUGINS = os.path.join(ROOT, "Plugins")


def iter_source_files(root_dir: str):
    pattern = re.compile(r"\.(h|hpp|cpp|inl)$", re.IGNORECASE)
    for dirpath, _dirnames, filenames in os.walk(root_dir):
        # Skip the TagManager plugin itself
        if os.path.basename(dirpath) == "SOTS_TagManager":
            continue
        for fname in filenames:
            if not pattern.search(fname):
                continue
            yield os.path.join(dirpath, fname)


def main() -> int:
    if not os.path.isdir(PLUGINS):
        print("Plugins directory not found: ", PLUGINS)
        return 1

    add_pattern = re.compile(r"\.AddTag\s*\(")
    remove_pattern = re.compile(r"\.RemoveTag\s*\(")
    matches = []

    for fpath in iter_source_files(PLUGINS):
        try:
            with open(fpath, "r", encoding="utf-8", errors="ignore") as f:
                for i, line in enumerate(f, start=1):
                    if add_pattern.search(line) or remove_pattern.search(line):
                        matches.append((fpath, i, line.strip()))
        except Exception as e:
            print("Failed to read", fpath, e)

    if not matches:
        print("No suspicious AddTag/RemoveTag usage found outside of SOTS_TagManager.")
        return 0

    print("\nSuspicious AddTag/RemoveTag usage (heuristic):\n")
    for fpath, line_no, line in matches:
        print(f"{os.path.relpath(fpath, ROOT)}:{line_no} -> {line}")

    # Optional: return non-zero to fail CI if configured
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
