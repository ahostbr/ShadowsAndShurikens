from __future__ import annotations

import argparse
import re
from pathlib import Path
from typing import Dict, List, Tuple

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


FSOTS_STRUCT_RE = re.compile(r"\bstruct\s+(FSOTS_[A-Za-z0-9_]+)")


def scan_fsots_structs(root: Path) -> Tuple[int, Dict[str, List[Path]]]:
    """
    Scan for 'struct FSOTS_*' definitions under root in .h/.hpp/.cpp files.

    Returns:
      total_unique_structs,
      name_to_files_map
    """
    name_to_files: Dict[str, List[Path]] = {}

    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in {".h", ".hpp", ".cpp"}:
            continue

        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue

        for match in FSOTS_STRUCT_RE.finditer(text):
            name = match.group(1)
            name_to_files.setdefault(name, []).append(path)

    total_unique = len(name_to_files)
    return total_unique, name_to_files


def summarize_fsots_dupes(name_to_files: Dict[str, List[Path]]) -> Tuple[bool, int, List[str]]:
    """
    Given name_to_files map, determine if there are duplicates.
    Returns:
      has_dupes,
      duplicate_count,
      duplicate_names (sorted)
    """
    dup_names: List[str] = []
    for name, files in name_to_files.items():
        if len(files) > 1:
            dup_names.append(name)

    dup_names_sorted = sorted(dup_names)
    has_dupes = len(dup_names_sorted) > 0
    dup_count = len(dup_names_sorted)
    return has_dupes, dup_count, dup_names_sorted


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Report all FSOTS_* struct definitions and duplicates across the project."
    )
    parser.add_argument(
        "--root",
        type=str,
        default="",
        help="Project root (defaults to auto-detected project root).",
    )
    parser.add_argument(
        "--name",
        type=str,
        default="",
        help="Filter to a single FSOTS_* name (optional).",
    )
    args = parser.parse_args()

    confirm_start("fsots_duplicate_report")

    project_root = Path(args.root).resolve() if args.root else get_project_root()
    print(f"[INFO] Project root: {project_root}")

    total_unique, name_to_files = scan_fsots_structs(project_root)
    has_dupes, dup_count, dup_names = summarize_fsots_dupes(name_to_files)

    filter_name = args.name.strip()
    if filter_name and not filter_name.startswith("FSOTS_"):
        filter_name = "FSOTS_" + filter_name

    print("\n=== FSOTS Struct Duplicate Report ===")
    print(f"Scan root:           {project_root}")
    print(f"Unique FSOTS_*:      {total_unique}")
    print(f"Duplicate names:     {dup_count}")
    print(f"Has duplicates:      {has_dupes}")
    if filter_name:
        print(f"Filter name:         {filter_name}")

    # Detailed listing
    print("\n--- Details ---")

    if filter_name:
        names_to_show = [filter_name] if filter_name in name_to_files else []
    else:
        names_to_show = sorted(name_to_files.keys())

    if not names_to_show:
        print("[INFO] No FSOTS_* structs found matching filter.")
    else:
        for name in names_to_show:
            files = name_to_files[name]
            # Safely compute relative paths where possible
            rels: List[str] = []
            for p in files:
                try:
                    rels.append(str(p.relative_to(project_root)))
                except ValueError:
                    # Not under project_root (symlink or odd path) – just use full path
                    rels.append(str(p))

            tag = " (DUPLICATE)" if len(files) > 1 else ""
            print(f"\n{name}{tag}")
            for rel in rels:
                print(f"  - {rel}")

    status = "OK"
    if has_dupes:
        status = "WARN"

    # For logging, don't spam the full paths: keep names and counts.
    fsots_counts = {name: len(files) for name, files in name_to_files.items()}

    print()
    print_llm_summary(
        "fsots_duplicate_report",
        status=status,
        PROJECT_ROOT=str(project_root),
        FSOTS_STRUCTS_TOTAL=total_unique,
        FSOTS_DUPLICATE_COUNT=dup_count,
        FSOTS_DUPLICATE_NAMES=dup_names,
        FSOTS_HAS_DUPES=has_dupes,
        FSOTS_COUNTS=fsots_counts,
        FILTER_NAME=filter_name or None,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
