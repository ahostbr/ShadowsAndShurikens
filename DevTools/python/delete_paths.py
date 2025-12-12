from __future__ import annotations

import argparse
import datetime
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)
LOG_FILE = LOG_DIR / "delete_paths.log"


def log(msg: str) -> None:
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] [DELETE_PATHS] {msg}"
    print(line)
    with LOG_FILE.open("a", encoding="utf-8") as f:
        f.write(line + "\n")


def resolve_path(raw: str) -> Path:
    p = Path(raw)
    if not p.is_absolute():
        p = PROJECT_ROOT / raw
    return p.resolve()


def delete_path(path: Path, dry_run: bool) -> None:
    if not path.exists():
        log(f"SKIP (missing): {path}")
        return

    if path.is_dir():
        if dry_run:
            log(f"DRY-RUN: Would delete directory {path}")
        else:
            try:
                shutil.rmtree(path)
                log(f"Deleted directory {path}")
            except Exception as e:
                log(f"ERROR deleting directory {path}: {e}")
    else:
        if dry_run:
            log(f"DRY-RUN: Would delete file {path}")
        else:
            try:
                path.unlink()
                log(f"Deleted file {path}")
            except Exception as e:
                log(f"ERROR deleting file {path}: {e}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="SOTS DevTools: delete files/folders by path (with dry-run support)"
    )
    parser.add_argument(
        "--paths",
        nargs="+",
        required=True,
        help="One or more paths (relative to project root or absolute) to delete.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Only log what would be deleted; do not actually delete anything.",
    )
    args = parser.parse_args()

    log(f"Starting delete_paths: paths={args.paths}, dry_run={args.dry_run}")

    for raw in args.paths:
        path = resolve_path(raw)
        delete_path(path, args.dry_run)

    log("Completed delete_paths run.")
    print(f"[DELETE_PATHS] Done. dry_run={args.dry_run}")


if __name__ == "__main__":
    main()
