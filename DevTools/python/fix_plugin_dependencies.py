from __future__ import annotations
import argparse
from pathlib import Path

from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def summarize_log(log_path: Path):
    text = log_path.read_text(encoding="utf-8", errors="ignore")
    lines = text.splitlines()
    interesting = []
    for line in lines:
        if ".uplugin" in line or "Missing Module" in line or "Unable to find module" in line:
            interesting.append(line.strip())
    return interesting


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize plugin-related dependency issues from a UBT log.")
    parser.add_argument("--log", type=str, required=True, help="Path to UBT log file.")
    parser.add_argument("--dry-run", action="store_true", help="Reserved for future automatic fixes.")
    args = parser.parse_args()

    confirm_start("fix_plugin_dependencies")

    log_path = Path(args.log).resolve()
    if not log_path.exists():
        print(f"[ERROR] Log file not found: {log_path}")
        print_llm_summary(
            "fix_plugin_dependencies",
            LOG=str(log_path),
            FOUND=False,
            LINES=0,
        )
        confirm_exit()
        return

    print(f"[INFO] Reading log: {log_path}")
    interesting = summarize_log(log_path)

    if interesting:
        print("\n[PLUGIN-RELATED LINES]")
        for line in interesting:
            print(f"  {line}")
    else:
        print("[INFO] No obvious plugin-related lines found.")

    print_llm_summary(
        "fix_plugin_dependencies",
        LOG=str(log_path),
        FOUND=True,
        LINES=len(interesting),
    )

    confirm_exit()


if __name__ == "__main__":
    main()
