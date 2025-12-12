"""Dev-oriented reporting tool for Execution DataAssets.

Checks that the expected stealth/mission constraint fields exist on each
definition, documents where they are referenced, and summarizes the
findings to aid stealth/mission tuning."""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Dict, Iterable, Sequence

from cli_utils import confirm_exit, confirm_start
from llm_log import print_llm_summary
from project_paths import get_project_root

DEFAULT_EXECUTION_DA_CLASS = "USOTS_KEM_ExecutionDefinition"
REQUIREMENT_FIELDS = [
    "bRequireTargetUndetected",
    "bAllowExecutionWhileDetected",
    "MinRequiredStealthTier",
    "MaxAllowedStealthTier",
    "RequiredMissionTags",
]
CAS_STRUCT_NAME = "FSOTS_KEM_CASConfig"
CAS_FIELDS = [
    "MinDistance",
    "MaxDistance",
    "MaxFacingAngleDegrees",
    "MaxSamePlaneHeightDelta",
    "InstigatorLocalOffsetFromTarget",
]
WARP_STRUCT_NAME = "FSOTS_KEM_WarpPointDef"
WARP_FIELDS = [
    "TargetName",
    "LocalOffset",
    "LocalRotationOffset",
    "MaxWarpDistance",
]
WARP_PROPERTY_FIELDS = [
    "WarpPoints",
    "InstigatorWarpPointNames",
    "TargetWarpPointNames",
]
POSITION_TAG_PATTERN = re.compile(r"SOTS\.KEM\.Position(?:\.[A-Za-z0-9_]+)+")
POSITION_ASSIGNMENT_PATTERN = re.compile(r"PositionTag\s*=\s*([^;]+);")
FILE_EXTENSIONS = {".h", ".cpp", ".ini"}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_config() -> dict:
    cfg_path = Path(__file__).resolve().parent / "sots_tools_config.json"
    if not cfg_path.exists():
        return {}

    try:
        with cfg_path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as exc:  # pragma: no cover - defensive
        print(f"[WARN] Failed to read config: {exc}")
        return {}


def build_scan_dirs(project_root: Path) -> Sequence[Path]:
    plugin_root = project_root / "Plugins" / "SOTS_KillExecutionManager"
    candidates = [
        plugin_root / "Source",
        plugin_root / "Config",
        project_root / "Source",
        project_root / "Config",
    ]
    return [candidate for candidate in candidates if candidate.exists()]


def collect_files(dirs: Sequence[Path], extensions: set[str]) -> list[Path]:
    seen: set[Path] = set()
    collected: list[Path] = []

    for directory in dirs:
        for path in directory.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in extensions:
                continue
            resolved = path.resolve()
            if resolved in seen:
                continue
            seen.add(resolved)
            collected.append(resolved)

    return collected


def iterate_lines(file_text: str) -> Iterable[tuple[int, str]]:
    for index, line in enumerate(file_text.splitlines(), start=1):
        yield index, line.rstrip()


def format_path(path: Path, root: Path) -> str:
    try:
        return str(path.relative_to(root))
    except ValueError:
        return str(path)


def extract_struct_block(text: str, struct_name: str) -> str | None:
    marker = f"struct {struct_name}"
    start_idx = text.find(marker)
    if start_idx == -1:
        return None

    brace_idx = text.find("{", start_idx)
    if brace_idx == -1:
        return None

    depth = 1
    current = brace_idx + 1
    while current < len(text) and depth > 0:
        char = text[current]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        current += 1

    if depth != 0:
        return None

    return text[brace_idx + 1 : current - 1]


def find_struct_fields(text: str, struct_name: str, field_names: Sequence[str]) -> tuple[list[str], list[str]]:
    block = extract_struct_block(text, struct_name)
    if not block:
        return [], list(field_names)

    found, missing = [], []
    for field in field_names:
        if field in block:
            found.append(field)
        else:
            missing.append(field)
    return found, missing


