from __future__ import annotations

import argparse
import shlex
import subprocess
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)


def now_tag() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="SOTS DevTools: generic script launcher")
    ap.add_argument("--script", required=True, help="Script name in DevTools/python")
    ap.add_argument("--args", default="", help="Raw argument string")
    args = ap.parse_args(argv)

    ts = now_tag()
    log_path = LOG_DIR / f"run_devtool_{ts}.log"

    script_path = ROOT / args.script
    if not script_path.is_file():
        msg = f"[RUN_DEVTOOL] ERROR: script not found: {script_path}"
        print(msg)
        with log_path.open("w", encoding="utf-8") as log:
            print(msg, file=log)
        return 1

    argv_list = [sys.executable, str(script_path)]
    if args.args.strip():
        argv_list.extend(shlex.split(args.args.strip()))

    with log_path.open("w", encoding="utf-8") as log:
        print(f"[RUN_DEVTOOL] cmd={argv_list}", file=log)

    proc = subprocess.Popen(argv_list, cwd=str(ROOT))
    print(f"[RUN_DEVTOOL] Launched PID={proc.pid}. Log: {log_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
