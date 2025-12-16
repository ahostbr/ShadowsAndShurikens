# Shared helpers for [SOTS_DEVTOOLS] headers
import os
from typing import Dict, Tuple, Optional, List

HEADER_START = "[SOTS_DEVTOOLS]"
HEADER_END = "[/SOTS_DEVTOOLS]"

TEXT_FILE_EXTS = (".txt", ".md", ".log", ".cfg", ".json")

KNOWN_TOOLS = {
    "quick_search",
    "regex_search",
    "run_python_script",
    "apply_json_pack",
    "version_cleanup",
    "mass_regex_edit",
    "pack_lint",
    "devtools_status_update",
    "export_report_bundle",
    "report_bundle_export",
    "pipeline_hub",
    "inbox_route",
    "devtools_status_dashboard",
    "log_error_digest",
    "pack_template_generator",
    "save_context_anchor",
}

PATH_LIKE_KEYS = {
    "path",
    "file",
    "folder",
    "project_root",
    "root",
    "source",
    "dest",
    "output",
}

TOOL_REQUIRED_FIELDS = {
    "quick_search": ["search", "exts"],
    "mass_regex_edit": ["search", "replace", "exts"],
    "devtools_status_update": ["plugin", "step", "status"],
    "export_report_bundle": ["category"],
    "report_bundle_export": ["category"],
}


def is_text_file_name(name: str) -> bool:
    return name.lower().endswith(TEXT_FILE_EXTS)


def parse_header_block(text: str) -> Optional[Dict[str, str]]:
    start_idx = text.find(HEADER_START)
    if start_idx == -1:
        return None
    end_idx = text.find(HEADER_END, start_idx)
    if end_idx == -1:
        return None
    block = text[start_idx:end_idx].splitlines()[1:]  # skip first line
    data: Dict[str, str] = {}
    for raw in block:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if ":" in line:
            k, v = line.split(":", 1)
        elif "=" in line:
            k, v = line.split("=", 1)
        else:
            continue
        data[k.strip().lower()] = v.strip()
    return data


def validate_header_required_fields(header: Dict[str, str]) -> Tuple[int, int, List[str]]:
    msgs: List[str] = []
    warns = 0
    fatals = 0

    tool = header.get("tool", "").strip().lower()
    if not tool:
        msgs.append("FATAL: Missing tool: in [SOTS_DEVTOOLS] header.")
        return 1, 0, msgs

    if tool not in KNOWN_TOOLS:
        msgs.append(f"WARNING: Unknown tool '{tool}'. Add to KNOWN_TOOLS if intended.")
        warns += 1

    required = TOOL_REQUIRED_FIELDS.get(tool)
    if not required:
        return fatals, warns, msgs

    for field in required:
        if not header.get(field):
            msgs.append(f"WARNING: tool '{tool}' missing recommended field '{field}:'")
            warns += 1

    return fatals, warns, msgs


def analyze_header_paths(header: Dict[str, str], project_root: str) -> Tuple[int, int, List[str]]:
    msgs: List[str] = []
    warns = 0
    fatals = 0
    for key, raw in header.items():
        if key not in PATH_LIKE_KEYS:
            continue
        value = raw.strip()
        if not value:
            continue
        candidate = value
        if not os.path.isabs(candidate):
            candidate = os.path.normpath(os.path.join(project_root, candidate))
        if not os.path.exists(candidate):
            msgs.append(f"WARNING: header path key '{key}' points to missing path: {candidate}")
            warns += 1
    return fatals, warns, msgs
