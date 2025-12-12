"""
Usage:
  1) In Unreal, run console commands: KEM_SelfTest and KEM_DumpCoverage.
  2) Quit the editor (or restart).
  3) Run this script from sots_tools.py menu:
     [KEM] Coverage Report (from logs)
  4) The script will parse the most recent log and summarize KEM coverage.
"""
from __future__ import annotations

import json
import re
from collections import Counter
from pathlib import Path
from typing import Iterable

from cli_utils import confirm_start, confirm_exit
from llm_log import print_llm_summary
from log_utils import append_log
from project_paths import get_project_root

CONFIG_PATH = Path(__file__).resolve().parent / "sots_tools_config.json"
DEFAULT_LOG_DIR = "Saved/Logs"
DEFAULT_PATTERNS = ["KEM_SelfTest:", "[KEM Coverage]"]
SUMMARY_PATTERN = re.compile(
    r"KEM_SelfTest:\s*(?P<total>\d+)\s+definitions,\s*(?P<valid>\d+)\s+valid,\s*(?P<invalid>\d+)\s+invalid",
    re.IGNORECASE,
)
DEF_PATTERN = re.compile(r"KEM_SelfTest:\s*(?P<name>[^:]+):\s*(?P<message>.+)", re.IGNORECASE)
COUNT_PATTERNS = [
    re.compile(r"Count\s*[:=]\s*(?P<count>\d+)", re.IGNORECASE),
    re.compile(r"(?P<count>\d+)\s+definitions", re.IGNORECASE),
    re.compile(r"\((?P<count>\d+)\)")
]


def load_config() -> dict:
    if not CONFIG_PATH.exists():
        return {}

    try:
        with CONFIG_PATH.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


def get_kem_coverage_config(config: dict) -> dict:
    defaults = {
        "execution_catalog_asset_path": "",
        "log_dir": DEFAULT_LOG_DIR,
        "log_search_patterns": DEFAULT_PATTERNS,
    }
    kem_cfg = config.get("kem_coverage", {})
    merged = {**defaults, **kem_cfg}
    return merged


def find_latest_log(log_dir: Path) -> Path | None:
    if not log_dir.exists() or not log_dir.is_dir():
        return None

    candidates = [path for path in log_dir.glob("*.log") if path.is_file()]
    if not candidates:
        return None

    candidates.sort(key=lambda path: path.stat().st_mtime, reverse=True)
    return candidates[0]


