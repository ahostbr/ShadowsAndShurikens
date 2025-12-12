from __future__ import annotations
import argparse
from pathlib import Path
from typing import List, Set

from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


ERROR_KEYWORDS = (
    "error c",
    "fatal error",
    " error: ",
    "error: ",
    "Error: ",
)


def extract_errors(lines: List[str], max_errors: int = 40) -> List[str]:
    errors: List[str] = []
    seen: Set[str] = set()

    for line in lines:
        stripped = line.strip()
        lower = stripped.lower()
        if any(k in lower for k in ERROR_KEYWORDS):
            key = stripped
            if key not in seen:
                seen.add(key)
                errors.append(stripped)
                if len(errors) >= max_errors:
                    break

    return errors


def guess_plugins_from_errors(errors: List[str]) -> Set[str]:
    plugins: Set[str] = set()

    for e in errors:
        parts = e.replace("\\", "/").split("/")
        if "Plugins" in parts:
            idx = parts.index("Plugins")
            if idx + 1 < len(parts):
                plugins.add(parts[idx + 1])

    return plugins


def main() -> None:
    parser = argparse.ArgumentParser(description="Analyze a build log and summarize compiler errors.")
    parser.add_argument("--log", type=str, required=True, help="Path to build log file to analyze.")
    args = parser.parse_args()

    confirm_start("analyze_build_log")

    log_path = Path(args.log).resolve()
    if not log_path.exists():
        print(f"[ERROR] Build log not found: {log_path}")
        print_llm_summary(
            "analyze_build_log",
            LOG=str(log_path),
            ERROR="LOG_NOT_FOUND",
            ERROR_COUNT=0,
            PLUGINS="",
        )
        confirm_exit()
        return

    text = log_path.read_text(encoding="utf-8", errors="ignore")
    lines = text.splitlines()

    errors = extract_errors(lines, max_errors=40)

    if not errors:
        print("[INFO] No obvious compiler errors found in log.")
        print_llm_summary(
            "analyze_build_log",
            LOG=str(log_path),
            ERROR_COUNT=0,
            PLUGINS="",
        )
        confirm_exit()
        return

    print("\n[ERRORS]")
    for e in errors:
        print(f"  {e}")

    plugins = guess_plugins_from_errors(errors)

    print("\n[SUMMARY]")
    print(f"  Total error lines: {len(errors)}")
    if plugins:
        print(f"  Plugins mentioned in error paths: {', '.join(sorted(plugins))}")
    else:
        print("  No plugin names could be inferred from paths.")

    print_llm_summary(
        "analyze_build_log",
        LOG=str(log_path),
        ERROR_COUNT=len(errors),
        PLUGINS=",".join(sorted(plugins)) if plugins else "",
    )

    confirm_exit()


if __name__ == "__main__":
    main()
