from __future__ import annotations

import argparse
import datetime
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent  # .../DevTools/python -> DevTools -> project root
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)
LOG_FILE = LOG_DIR / "regex_replace.log"


def log(msg: str) -> None:
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] [REGEX_REPLACE] {msg}"
    print(line)
    with LOG_FILE.open("a", encoding="utf-8") as f:
        f.write(line + "\n")


def iter_candidate_files(root: Path, exts_pattern: str | None) -> list[Path]:
    if exts_pattern:
        ext_re = re.compile(exts_pattern)
    else:
        ext_re = None

    files: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if ext_re is not None:
            # Use suffix (including the dot) as the "extension"
            if not ext_re.search(path.suffix):
                continue
        files.append(path)
    return files


def process_file(path: Path, pattern: re.Pattern[str], replace: str, dry_run: bool) -> int:
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        log(f"WARNING: Failed to read {path}: {e}")
        return 0

    new_text, count = pattern.subn(replace, text)
    if count > 0:
        if dry_run:
            log(f"DRY-RUN: Would replace {count} matches in {path}")
        else:
            try:
                path.write_text(new_text, encoding="utf-8")
                log(f"Replaced {count} matches in {path}")
            except Exception as e:
                log(f"ERROR: Failed to write {path}: {e}")
                return 0
    return count


def main() -> None:
    parser = argparse.ArgumentParser(description="SOTS DevTools: regex replace across project")
    parser.add_argument("--search", required=True, help="Regex pattern to search for")
    parser.add_argument("--replace", required=True, help="Replacement pattern")
    parser.add_argument(
        "--exts",
        default=None,
        help=r"Regex for file extensions, e.g. '\.cpp|\.h'. If omitted, all files are scanned.",
    )
    parser.add_argument(
        "--root",
        default=str(PROJECT_ROOT),
        help="Root directory to scan (defaults to project root)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Only log what would change; do not modify files.",
    )
    args = parser.parse_args()

    root = Path(args.root).resolve()
    if not root.is_dir():
        log(f"ERROR: Root directory does not exist or is not a dir: {root}")
        return

    try:
        pattern = re.compile(args.search)
    except re.error as e:
        log(f"ERROR: Invalid regex pattern {args.search!r}: {e}")
        return

    log(
        f"Starting regex_replace: root={root}, search={args.search!r}, "
        f"replace={args.replace!r}, exts={args.exts!r}, dry_run={args.dry_run}"
    )

    files = iter_candidate_files(root, args.exts)
    log(f"Found {len(files)} candidate files to scan.")

    total_matches = 0
    for path in files:
        count = process_file(path, pattern, args.replace, args.dry_run)
        total_matches += count

    log(f"Completed regex_replace. Total matches: {total_matches}, dry_run={args.dry_run}")
    print(f"[REGEX_REPLACE] Done. Total matches: {total_matches}, dry_run={args.dry_run}")


if __name__ == "__main__":
    main()