def find_field_matches(file_texts: Dict[Path, str], field_names: Sequence[str]) -> Dict[str, list[tuple[Path, int, str]]]:
    matches: Dict[str, list[tuple[Path, int, str]]] = {field: [] for field in field_names}
    for path, text in file_texts.items():
        for line_no, line in iterate_lines(text):
            trimmed = line.strip()
            for field in field_names:
                if field in line:
                    matches[field].append((path, line_no, trimmed))
    return matches


def find_position_assignments(file_texts: Dict[Path, str]) -> list[tuple[Path, int, str]]:
    results: list[tuple[Path, int, str]] = []
    for path, text in file_texts.items():
        for line_no, line in iterate_lines(text):
            match = POSITION_ASSIGNMENT_PATTERN.search(line)
            if match:
                results.append((path, line_no, line.strip()))
    return results


def collect_position_tags(file_texts: Dict[Path, str]) -> Dict[str, list[tuple[Path, int, str]]]:
    tags: Dict[str, list[tuple[Path, int, str]]] = {}
    for path, text in file_texts.items():
        for line_no, line in iterate_lines(text):
            for match in POSITION_TAG_PATTERN.findall(line):
                tags.setdefault(match, []).append((path, line_no, line.strip()))
    return tags


def summarize_position_tags(position_tags: Dict[str, list[tuple[Path, int, str]]]) -> Dict[str, dict]:
    summary: Dict[str, dict] = {}
    for tag, entries in position_tags.items():
        files = {entry[0] for entry in entries}
        summary[tag] = {
            "count": len(entries),
            "files": len(files),
            "example": entries[0],
        }
    return summary


def find_class_references(file_texts: Dict[Path, str], class_name: str) -> list[tuple[Path, int, str]]:
    pattern = re.compile(rf"\b{re.escape(class_name)}\b")
    matches: list[tuple[Path, int, str]] = []
    for path, text in file_texts.items():
        for line_no, line in iterate_lines(text):
            if pattern.search(line):
                matches.append((path, line_no, line.strip()))
    return matches


def print_matches(title: str, matches: list[tuple[Path, int, str]], project_root: Path, limit: int = 20) -> None:
    print(title)
    if not matches:
        print("  None found.")
        return

    for path, line_no, snippet in matches[:limit]:
        print(f"  {format_path(path, project_root)}:{line_no}    {snippet}")

    if len(matches) > limit:
        print(f"  ... ({len(matches) - limit} more matches)")


def print_field_summary(title: str, matches: Dict[str, list[tuple[Path, int, str]]], project_root: Path) -> None:
    print(title)
    for field, entries in matches.items():
        if entries:
            path, line_no, snippet = entries[0]
            print(f"  {field}: {format_path(path, project_root)}:{line_no}    {snippet}")
        else:
            print(f"  {field}: MISSING")


def print_position_summary(position_summary: Dict[str, dict], project_root: Path) -> None:
    print("KEM Position Summary:")
    if not position_summary:
        print("  No SOTS.KEM.Position.* tags found in scanned files.")
        return

    sorted_tags = sorted(position_summary.items(), key=lambda pair: (-pair[1]["count"], pair[0]))
    for tag, info in sorted_tags:
        path, line_no, _ = info["example"]
        print(
            f"  {tag}: {info['count']} references across {info['files']} files (example: {format_path(path, project_root)}:{line_no})"
        )


# ---------------------------------------------------------------------------
# Main logic
# ---------------------------------------------------------------------------

