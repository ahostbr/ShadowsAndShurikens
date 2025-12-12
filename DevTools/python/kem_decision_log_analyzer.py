"""Analyze KEM decision logs to surface request counts, reject reasons, and winners."""

from __future__ import annotations

import json
import re
from collections import Counter
from pathlib import Path
from typing import Iterable

from cli_utils import confirm_exit, confirm_start
from llm_log import print_llm_summary
from project_paths import get_project_root

CONFIG_FILE = Path(__file__).resolve().parent / "sots_tools_config.json"
DEFAULT_LOG_DIR = "Saved/Logs"
DEFAULT_LOG_PATTERNS = [
    "KEM: Candidate",
    "KEM: No valid execution",
]
DEFAULT_RECENT_LOGS = 5

CANDIDATE_PATTERN = re.compile(
    r"KEM: Candidate '(?P<candidate>[^']+)'[^\n]*?Score=(?P<score>[0-9.+-Ee]+)[^\n]*?Selected=(?P<selected>True|False|true|false)[^\n]*?Reason=(?P<reason>[A-Za-z0-9_]+)",
)
NO_VALID_PATTERN = re.compile(r"KEM: No valid execution for Instigator")
POSITION_TAG_PATTERN = re.compile(r"PositionTag=([A-Za-z0-9_.]+)")
REJECT_REASON_PATTERN = re.compile(r"RejectReason=([A-Za-z0-9_]+)")


def load_config() -> dict:
    if not CONFIG_FILE.exists():
        return {}

    try:
        with CONFIG_FILE.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as exc:
        print(f"[WARN] Failed to read config: {exc}")
        return {}


def resolve_log_paths(project_root: Path, kem_config: dict) -> list[Path]:
    log_dir = kem_config.get("default_log_dir", DEFAULT_LOG_DIR)
    logs_path = project_root / log_dir
    if not logs_path.exists():
        return []

    explicit = kem_config.get("log_files")
    if explicit:
        if isinstance(explicit, str):
            explicit = [explicit]
        explicit_paths = [logs_path / item for item in explicit]
        return [path for path in explicit_paths if path.exists()]

    candidates = [path for path in logs_path.glob("*.log")]
    candidates.sort(key=lambda path: path.stat().st_mtime, reverse=True)
    max_logs = kem_config.get("recent_logs", DEFAULT_RECENT_LOGS)
    return candidates[:max_logs]


def iterate_lines(paths: Iterable[Path]) -> Iterable[tuple[Path, str]]:
    for path in paths:
        try:
            with path.open("r", encoding="utf-8", errors="ignore") as f:
                for line in f:
                    yield path, line.rstrip("\n")
        except Exception:
            continue


def matches_patterns(line: str, patterns: list[str]) -> bool:
    return any(pattern in line for pattern in patterns)


def analyze_logs(paths: list[Path], patterns: list[str]) -> tuple[int, int, Counter[str], Counter[str]]:
    total_requests = 0
    no_execution = 0
    reject_reasons: Counter[str] = Counter()
    winner_positions: Counter[str] = Counter()

    for path, line in iterate_lines(paths):
        if not matches_patterns(line, patterns):
            continue

        candidate_match = CANDIDATE_PATTERN.search(line)
        if candidate_match:
            selected = candidate_match.group("selected").lower() == "true"
            reason = candidate_match.group("reason")
            if selected:
                total_requests += 1
                tag_match = POSITION_TAG_PATTERN.search(line)
                if tag_match:
                    winner_positions[tag_match.group(1)] += 1
            else:
                if reason:
                    reject_reasons[reason] += 1
            continue

        if NO_VALID_PATTERN.search(line):
            total_requests += 1
            no_execution += 1
            reason_match = REJECT_REASON_PATTERN.search(line)
            if reason_match:
                reject_reasons[reason_match.group(1)] += 1

    return total_requests, no_execution, reject_reasons, winner_positions


def format_summary(total: int, no_exec: int, reject_reasons: Counter[str], winner_positions: Counter[str]) -> list[str]:
    lines = ["[KEM Decision Log Summary]", f"Total Requests: {total}"]
    if total:
        percent = (no_exec / total) * 100
        lines.append(f"No Execution: {no_exec} ({percent:.1f}% of requests)")
    else:
        lines.append(f"No Execution: {no_exec}")

    lines.append("RejectReason frequency:")
    if reject_reasons:
        for reason, count in reject_reasons.most_common():
            lines.append(f"  {reason}: {count}")
    else:
        lines.append("  None detected")

    lines.append("Top winners by PositionTag:")
    if winner_positions:
        for tag, count in winner_positions.most_common(5):
            lines.append(f"  {tag}: {count}")
    else:
        lines.append("  None detected")

    return lines


def main() -> None:
    confirm_start("kem_decision_log_analyzer")

    project_root = get_project_root()
    config = load_config()
    kem_config = config.get("kem", {}) if config else {}

    log_paths = resolve_log_paths(project_root, kem_config)
    if not log_paths:
        print("[WARN] No log files found. Check kem.default_log_dir in config.")
        print_llm_summary(
            "kem_decision_log_analyzer",
            status="WARN",
            error="NO_LOGS",
        )
        confirm_exit()
        return

    log_patterns = kem_config.get("log_patterns", DEFAULT_LOG_PATTERNS)

    total_requests, no_execution, reject_reasons, winner_positions = analyze_logs(
        log_paths,
        log_patterns,
    )

    summary_lines = format_summary(total_requests, no_execution, reject_reasons, winner_positions)
    print("\n" + "\n".join(summary_lines) + "\n")

    print_llm_summary(
        "kem_decision_log_analyzer",
        total_requests=total_requests,
        no_execution=no_execution,
        reject_reason_counts=dict(reject_reasons),
        top_winner_position_tags=winner_positions.most_common(5),
        log_files=[str(path.relative_to(project_root)) for path in log_paths],
    )

    confirm_exit()


if __name__ == "__main__":
    main()