def iterate_log_lines(path: Path) -> Iterable[str]:
    try:
        with path.open("r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                yield line.rstrip()
    except Exception:
        return


def parse_count(line: str) -> int | None:
    for pattern in COUNT_PATTERNS:
        match = pattern.search(line)
        if match:
            try:
                return int(match.group("count"))
            except (TypeError, ValueError):
                continue
    return None


def extract_label_value(line: str, label: str) -> str | None:
    pattern = re.compile(rf"{label}\s*[:=]?\s*(?P<value>[^\s,:]+)", re.IGNORECASE)
    match = pattern.search(line)
    if match:
        return match.group("value").strip()

    parts = line.split(label, 1)
    if len(parts) > 1:
        tail = parts[1].strip()
        tail = tail.lstrip("=: ")
        tail = tail.split()[0] if tail else None
        if tail:
            return tail
    return None


def parse_log(path: Path, patterns: list[str]) -> dict:
    families: Counter[str] = Counter()
    positions: Counter[str] = Counter()
    issues: list[dict] = []
    summary = {"total": 0, "valid": 0, "invalid": 0}

    for line in iterate_log_lines(path):
        if not line:
            continue
        if not any(pattern in line for pattern in patterns):
            continue

        summary_match = SUMMARY_PATTERN.search(line)
        if summary_match:
            summary["total"] = int(summary_match.group("total"))
            summary["valid"] = int(summary_match.group("valid"))
            summary["invalid"] = int(summary_match.group("invalid"))
            continue

        def_match = DEF_PATTERN.search(line)
        if def_match:
            name = def_match.group("name").strip()
            message = def_match.group("message").strip()
            status = "info"
            lower = message.lower()
            if "invalid" in lower or "error" in lower:
                status = "invalid"
            elif "warning" in lower:
                status = "warning"
            elif "valid" in lower:
                status = "valid"

            if status in {"invalid", "warning", "error"}:
                issues.append({"definition": name, "status": status, "message": message})
            continue

        if "KEM_DumpCoverage" in line or "[KEM Coverage]" in line:
            family_name = extract_label_value(line, "Family")
            position_name = extract_label_value(line, "Position")
            count = parse_count(line)
            if count is None:
                continue
            if family_name:
                families[family_name] += count
            if position_name:
                positions[position_name] += count

    return {
        "total_definitions": summary["total"],
        "valid_definitions": summary["valid"],
        "invalid_definitions": summary["invalid"],
        "families": dict(families),
        "positions": dict(positions),
        "definitions_with_issues": issues,
    }


def format_summary(summary: dict, catalog_path: str, log_file: Path, project_root: Path) -> None:
    relative_log = log_file
    try:
        relative_log = log_file.relative_to(project_root)
    except Exception:
        pass

    print("\n=== KEM Coverage Report ===")
    print(f"Catalog asset: {catalog_path or 'Not configured'}")
    print(f"Log file: {relative_log}")
    print(
        f"Definitions: {summary['total_definitions']} (valid {summary['valid_definitions']}, invalid {summary['invalid_definitions']})"
    )

    if summary["families"]:
        print("Families:")
        for name, count in summary["families"].items():
            print(f"  {name}: {count}")

    if summary["positions"]:
        print("Positions:")
        for name, count in summary["positions"].items():
            print(f"  {name}: {count}")

    if summary["definitions_with_issues"]:
        print("Definitions with issues:")
        for issue in summary["definitions_with_issues"]:
            print(f"  {issue['definition']} ({issue['status']}): {issue['message']}")
    else:
        print("Definitions with issues: None")


def main() -> None:
    confirm_start("kem_coverage_report")

    project_root = get_project_root()
    config = load_config()
    coverage_cfg = get_kem_coverage_config(config)
    log_dir = project_root / coverage_cfg.get("log_dir", DEFAULT_LOG_DIR)
    log_patterns = coverage_cfg.get("log_search_patterns", DEFAULT_PATTERNS)

    latest_log = find_latest_log(log_dir)
    if not latest_log:
        print("[WARN] No log files found in {}".format(log_dir))
        print_llm_summary(
            "kem_coverage_report",
            status="WARN",
            error="NO_LOGS",
            log_dir=str(log_dir),
        )
        confirm_exit()
        return

    summary = parse_log(latest_log, log_patterns)
    format_summary(summary, coverage_cfg.get("execution_catalog_asset_path", ""), latest_log, project_root)

    logs_dir = Path(__file__).resolve().parent / "Logs"
    logs_dir.mkdir(parents=True, exist_ok=True)
    append_log(
        logs_dir / "kem_coverage_report.log",
        f"Parsed {latest_log.name}: {summary['total_definitions']} total, {summary['invalid_definitions']} invalid",
    )

    print_llm_summary(
        "kem_coverage_report",
        status="OK",
        log_file=str(latest_log.relative_to(project_root)) if latest_log.is_relative_to(project_root) else str(latest_log),
        catalog_asset=coverage_cfg.get("execution_catalog_asset_path", ""),
        total_definitions=summary["total_definitions"],
        valid_definitions=summary["valid_definitions"],
        invalid_definitions=summary["invalid_definitions"],
        family_counts=summary["families"],
        position_counts=summary["positions"],
        definitions_with_issues=summary["definitions_with_issues"],
    )

    confirm_exit()


if __name__ == "__main__":
    main()
