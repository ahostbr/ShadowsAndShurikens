from __future__ import annotations

import argparse
import csv
import datetime
import json
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Sequence

from cli_utils import confirm_exit, confirm_start
from llm_log import print_llm_summary
from project_paths import get_project_root

CONFIG_FILE = Path(__file__).resolve().parent / "sots_tools_config.json"
TELEMETRY_TAG = "[KEM_TEL]"
STEP_TAG = "[KEM_STEP]"
ALLOWED_TAGS = {TELEMETRY_TAG, STEP_TAG}
DEFAULT_LOG_DIR = "Saved/Logs"
DEFAULT_LOG_FILE = "ShadowsAndShurikens.log"
DEFAULT_OUTPUT_DIR = Path(__file__).resolve().parent / "DevTools_Output"


@dataclass
class ExecutionTagStats:
    attempts: int = 0
    successes: int = 0
    failure_counts: Counter[str] = field(default_factory=Counter)
    distance_sum: float = 0.0
    distance_count: int = 0
    position_tag_counts: Counter[str] = field(default_factory=Counter)

    def average_distance(self) -> float | None:
        if self.distance_count == 0:
            return None
        return self.distance_sum / self.distance_count


def load_config() -> dict:
    if not CONFIG_FILE.exists():
        return {}

    try:
        with CONFIG_FILE.open("r", encoding="utf-8") as fh:
            return json.load(fh)
    except Exception:
        return {}


def resolve_log_paths(project_root: Path, telemetry_cfg: dict, override: str | None) -> list[Path]:
    if override:
        candidate = Path(override)
        if not candidate.is_absolute():
            candidate = project_root / candidate
        return [candidate] if candidate.exists() else []

    raw_dir = telemetry_cfg.get("log_dir", DEFAULT_LOG_DIR)
    log_dir = project_root / raw_dir
    files = telemetry_cfg.get("log_files")
    if files:
        if isinstance(files, str):
            files = [files]
        paths = []
        for entry in files:
            candidate = Path(entry)
            if not candidate.is_absolute():
                candidate = log_dir / candidate
            if candidate.exists():
                paths.append(candidate)
        return paths

    default_file = log_dir / telemetry_cfg.get("log_file", DEFAULT_LOG_FILE)
    return [default_file] if default_file.exists() else []


def clean_value(token: str) -> str:
    return token.strip('"\'",.;)[]')


def parse_key_values(line: str) -> dict[str, str]:
    payload = line
    if "]" in line:
        payload = line.split("]", 1)[-1]
    data: dict[str, str] = {}
    for token in payload.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        data[key.strip()] = clean_value(value)
    return data


def analyze_logs(paths: Sequence[Path]) -> tuple[int, Counter[str], dict[str, ExecutionTagStats], Counter[str], int, int, int]:
    outcome_counts: Counter[str] = Counter()
    execution_stats: dict[str, ExecutionTagStats] = {}
    position_tag_counts: Counter[str] = Counter()
    total_attempts = 0
    telemetry_lines = 0
    step_lines = 0

    for path in paths:
        try:
            with path.open("r", encoding="utf-8", errors="ignore") as fh:
                for raw_line in fh:
                    line = raw_line.rstrip()
                    has_tag = any(tag in line for tag in ALLOWED_TAGS)
                    if not has_tag:
                        continue
                    if TELEMETRY_TAG in line:
                        telemetry_lines += 1
                        is_attempt = True
                    else:
                        step_lines += 1
                        is_attempt = False
                    data = parse_key_values(line)
                    if not data:
                        continue

                    execution_tag = data.get("ExecutionTag") or data.get("ExecutionName") or "Unknown"
                    outcome = data.get("Outcome", "Unknown")
                    distance = None
                    if (distance_text := data.get("Distance")):
                        try:
                            distance = float(distance_text)
                        except ValueError:
                            distance = None

                    position_tag = data.get("ExecutionPositionTag") or data.get("PositionTag")

                    stats = execution_stats.setdefault(execution_tag, ExecutionTagStats())
                    if is_attempt:
                        total_attempts += 1
                        stats.attempts += 1
                        if outcome.lower() == "succeeded":
                            stats.successes += 1
                        else:
                            stats.failure_counts[outcome] += 1
                        outcome_counts[outcome] += 1
                        if distance is not None:
                            stats.distance_sum += distance
                            stats.distance_count += 1
                    if position_tag:
                        stats.position_tag_counts[position_tag] += 1
                        position_tag_counts[position_tag] += 1
        except Exception:
            continue

    return (
        total_attempts,
        outcome_counts,
        execution_stats,
        position_tag_counts,
        telemetry_lines,
        step_lines,
        len(paths),
    )


