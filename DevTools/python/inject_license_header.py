from __future__ import annotations
import argparse
from pathlib import Path
from typing import List

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def inject_header(root: Path, header_text: str, exts: List[str], dry_run: bool) -> int:
    files_modified = 0
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in exts:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if header_text.strip() in text[: len(header_text) + 200]:
            continue
        files_modified += 1
        if dry_run:
            print(f"[DRY-RUN] Would inject header into: {path}")
        else:
            print(f"[INJECT] {path}")
            path.write_text(header_text + "\n" + text, encoding="utf-8")
    return files_modified


def main() -> None:
    parser = argparse.ArgumentParser(description="Inject license header into source files.")
    parser.add_argument("--root", type=str, default=".", help="Root folder (default: project root).")
    parser.add_argument("--header-file", type=str, required=True, help="Path to header text file.")
    parser.add_argument("--exts", nargs="*", default=[".h", ".cpp", ".cs"], help="File extensions to target.")
    parser.add_argument("--dry-run", action="store_true", help="Only print what would be changed.")
    args = parser.parse_args()

    confirm_start("inject_license_header")

    project_root = get_project_root()
    root = Path(args.root)
    if not root.is_absolute():
        root = project_root / root

    header_path = Path(args.header_file).resolve()
    if not header_path.exists():
        print(f"[ERROR] Header file not found: {header_path}")
        print_llm_summary(
            "inject_license_header",
            ROOT=str(root),
            HEADER=str(header_path),
            FILES_MODIFIED=0,
            ERROR="HEADER_NOT_FOUND",
        )
        confirm_exit()
        return

    header_text = header_path.read_text(encoding="utf-8")

    print(f"[INFO] Root: {root}")
    print(f"[INFO] Header file: {header_path}")
    print(f"[INFO] Extensions: {args.exts}")

    files_modified = inject_header(root, header_text, args.exts, args.dry_run)
    print(f"[SUMMARY] Files modified: {files_modified} (dry-run={args.dry_run})")

    print_llm_summary(
        "inject_license_header",
        ROOT=str(root),
        HEADER=str(header_path),
        FILES_MODIFIED=files_modified,
        DRY_RUN=args.dry_run,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
