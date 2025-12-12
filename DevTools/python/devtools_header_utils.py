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
}

PATH_LIKE_KEYS = {
    "path",
    "file",
    "target",
    "target_file",
    "script_path",
    "config_path",
    "project_root",
    "scope",
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
    for line in block:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        data[key.strip().lower()] = value.strip()
    return data


def load_header_from_file(path: str) -> Tuple[Optional[Dict[str, str]], Optional[str]]:
    if not os.path.isfile(path):
        return None, f"file does not exist or is not a file: {path}"
    try:
        with open(path, "r", encoding="utf-8") as f:
            text = f.read()
    except Exception as exc:
        return None, f"failed to read file: {exc}"
    header = parse_header_block(text)
    if header is None:
        return None, "no [SOTS_DEVTOOLS] header block found"
    return header, None


def analyze_header_core(header: Dict[str, str]) -> Tuple[int, int, List[str]]:
    msgs: List[str] = []
    fatals = 0
    warns = 0

    tool = header.get("tool", "").strip()
    if not tool:
        msgs.append("FATAL: header missing required 'tool:' field")
        fatals += 1
    else:
        msgs.append(f"OK: tool='{tool}'")
        if tool not in KNOWN_TOOLS:
            msgs.append(
                "WARNING: tool not in KNOWN_TOOLS; may be fine but check spelling/config."
            )
            warns += 1

    mode = header.get("mode", "").strip().lower()
    if not mode:
        msgs.append("WARNING: 'mode' not set; treating as 'manual' by convention.")
        warns += 1
    elif mode not in ("manual", "auto"):
        msgs.append(f"WARNING: unexpected mode '{mode}', expected 'manual' or 'auto'.")
        warns += 1

    for key in ("name", "plugin", "category"):
        if not header.get(key, "").strip():
            msgs.append(f"WARNING: recommended header field '{key}:' missing or empty.")
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
            candidate = os.path.join(project_root, candidate)
        candidate = os.path.normpath(candidate)
        if os.path.exists(candidate):
            msgs.append(f"OK: {key} -> {candidate} exists")
        else:
            msgs.append(f"WARNING: {key} -> {candidate} does not exist")
            warns += 1
    return fatals, warns, msgs


def analyze_header_tool_specific(header: Dict[str, str]) -> Tuple[int, int, List[str]]:
    msgs: List[str] = []
    fatals = 0
    warns = 0
    tool = header.get("tool", "").strip()
    if not tool:
        return fatals, warns, msgs
    required = TOOL_REQUIRED_FIELDS.get(tool)
    if not required:
        return fatals, warns, msgs
    for field in required:
        if not header.get(field):
            msgs.append(
                f"WARNING: tool '{tool}' missing recommended field '{field}:'"
            )
            warns += 1
    return fatals, warns, msgs
