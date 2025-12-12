from __future__ import annotations

import argparse
import csv
import datetime
import json
from dataclasses import dataclass
from pathlib import Path

from cli_utils import confirm_exit, confirm_start
from llm_log import print_llm_summary
from project_paths import get_project_root

CONFIG_FILE = Path(__file__).resolve().parent / "sots_tools_config.json"
DEFAULT_LOG_DIR = "Saved/Logs"
DEFAULT_OUTPUT_DIR = Path(__file__).resolve().parent / "DevTools_Output"
DEFAULT_LOG_PREFIX = "[KEM_COV]"


@dataclass
class CoverageCell:
    family_tag: str
    position_tag: str
    target_kind: str
    definition_count: int = 0
    has_boss: bool = False
    has_dragon: bool = False


def load_config() -> dict:
    if not CONFIG_FILE.exists():
        return {}

    try:
        with CONFIG_FILE.open("r", encoding="utf-8") as fh:
            return json.load(fh)
    except Exception:
        return {}


def find_latest_log(log_dir: Path) -> Path | None:
    if not log_dir.exists():
        return None

    candidates = sorted(log_dir.glob("*.log"), key=lambda path: path.stat().st_mtime, reverse=True)
    return candidates[0] if candidates else None


def resolve_log_file(project_root: Path, cfg: dict, override: str | None) -> Path | None:
    if override:
        path = Path(override)
        if not path.is_absolute():
            path = project_root / path
        return path if path.exists() else None

    log_dir = project_root / cfg.get("log_dir", DEFAULT_LOG_DIR)
    return find_latest_log(log_dir)


def clean_value(token: str) -> str:
    return token.strip('"\'",.;)[]')


def parse_line(line: str, prefix: str) -> dict[str, str]:
    if prefix not in line:
        return {}

    payload = line.split(prefix, 1)[-1]
    if "]" in payload:
        payload = payload.split("]", 1)[-1]
    data: dict[str, str] = {}
    for token in payload.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        data[key] = clean_value(value)
    return data


def bool_from_token(token: str) -> bool:
    return token.lower() in {"1", "true", "yes"}


def analyze_log(path: Path, prefix: str) -> tuple[dict[tuple[str, str, str], CoverageCell], int]:
    cells: dict[tuple[str, str, str], CoverageCell] = {}
    coverage_lines = 0

    with path.open("r", encoding="utf-8", errors="ignore") as fh:
        for raw_line in fh:
            line = raw_line.strip()
            if prefix not in line:
                continue
            coverage_lines += 1
            data = parse_line(line, prefix)
            if not data:
                continue

            family = data.get("Family", "Unknown")
            position = data.get("Position", "Unknown")
            target = data.get("Target", "Generic")
            count = 0
            if value := data.get("Count"):
                try:
                    count = int(value)
                except ValueError:
                    count = 0
            boss = bool_from_token(data.get("Boss", "0"))
            dragon = bool_from_token(data.get("Dragon", "0"))

            key = (family, position, target)
            cell = cells.setdefault(key, CoverageCell(family, position, target))
            cell.definition_count += count
            cell.has_boss = cell.has_boss or boss
            cell.has_dragon = cell.has_dragon or dragon

    return cells, coverage_lines


def write_csv(cells: list[CoverageCell], output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    output_path = output_dir / f"KEM_CoverageMatrix_{timestamp}.csv"

    with output_path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        writer.writerow([
            "FamilyTag",
            "PositionTag",
            "TargetKind",
            "DefinitionCount",
            "HasBossOnly",
            "HasDragonOnly",
        ])
        for cell in sorted(cells, key=lambda c: (c.family_tag, c.position_tag, c.target_kind)):
            writer.writerow([
                cell.family_tag,
                cell.position_tag,
                cell.target_kind,
                cell.definition_count,
                "1" if cell.has_boss else "0",
                "1" if cell.has_dragon else "0",
            ])
    return output_path


def format_empty_cells(cells: list[CoverageCell]) -> list[str]:
    return [f"  Family={cell.family_tag} Position={cell.position_tag} Target={cell.target_kind}" for cell in cells]


def main() -> None:
    confirm_start("kem_coverage_matrix_report")

    project_root = get_project_root()
    config = load_config()
    coverage_cfg = config.get("kem_coverage", {})

    parser = argparse.ArgumentParser(description="Summarize KEM coverage cells from log output.")
    parser.add_argument("--log-file", help="Explicit log file path (relative to project root).")
    parser.add_argument("--log-prefix", help="Coverage log prefix (default from config).")
    parser.add_argument("--output-dir", help="Directory to save the CSV (relative to DevTools/python).")
    args = parser.parse_args()

    prefix = args.log_prefix or coverage_cfg.get("log_prefix", DEFAULT_LOG_PREFIX)
    log_path = resolve_log_file(project_root, coverage_cfg, args.log_file)
    if not log_path:
        print("[WARN] No log file found to parse coverage cells.")
        print_llm_summary("kem_coverage_matrix_report", status="NO_LOGS")
        confirm_exit()
        return

    cells_map, parsed_lines = analyze_log(log_path, prefix)
    cells = list(cells_map.values())
    total_cells = len(cells)
    non_empty = len([cell for cell in cells if cell.definition_count > 0])
    empty_cells = [cell for cell in cells if cell.definition_count == 0]

    output_dir = DEFAULT_OUTPUT_DIR
    if args.output_dir:
        custom = Path(args.output_dir)
        if not custom.is_absolute():
            custom = Path(__file__).resolve().parent / custom
        output_dir = custom
    elif (configured := coverage_cfg.get("output_dir")):
        configured_path = Path(configured)
        if not configured_path.is_absolute():
            configured_path = Path(__file__).resolve().parent / configured_path
        output_dir = configured_path

    output_path = write_csv(cells, output_dir)

    print("\n[KEM Coverage Summary]")
    print(f"Total coverage cells: {total_cells}")
    print(f"Non-empty: {non_empty}")
    print("Empty cells (no executions):")
    if empty_cells:
        for line in format_empty_cells(empty_cells):
            print(line)
    else:
        print("  None detected.")

    print(f"Parsed coverage lines : {parsed_lines}")
    print(f"Output CSV           : {output_path}")

    print_llm_summary(
        "kem_coverage_matrix_report",
        summary=(f"Cells={total_cells} NonEmpty={non_empty} Empty={len(empty_cells)}"),
        coverage_cells=total_cells,
        non_empty=non_empty,
        empty=len(empty_cells),
        output_file=str(output_path.relative_to(project_root)) if output_path.exists() else str(output_path),
        parsed_lines=parsed_lines,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
