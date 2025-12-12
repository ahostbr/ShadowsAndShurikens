from __future__ import annotations
import argparse
from pathlib import Path
from typing import List, Tuple, Set

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def find_latest_log(log_dir: Path) -> Path | None:
    if not log_dir.exists():
        return None

    latest_path: Path | None = None
    latest_mtime: float = -1.0

    for p in log_dir.glob("*.log"):
        try:
            mtime = p.stat().st_mtime
        except OSError:
            continue
        if mtime > latest_mtime:
            latest_mtime = mtime
            latest_path = p

    return latest_path


def summarize_log(path: Path) -> Tuple[str | None, List[str], Set[str]]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    lines = text.splitlines()

    fatal_line: str | None = None
    context: List[str] = []
    in_fatal_block = False

    modules: Set[str] = set()

    for line in lines:
        if "Fatal error:" in line:
            fatal_line = line.strip()
            in_fatal_block = True
            context.append(line.strip())
            continue

        if in_fatal_block:
            if line.strip() == "":
                in_fatal_block = False
            else:
                context.append(line.rstrip())

        tokens = line.replace("(", " ").replace(")", " ").replace(",", " ").split()
        for t in tokens:
            if "SOTS_" in t or "UE5PMS" in t or "OmniTrace" in t:
                modules.add(t.strip())

    return fatal_line, context, modules


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize latest crash/editor log.")
    parser.add_argument("--log-dir", type=str, default=None, help="Logs directory (default: <ProjectRoot>/Saved/Logs).")
    args = parser.parse_args()

    confirm_start("summarize_crash_logs")

    project_root = get_project_root()
    if args.log_dir:
        log_dir = Path(args.log_dir).resolve()
    else:
        log_dir = project_root / "Saved" / "Logs"

    print(f"[INFO] Searching logs under: {log_dir}")
    latest = find_latest_log(log_dir)

    if not latest:
        print("[WARN] No .log files found.")
        print_llm_summary(
            "summarize_crash_logs",
            LOG_DIR=str(log_dir),
            LOG_FILE="",
            HAS_FATAL=False,
            MODULE_COUNT=0,
        )
        confirm_exit()
        return

    print(f"[INFO] Latest log: {latest}")
    fatal_line, context, modules = summarize_log(latest)

    if fatal_line is None:
        print("[INFO] No 'Fatal error:' line found in latest log.")
    else:
        print("\n[FATAL ERROR]")
        print(f"  {fatal_line}")
        print("\n[CONTEXT]")
        for c in context[:20]:
            print(f"  {c}")
        if len(context) > 20:
            print(f"  ... ({len(context) - 20} more lines)")

    if modules:
        print("\n[MODULES/KEYWORDS]")
        for m in sorted(modules):
            print(f"  {m}")

    print_llm_summary(
        "summarize_crash_logs",
        LOG_DIR=str(log_dir),
        LOG_FILE=str(latest),
        HAS_FATAL=fatal_line is not None,
        MODULE_COUNT=len(modules),
    )

    confirm_exit()


if __name__ == "__main__":
    main()
