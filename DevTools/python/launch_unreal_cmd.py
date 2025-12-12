#!/usr/bin/env python3
"""Launcher for UnrealEditor-Cmd from DevTools/python."""

from __future__ import annotations

import argparse
import datetime
import os
import subprocess
import sys
from pathlib import Path
from typing import Iterable


def detect_devtools_root() -> Path:
    """Return the DevTools/python directory this script lives in."""

    return Path(__file__).resolve().parent


def ensure_logs(root: Path) -> Path:
    """Create the logs directory if needed and return the path."""

    logs = root / "logs"
    logs.mkdir(parents=True, exist_ok=True)
    return logs


def timestamp() -> str:
    return datetime.datetime.now().strftime("%Y%m%d_%H%M%S")


def write_log(logs_dir: Path, prefix: str, payload: str) -> Path:
    log_path = logs_dir / f"{prefix}_{timestamp()}.log"
    try:
        with log_path.open("w", encoding="utf-8") as f:
            f.write(payload)
    except Exception as exc:
        print(f"[WARN] Failed to write log '{log_path}': {exc}", file=sys.stderr)
    return log_path


def build_command(ue_cmd: str, uproject: str, extra_args: Iterable[str]) -> list[str]:
    cmd = [ue_cmd, uproject]
    cmd.extend(extra_args)
    return cmd


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run UnrealEditor-Cmd.exe from DevTools/python with optional extra args."
    )

    parser.add_argument(
        "--ue-cmd",
        type=str,
        default=None,
        help="Path to UnrealEditor-Cmd.exe (fallbacks to SOTS_BPGEN_UE_CMD or UNREAL_EDITOR_CMD).",
    )
    parser.add_argument(
        "--uproject",
        type=str,
        default=None,
        help="Path to the .uproject file (fallbacks to SOTS_BPGEN_UPROJECT or UNREAL_UPROJECT).",
    )
    parser.add_argument(
        "--unreal-arg",
        action="append",
        default=[],
        help="Additional tokens appended to the UnrealEditor-Cmd invocation."
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the resolved command without spawning UnrealEditor-Cmd."
    )

    return parser.parse_args()


def main(argv: list[str] | None = None) -> int:
    if argv is None:
        argv = sys.argv[1:]

    args = parse_args()
    devtools_root = detect_devtools_root()
    logs_dir = ensure_logs(devtools_root)

    ue_cmd = (
        args.ue_cmd
        or os.environ.get("SOTS_BPGEN_UE_CMD")
        or os.environ.get("UNREAL_EDITOR_CMD")
    )
    uproject = (
        args.uproject
        or os.environ.get("SOTS_BPGEN_UPROJECT")
        or os.environ.get("UNREAL_UPROJECT")
    )

    log_lines: list[str] = []
    log_lines.append("launch_unreal_cmd.py started")
    log_lines.append(f"DevTools root : {devtools_root}")
    log_lines.append("")

    if not ue_cmd:
        log_lines.append("UE command   : (missing)")
    else:
        log_lines.append(f"UE command   : {ue_cmd}")

    if not uproject:
        log_lines.append("Project path : (missing)")
    else:
        log_lines.append(f"Project path : {uproject}")

    log_lines.append("Extra args   : " + " ".join(args.unreal_arg or ["(none)"]))
    log_lines.append("")

    if not ue_cmd or not uproject:
        msg = (
            "Missing required paths. Pass --ue-cmd/--uproject or set env vars "
            "(SOTS_BPGEN_UE_CMD/SOTS_BPGEN_UPROJECT or UNREAL_EDITOR_CMD/UNREAL_UPROJECT)."
        )
        print("[ERROR]", msg)
        log_lines.append("Execution    : skipped (missing paths)")
        log_path = write_log(logs_dir, "launch_unreal", "\n".join(log_lines))
        print(f"[INFO] Log written to: {log_path}")
        return 1

    command = build_command(ue_cmd, uproject, args.unreal_arg)
    formatted = " ".join(f'"{part}"' if " " in part else part for part in command)

    log_lines.append("Resolved command:")
    log_lines.append(f"  {formatted}")

    print("=== launch_unreal_cmd.py ===")
    print(formatted)
    print("")

    if args.dry_run:
        print("[INFO] Dry-run mode: not launching UnrealEditor-Cmd.exe.")
        log_lines.append("Execution    : dry-run")
        log_path = write_log(logs_dir, "launch_unreal", "\n".join(log_lines))
        print(f"[INFO] Log written to: {log_path}")
        return 0

    print("[INFO] Launching UnrealEditor-Cmd.exe...")
    try:
        completed = subprocess.run(command)
        exit_code = completed.returncode
        log_lines.append(f"Execution    : ran (exit {exit_code})")
        log_path = write_log(logs_dir, "launch_unreal", "\n".join(log_lines))
        print(f"[INFO] UnrealEditor-Cmd.exe exit code: {exit_code}")
        print(f"[INFO] Log written to: {log_path}")
        return exit_code
    except Exception as exc:
        msg = f"[ERROR] Failed to launch executable: {exc}"
        print(msg)
        log_lines.append(msg)
        log_path = write_log(logs_dir, "launch_unreal_error", "\n".join(log_lines))
        print(f"[INFO] Log written to: {log_path}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