def write_summary_csv(execution_stats: dict[str, ExecutionTagStats], output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    output_path = output_dir / f"KEM_TelemetrySummary_{timestamp}.csv"

    with output_path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        writer.writerow([
            "ExecutionTag",
            "Attempts",
            "Successes",
            "Failures",
            "FailureBreakdown",
            "AverageDistance",
            "PositionTagCounts",
        ])
        for tag, stats in sorted(execution_stats.items(), key=lambda item: (-item[1].attempts, item[0])):
            if stats.attempts == 0:
                continue
            failure_total = sum(stats.failure_counts.values())
            failure_detail = ";".join(f"{name}:{count}" for name, count in stats.failure_counts.items())
            distance = stats.average_distance()
            pos_detail = ";".join(f"{name}:{count}" for name, count in stats.position_tag_counts.items())
            writer.writerow([
                tag,
                stats.attempts,
                stats.successes,
                failure_total,
                failure_detail,
                f"{distance:.2f}" if distance is not None else "",
                pos_detail,
            ])
    return output_path


def main() -> None:
    confirm_start("kem_telemetry_report")

    project_root = get_project_root()
    config = load_config()
    telemetry_cfg = config.get("kem_telemetry", {})

    parser = argparse.ArgumentParser(description="Summarize KEM telemetry lines from UE logs.")
    parser.add_argument("--log-file", help="Optional explicit log file to parse (relative to project root).")
    parser.add_argument(
        "--output-dir",
        help="Optional directory to write KEM per-tag summaries (relative to DevTools/python).",
    )
    args = parser.parse_args()

    log_paths = resolve_log_paths(project_root, telemetry_cfg, args.log_file)
    if not log_paths:
        print("[WARN] No log files found for telemetry parsing.")
        print_llm_summary("kem_telemetry_report", status="NO_LOGS")
        confirm_exit()
        return

    (
        total_attempts,
        outcome_counts,
        execution_stats,
        position_counts,
        telemetry_lines,
        step_lines,
        log_file_count,
    ) = analyze_logs(log_paths)

    average_distance = None
    distance_entries = sum(stats.distance_count for stats in execution_stats.values())
    distance_total = sum(stats.distance_sum for stats in execution_stats.values())
    if distance_entries:
        average_distance = distance_total / distance_entries

    output_dir = DEFAULT_OUTPUT_DIR
    if args.output_dir:
        custom = Path(args.output_dir)
        if not custom.is_absolute():
            custom = Path(__file__).resolve().parent / custom
        output_dir = custom
    elif (configured := telemetry_cfg.get("output_dir")):
        configured_path = Path(configured)
        if not configured_path.is_absolute():
            configured_path = Path(__file__).resolve().parent / configured_path
        output_dir = configured_path

    output_path = write_summary_csv(execution_stats, output_dir)

    print("\n=== KEM Telemetry Summary ===")
    print(f"Log files scanned : {log_file_count}")
    print(f"Telemetry lines   : {telemetry_lines}")
    print(f"Step lines        : {step_lines}")
    print(f"Total attempts    : {total_attempts}")
    print("\nOutcomes:")
    if outcome_counts:
        for outcome, count in outcome_counts.most_common():
            print(f"  {outcome}: {count}")
    else:
        print("  None detected.")

    print("\nTop execution tags:")
    top_tags = [
        (tag, stats)
        for tag, stats in sorted(execution_stats.items(), key=lambda item: (-item[1].attempts, item[0]))
        if stats.attempts > 0
    ][:10]
    if top_tags:
        for tag, stats in top_tags:
            failures = sum(stats.failure_counts.values())
            distance = stats.average_distance()
            avg_text = f", avg dist {distance:.2f}" if distance is not None else ""
            print(f"  {tag}: attempts={stats.attempts}, success={stats.successes}, failure={failures}{avg_text}")
    else:
        print("  No execution tags recorded.")

    print("\nPosition tag frequencies:")
    if position_counts:
        for tag, count in position_counts.most_common(10):
            print(f"  {tag}: {count}")
    else:
        print("  No position tags logged.")

    if average_distance is not None:
        print(f"\nAverage distance per execution: {average_distance:.2f}")
    else:
        print("\nAverage distance per execution: N/A")

    print(f"\nDetailed CSV saved to: {output_path}")

    print_llm_summary(
        "kem_telemetry_report",
        logs_scanned=log_file_count,
        telemetry_lines=telemetry_lines,
        step_lines=step_lines,
        total_attempts=total_attempts,
        outcomes=dict(outcome_counts),
        execution_tags={tag: stats.attempts for tag, stats in execution_stats.items()},
        position_tags=dict(position_counts),
        output_file=str(output_path.relative_to(project_root)) if output_path.exists() else str(output_path),
    )

    confirm_exit()


if __name__ == "__main__":
    main()