def main() -> None:
    confirm_start("kem_execution_report")

    project_root = get_project_root()
    config = load_config()
    kem_config = config.get("kem_execution", {})
    execution_class = kem_config.get("execution_da_class", DEFAULT_EXECUTION_DA_CLASS)

    scan_dirs = build_scan_dirs(project_root)
    if not scan_dirs:
        print("[ERROR] No directories found to scan. Check project layout.")
        print_llm_summary(
            "kem_execution_report",
            status="ERROR",
            error="NO_SCAN_DIRECTORIES",
        )
        confirm_exit()
        return

    files = collect_files(scan_dirs, FILE_EXTENSIONS)
    file_texts: Dict[Path, str] = {}
    for path in files:
        try:
            file_texts[path] = path.read_text(encoding="utf-8", errors="ignore")
        except Exception as exc:  # pragma: no cover - be defensive
            print(f"[WARN] Failed to read {path}: {exc}")

    requirement_matches = find_field_matches(file_texts, REQUIREMENT_FIELDS)
    position_tag_matches = find_field_matches(file_texts, ["PositionTag", "AdditionalPositionTags"])
    warp_property_matches = find_field_matches(file_texts, WARP_PROPERTY_FIELDS)
    position_assignments = find_position_assignments(file_texts)
    position_tag_tokens = collect_position_tags(file_texts)
    position_summary = summarize_position_tags(position_tag_tokens)
    execution_refs = find_class_references(file_texts, execution_class)

    types_path = (
        project_root
        / "Plugins"
        / "SOTS_KillExecutionManager"
        / "Source"
        / "SOTS_KillExecutionManager"
        / "Public"
        / "SOTS_KEM_Types.h"
    )
    cas_found, cas_missing = [], []
    warp_found, warp_missing = [], []
    if types_path.exists():
        try:
            types_text = types_path.read_text(encoding="utf-8", errors="ignore")
            cas_found, cas_missing = find_struct_fields(types_text, CAS_STRUCT_NAME, CAS_FIELDS)
            warp_found, warp_missing = find_struct_fields(types_text, WARP_STRUCT_NAME, WARP_FIELDS)
        except Exception as exc:  # pragma: no cover
            print(f"[WARN] Failed to read types header: {exc}")
    else:
        print("[WARN] SOTS_KEM_Types.h not found; skipping struct analysis.")

    print("\n=== KEM Execution Report ===")
    print(f"Project root: {project_root}")
    print(f"Execution DA class: {execution_class}")
    print(f"Scanned files: {len(file_texts)} (from {len(scan_dirs)} directories)")
    print("")

    print_matches(
        "ExecutionDefinition references:",
        execution_refs,
        project_root,
    )
    print("")
    print_field_summary("Stealth / requirement fields:", requirement_matches, project_root)
    print("")
    print_field_summary("Position fields:", position_tag_matches, project_root)
    print("")
    print("PositionTag defaults / assignments:")
    if position_assignments:
        for path, line_no, snippet in position_assignments:
            print(f"  {format_path(path, project_root)}:{line_no}    {snippet}")
    else:
        print("  None found in scanned source.")
    print("")
    print_position_summary(position_summary, project_root)
    print("")
    print_field_summary("Warp-related field declarations:", warp_property_matches, project_root)
    print("")
    print("")
    print("CASConfig struct fields detected:")
    if cas_found:
        print(f"  Found: {', '.join(cas_found)}")
    else:
        print("  No CASConfig fields detected.")
    if cas_missing:
        print(f"  Missing: {', '.join(cas_missing)}")
    print("")
    print("WarpPoint struct fields detected:")
    if warp_found:
        print(f"  Found: {', '.join(warp_found)}")
    else:
        print("  No WarpPoint fields detected.")
    if warp_missing:
        print(f"  Missing: {', '.join(warp_missing)}")
    print("")

    print_llm_summary(
        "kem_execution_report",
        files_scanned=len(file_texts),
        directories_scanned=len(scan_dirs),
        execution_references=len(execution_refs),
        position_tags={tag: info["count"] for tag, info in position_summary.items()},
        position_tag_files={tag: info["files"] for tag, info in position_summary.items()},
        requirement_fields={field: bool(entries) for field, entries in requirement_matches.items()},
        warp_property_fields={field: bool(entries) for field, entries in warp_property_matches.items()},
        cas_fields=cas_found,
        warp_fields=warp_found,
        config_execution_class=execution_class,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
