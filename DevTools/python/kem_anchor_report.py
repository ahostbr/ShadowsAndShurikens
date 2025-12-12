"""
Usage:
    Run this from the DevTools hub to get a quick snapshot of the KEM anchor class
    and the folders that likely contain map-level instances. This is a static source
    report only; actual anchor counts still require the editor.
"""
from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Iterable

from cli_utils import confirm_exit, confirm_start
from llm_log import print_llm_summary
from project_paths import get_project_root

CONFIG_PATH = Path(__file__).resolve().parent / "sots_tools_config.json"
ANCHOR_FILE_PATTERN = "*KEMExecutionAnchor*.h"
REQUIRED_FIELDS = ["PositionTag", "ExecutionFamily", "PreferredExecutions", "UseRadius"]
MAP_DISPLAY_LIMIT = 20
HELPER_FUNCTION_PATTERN = re.compile(r"\b([A-Za-z_][A-Za-z_0-9]*(?:PositionTag|ExecutionFamily)[A-Za-z_0-9]*)\s*\(")


def load_config() -> dict:
    if not CONFIG_PATH.exists():
        return {}

    try:
        with CONFIG_PATH.open("r", encoding="utf-8") as config_file:
            return json.load(config_file)
    except Exception:
        return {}


def get_anchor_config(config: dict) -> dict:
    defaults = {
        "anchor_class_name": "ASOTS_KEMExecutionAnchor",
        "map_root": "Content/Maps",
        "log_note": "",
    }
    return {**defaults, **config.get("kem_anchors", {})}


def find_anchor_header(search_roots: Iterable[Path], class_name: str) -> Path | None:
    for root in search_roots:
        if not root.exists():
            continue
        for candidate in root.rglob(ANCHOR_FILE_PATTERN):
            try:
                text = candidate.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue
            if f"class {class_name}" in text:
                return candidate
    return None


def get_field_presence(header_path: Path) -> dict[str, bool]:
    try:
        text = header_path.read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return {field: False for field in REQUIRED_FIELDS}

    return {field: field in text for field in REQUIRED_FIELDS}


def extract_position_tag_lines(header_path: Path) -> list[str]:
    try:
        lines = header_path.read_text(encoding="utf-8", errors="ignore").splitlines()
    except Exception:
        return []

    matches: list[str] = []
    for line in lines:
        if "PositionTag" not in line:
            continue
        cleaned = line.strip()
        if not cleaned:
            continue
        if cleaned.startswith("//"):
            cleaned = cleaned.lstrip("/").strip()
        if cleaned not in matches:
            matches.append(cleaned)
        if len(matches) >= 8:
            break
    return matches


def extract_helper_functions(header_path: Path) -> list[str]:
    if not header_path.exists():
        return []

    try:
        text = header_path.read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return []

    seen: dict[str, None] = {}
    for match in HELPER_FUNCTION_PATTERN.finditer(text):
        name = match.group(1)
        if name not in seen:
            seen[name] = None
    return list(seen.keys())[:10]


def list_maps(project_root: Path, map_root: str, limit: int = MAP_DISPLAY_LIMIT) -> list[str]:
    maps_dir = project_root / map_root
    if not maps_dir.exists():
        return []

    collected: list[str] = []
    for path in sorted(maps_dir.rglob("*.umap")):
        if len(collected) >= limit:
            break
        try:
            relative = path.relative_to(project_root)
        except ValueError:
            relative = path
        relative_str = str(relative).replace("\\", "/")
        if relative_str.startswith("Content/"):
            relative_str = relative_str[len("Content/"):]
        if relative_str.endswith(".umap"):
            relative_str = relative_str[: -len(".umap")]
        relative_str = relative_str.lstrip("/")
        collected.append(f"/Game/{relative_str}")
    return collected


def describe_path(path: Path | None, root: Path) -> str:
    if not path:
        return "not found"

    try:
        return str(path.relative_to(root))
    except ValueError:
        return str(path)


def print_report(
    anchor_path: Path | None,
    class_name: str,
    fields_status: dict[str, bool],
    position_lines: list[str],
    helpers: list[str],
    maps: list[str],
    map_root: str,
    log_note: str,
    project_root: Path,
) -> None:
    print("\n=== KEM Anchor Report ===")
    if anchor_path:
        try:
            display_path = anchor_path.relative_to(project_root)
        except ValueError:
            display_path = anchor_path
        print(f"Anchor class {class_name} located at: {display_path}")
        print("Fields:")
        for field, present in fields_status.items():
            status = "OK" if present else "MISSING"
            print(f"  - {field}: {status}")
    else:
        print(f"Unable to find the anchor class {class_name} in Source/Plugins.")

    print(f"Maps scanned under '{map_root}':")
    if maps:
        for map_path in maps:
            print(f"  - {map_path}")
    else:
        print("  (no .umap files found or map folder missing)")

    if position_lines:
        print("\nPositionTag hints from anchor header:")
        for line in position_lines:
            print(f"  - {line}")

    if helpers:
        print("\nHelper functions mentioning PositionTag/ExecutionFamily:")
        for helper in helpers:
            print(f"  - {helper}")

    if log_note:
        print("\nLog Note:")
        print(f"  {log_note}")

    print("\nNOTE: Actual anchor counts still require the editor. Logs can help once the anchor manager emits events.")


def main() -> None:
    confirm_start("kem_anchor_report")

    project_root = get_project_root()
    config = load_config()
    anchor_cfg = get_anchor_config(config)
    class_name = anchor_cfg["anchor_class_name"]
    map_root = anchor_cfg["map_root"]
    log_note = anchor_cfg.get("log_note", "")

    search_dirs = [project_root / "Source", project_root / "Plugins"]
    anchor_path = find_anchor_header(search_dirs, class_name)
    fields_status: dict[str, bool] = {}
    position_lines: list[str] = []
    helper_functions: list[str] = []

    if anchor_path:
        fields_status = get_field_presence(anchor_path)
        position_lines = extract_position_tag_lines(anchor_path)
        helper_functions = extract_helper_functions(anchor_path)
    else:
        fields_status = {field: False for field in REQUIRED_FIELDS}

    map_entries = list_maps(project_root, map_root)

    print_report(
        anchor_path,
        class_name,
        fields_status,
        position_lines,
        helper_functions,
        map_entries,
        map_root,
        log_note,
        project_root,
    )

    print_llm_summary(
        "kem_anchor_report",
        status="OK" if anchor_path else "WARN",
        anchor_header=describe_path(anchor_path, project_root),
        map_root=map_root,
        maps_found=len(map_entries),
        fields=fields_status,
        position_tags=position_lines,
        helper_functions=helper_functions,
        log_note=log_note,
    )

    confirm_exit()


if __name__ == "__main__":
    main()