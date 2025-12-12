import argparse
import json
import os
import sys
from datetime import datetime

VALID_STATUSES = {"todo", "in_progress", "done"}


def debug_print(msg: str) -> None:
    print(f"[devtools_status_dashboard] {msg}")


def ensure_dir(path: str) -> str:
    os.makedirs(path, exist_ok=True)
    return path


def load_status(path: str) -> dict:
    if not os.path.isfile(path):
        return {}
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        debug_print(f"WARNING: status file '{path}' unreadable; starting fresh.")
        return {}


def save_status(path: str, data: dict) -> None:
    ensure_dir(os.path.dirname(path))
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, sort_keys=True)


def format_dashboard(data: dict):
    lines = []
    lines.append("DEVTOOLS STATUS DASHBOARD")
    lines.append("-" * 72)
    if not data:
        lines.append("No status data yet. Use --mode update to add entries.")
        return lines
    for plugin, steps in sorted(data.items()):
        total = len(steps)
        done = sum(1 for s in steps.values() if s == "done")
        in_progress = sum(1 for s in steps.values() if s == "in_progress")
        todo = sum(1 for s in steps.values() if s == "todo")
        lines.append(
            f"{plugin}: done={done}, in_progress={in_progress}, todo={todo}, total={total}"
        )
        for step, status in sorted(steps.items()):
            lines.append(f"  - {step}: {status}")
        lines.append("")
    return lines


def write_log(log_dir: str, lines) -> str:
    ensure_dir(log_dir)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = os.path.join(log_dir, f"devtools_status_{ts}.log")
    with open(log_path, "w", encoding="utf-8") as f:
        for line in lines:
            f.write(line.rstrip("\n") + "\n")
    return log_path


def main(argv=None):
    parser = argparse.ArgumentParser(description="DevTools status dashboard & updater.")
    parser.add_argument("--mode", choices=["dashboard", "update"], default="dashboard")
    parser.add_argument("--plugin", type=str)
    parser.add_argument("--step", type=str)
    parser.add_argument("--status", type=str, choices=sorted(VALID_STATUSES))
    parser.add_argument(
        "--status-file",
        type=str,
        default=os.path.join("DevTools", "devtools_status.json"),
    )
    parser.add_argument(
        "--log-dir",
        type=str,
        default=os.path.join("DevTools", "logs"),
    )
    args = parser.parse_args(argv)

    debug_print("Starting devtools_status_dashboard")
    debug_print(f"Mode:        {args.mode}")
    debug_print(f"Status file: {args.status_file}")
    debug_print(f"Log dir:     {args.log_dir}")

    data = load_status(args.status_file)

    if args.mode == "update":
        if not args.plugin or not args.step or not args.status:
            debug_print("FATAL: --plugin, --step, and --status are required in update mode.")
            return 1
        if args.status not in VALID_STATUSES:
            debug_print(f"FATAL: invalid status '{args.status}'")
            return 1
        plugin = args.plugin
        step = args.step
        status = args.status
        if plugin not in data:
            data[plugin] = {}
        data[plugin][step] = status
        save_status(args.status_file, data)
        debug_print(f"Updated {plugin} / {step} -> {status}")

    lines = format_dashboard(data)
    for line in lines:
        print(line)
    log_path = write_log(args.log_dir, lines)
    debug_print(f"Dashboard log written to: {log_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
