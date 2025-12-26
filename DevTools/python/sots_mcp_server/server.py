"""
SOTS MCP Server (read-only by default)
- Exposes safe DevTools entrypoints + scoped search + report reading
- Adds guarded delegates to the SOTS Agents orchestrator and BPGen bridge
- Designed for VS Code MCP (stdio). Do NOT print to stdout (breaks JSON-RPC).
- Buddy edits files; this server only reads / runs allowlisted scripts unless apply is explicitly enabled.
"""

from __future__ import annotations

import base64
import datetime as dt
import io
import json
import mimetypes
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple
import logging
import uuid
import time
from collections import deque

from mcp.server.fastmcp import FastMCP  # official MCP Python SDK
try:
    from mcp.types import ImageContent, TextContent
except Exception:  # pragma: no cover - optional import guard
    ImageContent = None
    TextContent = None

# Ensure DevTools/python is importable before pulling sibling modules
DEVTOOLS_PY_ROOT = Path(__file__).resolve().parent.parent
if str(DEVTOOLS_PY_ROOT) not in sys.path:
    sys.path.insert(0, str(DEVTOOLS_PY_ROOT))

from sots_agents_runner import RunConfig as AgentsRunConfig
from sots_agents_runner import SOTSAgentsOrchestrator, setup_logging as setup_agents_logging
from sots_agents_runner import sots_agents_run_task
from sots_bpgen_bridge_client import (
    DEFAULT_CONNECT_TIMEOUT as BPGEN_DEFAULT_CONNECT_TIMEOUT,
    DEFAULT_HOST as BPGEN_DEFAULT_HOST,
    DEFAULT_MAX_BYTES as BPGEN_DEFAULT_MAX_BYTES,
    DEFAULT_PORT as BPGEN_DEFAULT_PORT,
    DEFAULT_RECV_TIMEOUT as BPGEN_DEFAULT_RECV_TIMEOUT,
    bpgen_call,
)

# -----------------------------------------------------------------------------
# IMPORTANT for stdio MCP:
# Never write to stdout (print). Use stderr for logs.
# -----------------------------------------------------------------------------

def _elog(msg: str) -> None:
    sys.stderr.write(msg.rstrip() + "\n")
    sys.stderr.flush()


def _env_bool(name: str, default: bool = False) -> bool:
    val = os.environ.get(name)
    if val is None:
        return default
    return val.strip().lower() in {"1", "true", "yes", "on"}


def _sots_ok(data: Any = None, warnings: Optional[List[str]] = None, meta: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    return {"ok": True, "data": data, "error": None, "warnings": warnings or [], "meta": meta or {}}


def _sots_err(error: str, data: Any = None, warnings: Optional[List[str]] = None, meta: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    return {"ok": False, "data": data, "error": error, "warnings": warnings or [], "meta": meta or {}}


def _not_implemented(tool: str, action: str, hint: str, req_id: str) -> Dict[str, Any]:
    return _sots_err(
        f"{tool}.{action}: not implemented in BPGen parity layer yet",
        data={"tool": tool, "action": action, "hint": hint},
        meta={"request_id": req_id},
    )


@dataclass(frozen=True)
class SotsPaths:
    project_root: Path
    devtools_py: Path
    reports_dir: Path

def _resolve_paths() -> SotsPaths:
    # Prefer explicit env var; otherwise walk up from this file until DevTools/python exists.
    env_root = os.environ.get("SOTS_PROJECT_ROOT", "").strip()
    if env_root:
        project_root = Path(env_root).expanduser().resolve()
    else:
        here = Path(__file__).resolve()
        # Expect: <ProjectRoot>/DevTools/python/sots_mcp_server/server.py
        project_root = here
        for _ in range(8):
            candidate = project_root / "DevTools" / "python"
            if candidate.exists():
                break
            project_root = project_root.parent
        project_root = project_root.resolve()

    devtools_py = (project_root / "DevTools" / "python").resolve()
    reports_dir = (project_root / "DevTools" / "reports").resolve()
    return SotsPaths(project_root=project_root, devtools_py=devtools_py, reports_dir=reports_dir)

PATHS = _resolve_paths()
ALLOW_APPLY = _env_bool("SOTS_ALLOW_APPLY", False)
EXPORTS_ROOT = (PATHS.project_root / "DevTools" / "prompts" / "BPGen").resolve()
ALLOW_BPGEN_APPLY = _env_bool("SOTS_ALLOW_BPGEN_APPLY", ALLOW_APPLY)
ALLOW_DEVTOOLS_RUN = _env_bool("SOTS_ALLOW_DEVTOOLS_RUN", True)
REPORTS_DIR = (PATHS.devtools_py / "reports").resolve()

ROOT_MAP = {
    "repo": PATHS.project_root,
    "exports": EXPORTS_ROOT,
}

TOOL_ALIASES = {
    "sots_search_workspace": "sots_search (root=Repo)",
    "sots_search_exports": "sots_search (root=Exports)",
    "manage_bpgen": "bpgen_call",
    "agents_run_prompt": "sots_agents_run",
    "agents_server_info": "sots_agents_help",
    "bpgen_call": "manage_bpgen",
    "bpgen_ping": "bpgen_ping",
}


# -----------------------------------------------------------------------------
# MCP server definition (FastMCP)
#
# IMPORTANT:
# `mcp` MUST be defined before any `@mcp.tool(...)` decorators run at import time.
# -----------------------------------------------------------------------------

mcp = FastMCP(
    "SOTS DevTools (unified)",
    # json_response=True helps some clients interpret dict outputs consistently.
    json_response=True,
)


# -----------------------------------------------------------------------------
# VibeUE upstream compatibility layer (initial scaffold)
#
# Upstream VibeUE exposes 13 multi-action tools. This unified server adds tools
# with matching names to ease prompt-pack reuse. Only a BPGen-expressible subset
# is implemented initially.
# -----------------------------------------------------------------------------


def _vibeue_action_str(value: str) -> str:
    return (value or "").strip().lower()


def _vibeue_build_asset_path(blueprint_name: str, destination_path: str, name: str) -> str:
    bp = (blueprint_name or "").strip()
    if bp:
        return bp
    dst = (destination_path or "/Game").strip() or "/Game"
    nm = (name or "").strip()
    if not nm:
        raise ValueError("Missing blueprint name (provide blueprint_name or name)")
    if dst.endswith("/"):
        dst = dst[:-1]
    return f"{dst}/{nm}"


def _vibeue_new_guid_str() -> str:
    # VibeUE upstream treats NodeId as a GUID string.
    return str(uuid.uuid4())


def _bpgen_vec2(pos: Optional[Sequence[int]]) -> Dict[str, int]:
    if not pos or len(pos) != 2:
        return {"X": 0, "Y": 0}
    try:
        return {"X": int(pos[0]), "Y": int(pos[1])}
    except Exception:
        return {"X": 0, "Y": 0}


def _bpgen_literal(value: Any) -> str:
    if value is None:
        return ""
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (int, float)):
        return str(value)
    if isinstance(value, str):
        return value
    # For lists/dicts/other objects, roundtrip as JSON.
    try:
        return json.dumps(value, ensure_ascii=False)
    except Exception:
        return str(value)


def _bpgen_target_for_scope(scope: str, function_name: str) -> Dict[str, Any]:
    s = _vibeue_action_str(scope) or "event"
    if s == "function":
        return {"target_type": "Function", "name": (function_name or "").strip()}
    # VibeUE "event" scope means the Blueprint's EventGraph.
    if s == "event":
        return {"target_type": "EventGraph", "name": ""}
    # Unknown scope: best effort to avoid mutations in the wrong graph.
    raise ValueError(f"Unsupported graph_scope '{scope}'")


def _umg_class_path(component_type: str) -> str:
    ct = (component_type or "").strip()
    if not ct:
        return ""
    if "/" in ct:
        return ct
    # Common UMG widget class paths are /Script/UMG.<ClassName>
    return f"/Script/UMG.{ct}"


def _umg_import_text(value: Any) -> str:
    """Best-effort conversion to UE ImportText strings.

    BPGen UMG property setters use FProperty::ImportText_Direct, so the value must be
    in UE's ImportText format. This helper covers a few common cases and otherwise
    accepts caller-provided strings.
    """
    if value is None:
        return ""
    if isinstance(value, str):
        return value
    if isinstance(value, bool):
        return "True" if value else "False"
    if isinstance(value, (int, float)):
        return str(value)

    if isinstance(value, (list, tuple)):
        nums: List[float] = []
        for v in value:
            if isinstance(v, (int, float)):
                nums.append(float(v))
            else:
                nums = []
                break
        if len(nums) == 2:
            return f"(X={nums[0]},Y={nums[1]})"
        if len(nums) == 3:
            return f"(X={nums[0]},Y={nums[1]},Z={nums[2]})"
        if len(nums) == 4:
            return f"(R={nums[0]},G={nums[1]},B={nums[2]},A={nums[3]})"

    if isinstance(value, dict):
        # Common struct shapes
        keys = {str(k) for k in value.keys()}
        if keys.issuperset({"Left", "Top", "Right", "Bottom"}):
            return f"(Left={value.get('Left')},Top={value.get('Top')},Right={value.get('Right')},Bottom={value.get('Bottom')})"

        # Generic struct-ish formatting
        parts: List[str] = []
        for k, v in value.items():
            parts.append(f"{k}={_umg_import_text(v)}")
        return f"({','.join(parts)})"

    return str(value)


def _ue_import_text(value: Any) -> str:
    # Actor-component property setters also use UE ImportText parsing.
    return _umg_import_text(value)


def _umg_split_props(props: Optional[Dict[str, Any]]) -> Tuple[Dict[str, str], Dict[str, str]]:
    """Split properties into widget props and slot props.

    ensure_widget takes slot_properties without the "Slot." prefix.
    """
    widget_props: Dict[str, str] = {}
    slot_props: Dict[str, str] = {}
    if not props:
        return widget_props, slot_props
    for k, v in props.items():
        key = (k or "").strip()
        if not key:
            continue
        val = _umg_import_text(v)
        if key.lower().startswith("slot."):
            slot_props[key[5:]] = val
        else:
            widget_props[key] = val
    return widget_props, slot_props


@mcp.tool(
    name="manage_blueprint",
    description="VibeUE-compatible Blueprint lifecycle tool (subset implemented via BPGen).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "VibeUE Compat: manage_blueprint"},
)
def manage_blueprint(
    action: str,
    blueprint_name: str = "",
    name: str = "",
    destination_path: str = "/Game",
    parent_class_path: str = "",
    property_name: str = "",
    property_value: Any = None,
    new_parent_class: str = "",
    include_class_defaults: bool = True,
    max_nodes: int = 200,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    try:
        if act == "create":
            asset_path = _vibeue_build_asset_path(blueprint_name, destination_path, name)
            res = bpgen_create_blueprint(asset_path=asset_path, parent_class_path=parent_class_path or None)
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out
        if act == "compile":
            if not blueprint_name:
                return _sots_err("manage_blueprint.compile requires blueprint_name", meta={"request_id": req_id})
            res = bpgen_compile(blueprint_asset_path=blueprint_name)
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act == "get_info":
            if not blueprint_name:
                return _sots_err("manage_blueprint.get_info requires blueprint_name", meta={"request_id": req_id})
            res = _bpgen_call(
                "bp_blueprint_get_info",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "include_class_defaults": bool(include_class_defaults),
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act == "get_property":
            if not blueprint_name:
                return _sots_err("manage_blueprint.get_property requires blueprint_name", meta={"request_id": req_id})
            if not property_name:
                return _sots_err("manage_blueprint.get_property requires property_name", meta={"request_id": req_id})
            res = _bpgen_call(
                "bp_blueprint_get_property",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "property_name": (property_name or "").strip(),
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act == "set_property":
            if not blueprint_name:
                return _sots_err("manage_blueprint.set_property requires blueprint_name", meta={"request_id": req_id})
            if not property_name:
                return _sots_err("manage_blueprint.set_property requires property_name", meta={"request_id": req_id})

            action_name = "bp_blueprint_set_property"
            guard = _bpgen_guard_mutation(action_name)
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                action_name,
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "property_name": (property_name or "").strip(),
                    "property_value": _ue_import_text(property_value),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act == "reparent":
            if not blueprint_name:
                return _sots_err("manage_blueprint.reparent requires blueprint_name", meta={"request_id": req_id})
            if not new_parent_class:
                return _sots_err("manage_blueprint.reparent requires new_parent_class", meta={"request_id": req_id})

            action_name = "bp_blueprint_reparent"
            guard = _bpgen_guard_mutation(action_name)
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                action_name,
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "new_parent_class": (new_parent_class or "").strip(),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act == "list_custom_events":
            if not blueprint_name:
                return _sots_err("manage_blueprint.list_custom_events requires blueprint_name", meta={"request_id": req_id})
            res = _bpgen_call(
                "bp_blueprint_list_custom_events",
                {"blueprint_name": (blueprint_name or "").strip()},
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act == "summarize_event_graph":
            if not blueprint_name:
                return _sots_err("manage_blueprint.summarize_event_graph requires blueprint_name", meta={"request_id": req_id})
            res = _bpgen_call(
                "bp_blueprint_summarize_event_graph",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "max_nodes": int(max_nodes),
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        return _not_implemented(
            "manage_blueprint",
            act or "(empty)",
            "Implemented: create (via bpgen_create_blueprint), compile (via bpgen_compile), get_info/get_property/set_property/reparent/list_custom_events/summarize_event_graph (via bp_blueprint_* bridge ops).",
            req_id,
        )
    except Exception as exc:
        out = _sots_err(f"manage_blueprint failed: {exc}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "manage_blueprint", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out


@mcp.tool(
    name="manage_blueprint_function",
    description="VibeUE-compatible Blueprint function tool (subset implemented via BPGen ensure_function).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "VibeUE Compat: manage_blueprint_function"},
)
def manage_blueprint_function(
    action: str,
    blueprint_name: str = "",
    function_name: str = "",
    param_name: str = "",
    direction: str = "",
    type: str = "",
    new_type: str = "",
    new_name: str = "",
    properties: Any = None,
    extra: Any = None,
    signature: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    try:
        if act == "create":
            if not blueprint_name or not function_name:
                return _sots_err("manage_blueprint_function.create requires blueprint_name and function_name", meta={"request_id": req_id})
            res = bpgen_ensure_function(
                blueprint_asset_path=blueprint_name,
                function_name=function_name,
                signature=signature or {},
                create_if_missing=True,
                update_if_exists=True,
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"list"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.list requires blueprint_name", meta={"request_id": req_id})
            res = _bpgen_call("bp_function_list", {"blueprint_name": (blueprint_name or "").strip()})
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"get"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.get requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.get requires function_name", meta={"request_id": req_id})
            res = _bpgen_call(
                "bp_function_get",
                {"blueprint_name": (blueprint_name or "").strip(), "function_name": (function_name or "").strip()},
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"delete"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.delete requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.delete requires function_name", meta={"request_id": req_id})
            guard = _bpgen_guard_mutation("bp_function_delete")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard
            res = _bpgen_call(
                "bp_function_delete",
                {"blueprint_name": (blueprint_name or "").strip(), "function_name": (function_name or "").strip(), "dangerous_ok": True},
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"list_params"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.list_params requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.list_params requires function_name", meta={"request_id": req_id})
            res = _bpgen_call(
                "bp_function_list_params",
                {"blueprint_name": (blueprint_name or "").strip(), "function_name": (function_name or "").strip()},
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"add_param"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.add_param requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.add_param requires function_name", meta={"request_id": req_id})
            if not param_name:
                return _sots_err("manage_blueprint_function.add_param requires param_name", meta={"request_id": req_id})
            if not direction:
                return _sots_err("manage_blueprint_function.add_param requires direction ('input' or 'out')", meta={"request_id": req_id})
            if not type:
                return _sots_err("manage_blueprint_function.add_param requires type", meta={"request_id": req_id})
            guard = _bpgen_guard_mutation("bp_function_add_param")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard
            res = _bpgen_call(
                "bp_function_add_param",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "function_name": (function_name or "").strip(),
                    "param_name": (param_name or "").strip(),
                    "direction": (direction or "").strip(),
                    "type": (type or "").strip(),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"remove_param"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.remove_param requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.remove_param requires function_name", meta={"request_id": req_id})
            if not param_name:
                return _sots_err("manage_blueprint_function.remove_param requires param_name", meta={"request_id": req_id})
            if not direction:
                return _sots_err("manage_blueprint_function.remove_param requires direction ('input' or 'out')", meta={"request_id": req_id})
            guard = _bpgen_guard_mutation("bp_function_remove_param")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard
            res = _bpgen_call(
                "bp_function_remove_param",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "function_name": (function_name or "").strip(),
                    "param_name": (param_name or "").strip(),
                    "direction": (direction or "").strip(),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"update_param"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.update_param requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.update_param requires function_name", meta={"request_id": req_id})
            if not param_name:
                return _sots_err("manage_blueprint_function.update_param requires param_name", meta={"request_id": req_id})
            if not direction:
                return _sots_err("manage_blueprint_function.update_param requires direction ('input' or 'out')", meta={"request_id": req_id})
            if not new_type and not new_name:
                return _sots_err("manage_blueprint_function.update_param requires new_type and/or new_name", meta={"request_id": req_id})
            guard = _bpgen_guard_mutation("bp_function_update_param")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard
            res = _bpgen_call(
                "bp_function_update_param",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "function_name": (function_name or "").strip(),
                    "param_name": (param_name or "").strip(),
                    "direction": (direction or "").strip(),
                    "new_type": (new_type or "").strip(),
                    "new_name": (new_name or "").strip(),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"list_locals", "list_local_vars", "locals"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.list_locals requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.list_locals requires function_name", meta={"request_id": req_id})
            res = _bpgen_call(
                "bp_function_list_locals",
                {"blueprint_name": (blueprint_name or "").strip(), "function_name": (function_name or "").strip()},
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"add_local"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.add_local requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.add_local requires function_name", meta={"request_id": req_id})
            if not param_name:
                return _sots_err("manage_blueprint_function.add_local requires param_name (local name)", meta={"request_id": req_id})
            if not type:
                return _sots_err("manage_blueprint_function.add_local requires type", meta={"request_id": req_id})
            guard = _bpgen_guard_mutation("bp_function_add_local")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard
            res = _bpgen_call(
                "bp_function_add_local",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "function_name": (function_name or "").strip(),
                    "param_name": (param_name or "").strip(),
                    "type": (type or "").strip(),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"remove_local"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.remove_local requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.remove_local requires function_name", meta={"request_id": req_id})
            if not param_name:
                return _sots_err("manage_blueprint_function.remove_local requires param_name (local name)", meta={"request_id": req_id})
            guard = _bpgen_guard_mutation("bp_function_remove_local")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard
            res = _bpgen_call(
                "bp_function_remove_local",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "function_name": (function_name or "").strip(),
                    "param_name": (param_name or "").strip(),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"update_local"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.update_local requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.update_local requires function_name", meta={"request_id": req_id})
            if not param_name:
                return _sots_err("manage_blueprint_function.update_local requires param_name (local name)", meta={"request_id": req_id})
            if not new_type:
                return _sots_err("manage_blueprint_function.update_local requires new_type", meta={"request_id": req_id})
            guard = _bpgen_guard_mutation("bp_function_update_local")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard
            res = _bpgen_call(
                "bp_function_update_local",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "function_name": (function_name or "").strip(),
                    "param_name": (param_name or "").strip(),
                    "new_type": (new_type or "").strip(),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"update_properties"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_function.update_properties requires blueprint_name", meta={"request_id": req_id})
            if not function_name:
                return _sots_err("manage_blueprint_function.update_properties requires function_name", meta={"request_id": req_id})

            props_obj: Optional[Dict[str, Any]] = None
            if isinstance(properties, dict):
                props_obj = properties
            elif isinstance(properties, str) and properties.strip():
                try:
                    parsed = json.loads(properties)
                    if isinstance(parsed, dict):
                        props_obj = parsed
                except Exception:
                    props_obj = None

            if not props_obj:
                return _sots_err(
                    "manage_blueprint_function.update_properties requires properties (dict or JSON dict string)",
                    meta={"request_id": req_id},
                )

            guard = _bpgen_guard_mutation("bp_function_update_properties")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                "bp_function_update_properties",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "function_name": (function_name or "").strip(),
                    "properties": props_obj,
                    "extra": extra,
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        return _not_implemented(
            "manage_blueprint_function",
            act or "(empty)",
            "Implemented: create (mapped to bpgen_ensure_function) plus list/get/delete/list_params/add_param/remove_param/update_param/list_locals/add_local/remove_local/update_local/update_properties (via bp_function_* bridge ops).",
            req_id,
        )
    except Exception as exc:
        out = _sots_err(f"manage_blueprint_function failed: {exc}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_function", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out


@mcp.tool(
    name="manage_blueprint_variable",
    description="VibeUE-compatible Blueprint variable tool (7 actions implemented via BPGen + bridge variable ops).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "VibeUE Compat: manage_blueprint_variable"},
)
def manage_blueprint_variable(
    action: str,
    blueprint_name: str = "",
    variable_name: str = "",
    # Back-compat (SOTS legacy): bpgen_ensure_variable pathway
    pin_type: Optional[Dict[str, Any]] = None,
    default_value: Optional[Any] = None,
    instance_editable: bool = True,
    expose_on_spawn: bool = False,
    # VibeUE contract
    variable_config: Optional[Dict[str, Any]] = None,
    property_path: Optional[str] = None,
    value: Optional[Any] = None,
    delete_options: Optional[Dict[str, Any]] = None,
    list_criteria: Optional[Dict[str, Any]] = None,
    info_options: Optional[Dict[str, Any]] = None,
    search_criteria: Optional[Dict[str, Any]] = None,
    options: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    try:
        if act == "create":
            if not blueprint_name or not variable_name:
                return _sots_err("manage_blueprint_variable.create requires blueprint_name and variable_name", meta={"request_id": req_id})
            if not isinstance(pin_type, dict) or not pin_type:
                return _sots_err(
                    "manage_blueprint_variable.create requires pin_type (BPGen pin_type dict)",
                    data={"example": {"pin_type": {"PinCategory": "real"}}},
                    meta={"request_id": req_id},
                )
            res = bpgen_ensure_variable(
                blueprint_asset_path=blueprint_name,
                variable_name=variable_name,
                pin_type=pin_type,
                default_value=default_value,
                instance_editable=instance_editable,
                expose_on_spawn=expose_on_spawn,
                create_if_missing=True,
                update_if_exists=True,
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out
        return _not_implemented(
            "manage_blueprint_variable",
            act or "(empty)",
            "Implemented: create (mapped to bpgen_ensure_variable). List/get/set need BPGen extensions or spec-driven reads.",
            req_id,
        )
    except Exception as exc:
        out = _sots_err(f"manage_blueprint_variable failed: {exc}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out


@mcp.tool(
    name="manage_blueprint_node",
    description="VibeUE-compatible Blueprint node tool (subset implemented via BPGen discover/list/describe + edit wrappers).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "VibeUE Compat: manage_blueprint_node"},
)
def manage_blueprint_node(
    action: str,
    blueprint_name: str = "",
    graph_scope: str = "event",
    function_name: str = "",
    node_id: str = "",
    node_identifier: str = "",
    node_type: Optional[str] = None,
    node_params: Optional[Dict[str, Any]] = None,
    node_config: Optional[Dict[str, Any]] = None,
    position: Optional[List[int]] = None,
    node_position: Optional[List[int]] = None,
    source_node_id: Optional[str] = None,
    source_pin: Optional[str] = None,
    target_node_id: Optional[str] = None,
    target_pin: Optional[str] = None,
    property_name: Optional[str] = None,
    property_value: Any = None,
    search_term: str = "",
    max_results: int = 200,
    include_pins: bool = False,
    extra: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    scope = _vibeue_action_str(graph_scope) or "event"
    node_params = node_params or {}
    node_config = node_config or {}
    extra = extra or {}
    try:
        is_function_scope = scope == "function"
        is_event_scope = scope == "event"

        if act == "discover":
            if not blueprint_name:
                return _sots_err("manage_blueprint_node.discover requires blueprint_name", meta={"request_id": req_id})
            res = bpgen_discover(
                blueprint_asset_path=blueprint_name,
                search_text=search_term,
                max_results=max_results,
                include_pins=include_pins,
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out
        if act == "list":
            if not blueprint_name or not function_name:
                return _sots_err("manage_blueprint_node.list requires blueprint_name and function_name", meta={"request_id": req_id})
            res = bpgen_list(blueprint_asset_path=blueprint_name, function_name=function_name, include_pins=include_pins)
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out
        if act == "describe":
            if not blueprint_name or not function_name or not node_id:
                return _sots_err("manage_blueprint_node.describe requires blueprint_name, function_name, node_id", meta={"request_id": req_id})
            res = bpgen_describe(blueprint_asset_path=blueprint_name, function_name=function_name, node_id=node_id, include_pins=True)
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out
        if act == "delete":
            if not blueprint_name or not function_name or not node_id:
                return _sots_err("manage_blueprint_node.delete requires blueprint_name, function_name, node_id", meta={"request_id": req_id})
            res = bpgen_delete_node(blueprint_asset_path=blueprint_name, function_name=function_name, node_id=node_id)
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act == "create":
            if not blueprint_name:
                return _sots_err("manage_blueprint_node.create requires blueprint_name", meta={"request_id": req_id})
            if is_function_scope and not function_name:
                return _sots_err("manage_blueprint_node.create requires function_name when graph_scope='function'", meta={"request_id": req_id})
            if not (is_function_scope or is_event_scope):
                return _not_implemented(
                    "manage_blueprint_node",
                    f"{act}:{scope}",
                    "Only graph_scope='function' and graph_scope='event' are supported by the BPGen parity layer today.",
                    req_id,
                )

            # Prefer caller-provided node_id; otherwise generate a GUID-like id.
            node_id_out = (node_id or node_identifier or "").strip() or _vibeue_new_guid_str()

            # BPGen expects GraphSpec node fields; we primarily support spawner-based nodes.
            spawner_key = (node_params.get("spawner_key") or node_params.get("SpawnerKey") or "").strip()
            function_path = (node_params.get("function_path") or node_params.get("FunctionPath") or "").strip()

            # Allow node_type itself to be a spawner key for convenience.
            if not spawner_key and isinstance(node_type, str):
                nt = node_type.strip()
                if nt.startswith("/Script/") or ":" in nt:
                    spawner_key = nt
            if not function_path and spawner_key and (spawner_key.startswith("/Script/") or ":" in spawner_key):
                function_path = spawner_key

            # Synthetic nodes allowed without spawner keys.
            synthetic_allowed = {
                "K2NODE_FUNCTIONENTRY",
                "K2NODE_FUNCTIONRESULT",
                "K2NODE_KNOT",
                "K2NODE_SELECT",
                "K2NODE_DYNAMICCAST",
            }
            node_type_str = (node_type or "").strip()
            node_type_norm = node_type_str.upper()

            if not spawner_key and node_type_norm not in synthetic_allowed:
                return _sots_err(
                    "manage_blueprint_node.create requires node_params.spawner_key (preferred) or a supported synthetic NodeType",
                    data={
                        "hint": "Use discover to obtain spawner_key, then create with node_params={spawner_key: ...}",
                        "synthetic_node_types": sorted(synthetic_allowed),
                    },
                    meta={"request_id": req_id},
                )

            pos = node_position or position
            graph_node: Dict[str, Any] = {
                "Id": node_id_out,
                "NodeId": node_id_out,
                "NodePosition": _bpgen_vec2(pos),
            }
            if spawner_key:
                graph_node["SpawnerKey"] = spawner_key
            if function_path:
                graph_node["FunctionPath"] = function_path
            if node_type_norm in synthetic_allowed:
                # Preserve exact case for BPGen synthetic set.
                graph_node["NodeType"] = node_type_str

            # Carry across any literal defaults provided as extra_data.
            extra_data_raw = node_params.get("extra_data") or node_params.get("ExtraData") or {}
            if isinstance(extra_data_raw, dict) and extra_data_raw:
                graph_node["ExtraData"] = {str(k): _bpgen_literal(v) for k, v in extra_data_raw.items()}

            if is_function_scope:
                # Ensure function exists before applying nodes.
                bpgen_ensure_function(
                    blueprint_asset_path=blueprint_name,
                    function_name=function_name,
                    signature={},
                    create_if_missing=True,
                    update_if_exists=False,
                )

                res = bpgen_apply(
                    blueprint_asset_path=blueprint_name,
                    function_name=function_name,
                    graph_spec={"Nodes": [graph_node], "Links": []},
                    dry_run=False,
                )
            else:
                target = _bpgen_target_for_scope(scope, function_name)
                graph_spec = {
                    "target": {"blueprint_asset_path": blueprint_name, **target},
                    "Nodes": [graph_node],
                    "Links": [],
                }
                res = bpgen_apply_to_target(graph_spec=graph_spec, dry_run=False)

            # Best-effort: find the underlying node guid for convenience.
            node_guid: Optional[str] = None
            try:
                listed = bpgen_list(blueprint_asset_path=blueprint_name, function_name=function_name, include_pins=False)
                nodes = (((listed or {}).get("result") or {}).get("result") or {}).get("nodes")  # defensive
                if isinstance(nodes, list):
                    for n in nodes:
                        if isinstance(n, dict) and (n.get("node_id") == node_id_out or n.get("NodeId") == node_id_out):
                            node_guid = n.get("node_guid") or n.get("NodeGuid")
                            break
            except Exception:
                node_guid = None

            out = _sots_ok({"node_id": node_id_out, "node_guid": node_guid, "bpgen": res, "graph_scope": scope}, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"connect", "connect_pins"}:
            if not blueprint_name:
                return _sots_err(f"manage_blueprint_node.{act} requires blueprint_name", meta={"request_id": req_id})
            if is_function_scope and not function_name:
                return _sots_err(f"manage_blueprint_node.{act} requires function_name when graph_scope='function'", meta={"request_id": req_id})
            if not (is_function_scope or is_event_scope):
                return _not_implemented(
                    "manage_blueprint_node",
                    f"{act}:{scope}",
                    "Only graph_scope='function' and graph_scope='event' are supported by the BPGen parity layer today.",
                    req_id,
                )

            connections: List[Dict[str, Any]] = []
            if act == "connect":
                if not (source_node_id and source_pin and target_node_id and target_pin):
                    return _sots_err("manage_blueprint_node.connect requires source_node_id/source_pin/target_node_id/target_pin", meta={"request_id": req_id})
                connections = [
                    {
                        "source_node_id": source_node_id,
                        "source_pin_name": source_pin,
                        "target_node_id": target_node_id,
                        "target_pin_name": target_pin,
                    }
                ]
            else:
                connections_raw = extra.get("connections")
                if not isinstance(connections_raw, list) or not connections_raw:
                    return _sots_err("manage_blueprint_node.connect_pins requires extra.connections[]", meta={"request_id": req_id})
                connections = [c for c in connections_raw if isinstance(c, dict)]

            # Build a minimal GraphSpec that references existing nodes by NodeId.
            node_ref_ids: Dict[str, str] = {}
            nodes_spec: List[Dict[str, Any]] = []
            links_spec: List[Dict[str, Any]] = []

            def _ref_spec_id(nid: str) -> str:
                nid = (nid or "").strip()
                if nid not in node_ref_ids:
                    spec_id = f"N{len(node_ref_ids)}"
                    node_ref_ids[nid] = spec_id
                    nodes_spec.append({"Id": spec_id, "NodeId": nid, "allow_create": False, "allow_update": True})
                return node_ref_ids[nid]

            for c in connections:
                sn = (c.get("source_node_id") or "").strip()
                sp = (c.get("source_pin_name") or c.get("source_pin") or "").strip()
                tn = (c.get("target_node_id") or "").strip()
                tp = (c.get("target_pin_name") or c.get("target_pin") or "").strip()
                if not (sn and sp and tn and tp):
                    return _sots_err("Invalid connection entry; requires source_node_id/source_pin_name/target_node_id/target_pin_name", data={"bad_connection": c}, meta={"request_id": req_id})
                links_spec.append(
                    {
                        "FromNodeId": _ref_spec_id(sn),
                        "FromPinName": sp,
                        "ToNodeId": _ref_spec_id(tn),
                        "ToPinName": tp,
                    }
                )

            if is_function_scope:
                res = bpgen_apply(
                    blueprint_asset_path=blueprint_name,
                    function_name=function_name,
                    graph_spec={"Nodes": nodes_spec, "Links": links_spec},
                    dry_run=False,
                )
            else:
                target = _bpgen_target_for_scope(scope, function_name)
                graph_spec = {
                    "target": {"blueprint_asset_path": blueprint_name, **target},
                    "Nodes": nodes_spec,
                    "Links": links_spec,
                }
                res = bpgen_apply_to_target(graph_spec=graph_spec, dry_run=False)
            out = _sots_ok({"bpgen": res, "connections": connections}, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act == "configure":
            if not blueprint_name or not node_id:
                return _sots_err("manage_blueprint_node.configure requires blueprint_name and node_id", meta={"request_id": req_id})
            if is_function_scope and not function_name:
                return _sots_err("manage_blueprint_node.configure requires function_name when graph_scope='function'", meta={"request_id": req_id})
            if not (is_function_scope or is_event_scope):
                return _not_implemented(
                    "manage_blueprint_node",
                    f"{act}:{scope}",
                    "Only graph_scope='function' and graph_scope='event' are supported by the BPGen parity layer today.",
                    req_id,
                )
            if not property_name:
                return _sots_err("manage_blueprint_node.configure requires property_name", meta={"request_id": req_id})

            graph_node = {
                "Id": "N0",
                "NodeId": node_id,
                "allow_create": False,
                "allow_update": True,
                "ExtraData": {str(property_name): _bpgen_literal(property_value)},
            }
            if is_function_scope:
                res = bpgen_apply(
                    blueprint_asset_path=blueprint_name,
                    function_name=function_name,
                    graph_spec={"Nodes": [graph_node], "Links": []},
                    dry_run=False,
                )
            else:
                target = _bpgen_target_for_scope(scope, function_name)
                graph_spec = {
                    "target": {"blueprint_asset_path": blueprint_name, **target},
                    "Nodes": [graph_node],
                    "Links": [],
                }
                res = bpgen_apply_to_target(graph_spec=graph_spec, dry_run=False)
            out = _sots_ok({"bpgen": res, "node_id": node_id, "property_name": property_name}, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act == "move":
            if not blueprint_name or not node_id:
                return _sots_err("manage_blueprint_node.move requires blueprint_name and node_id", meta={"request_id": req_id})
            if is_function_scope and not function_name:
                return _sots_err("manage_blueprint_node.move requires function_name when graph_scope='function'", meta={"request_id": req_id})
            if not (is_function_scope or is_event_scope):
                return _not_implemented(
                    "manage_blueprint_node",
                    f"{act}:{scope}",
                    "Only graph_scope='function' and graph_scope='event' are supported by the BPGen parity layer today.",
                    req_id,
                )
            pos = node_position or position
            graph_node = {
                "Id": "N0",
                "NodeId": node_id,
                "allow_create": False,
                "allow_update": True,
                "NodePosition": _bpgen_vec2(pos),
            }
            if is_function_scope:
                res = bpgen_apply(
                    blueprint_asset_path=blueprint_name,
                    function_name=function_name,
                    graph_spec={"Nodes": [graph_node], "Links": []},
                    dry_run=False,
                )
            else:
                target = _bpgen_target_for_scope(scope, function_name)
                graph_spec = {
                    "target": {"blueprint_asset_path": blueprint_name, **target},
                    "Nodes": [graph_node],
                    "Links": [],
                }
                res = bpgen_apply_to_target(graph_spec=graph_spec, dry_run=False)
            out = _sots_ok({"bpgen": res, "node_id": node_id, "position": graph_node["NodePosition"]}, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"disconnect", "disconnect_pins"}:
            if not blueprint_name or not function_name:
                return _sots_err(f"manage_blueprint_node.{act} requires blueprint_name and function_name", meta={"request_id": req_id})

            connections_raw = extra.get("connections")
            if act == "disconnect":
                if not (source_node_id and source_pin and target_node_id and target_pin):
                    return _sots_err("manage_blueprint_node.disconnect requires source_node_id/source_pin/target_node_id/target_pin", meta={"request_id": req_id})
                connections_raw = [
                    {
                        "source_node_id": source_node_id,
                        "source_pin_name": source_pin,
                        "target_node_id": target_node_id,
                        "target_pin_name": target_pin,
                    }
                ]

            if not isinstance(connections_raw, list) or not connections_raw:
                return _sots_err("manage_blueprint_node.disconnect_pins requires extra.connections[]", meta={"request_id": req_id})

            results: List[Dict[str, Any]] = []
            for c in connections_raw:
                if not isinstance(c, dict):
                    continue
                sn = (c.get("source_node_id") or "").strip()
                sp = (c.get("source_pin_name") or c.get("source_pin") or "").strip()
                tn = (c.get("target_node_id") or "").strip()
                tp = (c.get("target_pin_name") or c.get("target_pin") or "").strip()
                if not (sn and sp and tn and tp):
                    results.append({"ok": False, "error": "invalid connection", "connection": c})
                    continue
                r = bpgen_delete_link(
                    blueprint_asset_path=blueprint_name,
                    function_name=function_name,
                    from_node_id=sn,
                    from_pin=sp,
                    to_node_id=tn,
                    to_pin=tp,
                    compile=True,
                    dry_run=False,
                )
                results.append({"ok": True, "result": r, "connection": {"from": sn, "from_pin": sp, "to": tn, "to_pin": tp}})

            out = _sots_ok({"results": results}, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        return _not_implemented(
            "manage_blueprint_node",
            act or "(empty)",
            "Implemented: discover/list/describe/delete/create/connect(_pins)/configure/move/disconnect(_pins) for function graphs via BPGen apply_graph_spec + delete_link; plus create/connect/configure/move for event graphs via apply_graph_spec_to_target.",
            req_id,
        )
    except Exception as exc:
        out = _sots_err(f"manage_blueprint_node failed: {exc}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_node", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out


@mcp.tool(
    name="manage_asset",
    description="VibeUE-compatible asset tool (partial; backed by BPGen bridge asset_* actions).",
    annotations={"readOnlyHint": not ALLOW_BPGEN_APPLY, "title": "VibeUE Compat: manage_asset"},
)
def manage_asset(
    action: str,
    search_term: str = "",
    asset_type: str = "",
    path: str = "/Game",
    case_sensitive: bool = False,
    include_engine_content: bool = False,
    max_results: int = 100,
    file_path: str = "",
    destination_path: str = "/Game/Textures/Imported",
    texture_name: Optional[str] = None,
    compression_settings: str = "TC_Default",
    generate_mipmaps: bool = True,
    validate_format: bool = True,
    auto_optimize: bool = True,
    convert_if_needed: bool = False,
    raster_size: Optional[List[int]] = None,
    auto_convert_svg: bool = True,
    asset_path: str = "",
    export_format: str = "PNG",
    max_size: Optional[List[int]] = None,
    temp_folder: str = "",
    force_open: bool = False,
    force_delete: bool = False,
    show_confirmation: bool = True,
    svg_path: str = "",
    output_path: Optional[str] = None,
    size: Optional[List[int]] = None,
    scale: float = 1,
    background: Optional[str] = None,
    new_name: str = "",
) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    try:
        if act in {"search"}:
            guard = _bpgen_guard_mutation("asset_search")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            params = {
                "search_term": (search_term or "").strip(),
                "asset_type": (asset_type or "").strip(),
                "path": (path or "/Game").strip() or "/Game",
                "case_sensitive": bool(case_sensitive),
                "include_engine_content": bool(include_engine_content),
                "max_results": int(max_results),
            }
            res = _bpgen_call("asset_search", params)
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"open_in_editor"}:
            if act == "search_types":
                if not blueprint_name:
                    return _sots_err("manage_blueprint_variable.search_types requires blueprint_name", meta={"request_id": req_id})
                res = _bpgen_call(
                    "bp_variable_search_types",
                    {
                        "blueprint_name": (blueprint_name or "").strip(),
                        "search_criteria": search_criteria or {},
                        "options": options or {},
                    },
                )
                out = _sots_ok(res, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out

            if act == "create":
                if not blueprint_name or not variable_name:
                    return _sots_err("manage_blueprint_variable.create requires blueprint_name and variable_name", meta={"request_id": req_id})

                if not _allow_apply_or_err("manage_blueprint_variable.create", req_id):
                    return _apply_gate_err("manage_blueprint_variable.create", req_id)

                # Preferred: VibeUE-style variable_config with type_path
                if isinstance(variable_config, dict) and variable_config:
                    if "type" in variable_config and "type_path" not in variable_config:
                        return _sots_err(
                            "CRITICAL: variable_config uses 'type' but should use 'type_path'",
                            meta={"request_id": req_id, "variable_config": variable_config},
                        )
                    if "type_path" not in variable_config:
                        return _sots_err(
                            "CRITICAL: variable_config missing required 'type_path'",
                            meta={"request_id": req_id, "variable_config": variable_config},
                        )

                    res = _bpgen_call(
                        "bp_variable_create",
                        {
                            "blueprint_name": (blueprint_name or "").strip(),
                            "variable_name": (variable_name or "").strip(),
                            "variable_config": variable_config,
                            "options": options or {},
                        },
                    )
                    out = _sots_ok(res, meta={"request_id": req_id})
                    _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                    return out

                # Back-compat: bpgen ensure_variable with pin_type dict
                if not isinstance(pin_type, dict) or not pin_type:
                    return _sots_err(
                        "manage_blueprint_variable.create requires variable_config (with type_path) or legacy pin_type (dict)",
                        meta={"request_id": req_id},
                    )

                res = _bpgen_call(
                    "bpgen_ensure_variable",
                    {
                        "blueprint_asset_path": (blueprint_name or "").strip(),
                        "variable_name": (variable_name or "").strip(),
                        "default_value": default_value,
                        "create_if_missing": True,
                        "update_if_exists": True,
                        "instance_editable": bool(instance_editable),
                        "expose_on_spawn": bool(expose_on_spawn),
                    },
                )
                out = _sots_ok(res, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out

            if act == "delete":
                if not blueprint_name or not variable_name:
                    return _sots_err("manage_blueprint_variable.delete requires blueprint_name and variable_name", meta={"request_id": req_id})
                if not _allow_apply_or_err("manage_blueprint_variable.delete", req_id):
                    return _apply_gate_err("manage_blueprint_variable.delete", req_id)
                res = _bpgen_call(
                    "bp_variable_delete",
                    {
                        "blueprint_name": (blueprint_name or "").strip(),
                        "variable_name": (variable_name or "").strip(),
                        "delete_options": delete_options or {},
                        "options": options or {},
                    },
                )
                out = _sots_ok(res, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out

            if act == "list":
                if not blueprint_name:
                    return _sots_err("manage_blueprint_variable.list requires blueprint_name", meta={"request_id": req_id})
                res = _bpgen_call(
                    "bp_variable_list",
                    {
                        "blueprint_name": (blueprint_name or "").strip(),
                        "list_criteria": list_criteria or {},
                        "options": options or {},
                    },
                )
                out = _sots_ok(res, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out

            if act == "get_info":
                if not blueprint_name or not variable_name:
                    return _sots_err("manage_blueprint_variable.get_info requires blueprint_name and variable_name", meta={"request_id": req_id})
                res = _bpgen_call(
                    "bp_variable_get_info",
                    {
                        "blueprint_name": (blueprint_name or "").strip(),
                        "variable_name": (variable_name or "").strip(),
                        "info_options": info_options or {},
                        "options": options or {},
                    },
                )
                out = _sots_ok(res, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out

            if act == "get_property":
                if not blueprint_name or not variable_name:
                    return _sots_err("manage_blueprint_variable.get_property requires blueprint_name and variable_name", meta={"request_id": req_id})
                if not property_path:
                    return _sots_err("manage_blueprint_variable.get_property requires property_path", meta={"request_id": req_id})
                res = _bpgen_call(
                    "bp_variable_get_property",
                    {
                        "blueprint_name": (blueprint_name or "").strip(),
                        "variable_name": (variable_name or "").strip(),
                        "property_path": (property_path or "").strip(),
                        "options": options or {},
                    },
                )
                out = _sots_ok(res, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out

            if act == "set_property":
                if not blueprint_name or not variable_name:
                    return _sots_err("manage_blueprint_variable.set_property requires blueprint_name and variable_name", meta={"request_id": req_id})
                if not property_path:
                    return _sots_err("manage_blueprint_variable.set_property requires property_path", meta={"request_id": req_id})
                if not _allow_apply_or_err("manage_blueprint_variable.set_property", req_id):
                    return _apply_gate_err("manage_blueprint_variable.set_property", req_id)
                res = _bpgen_call(
                    "bp_variable_set_property",
                    {
                        "blueprint_name": (blueprint_name or "").strip(),
                        "variable_name": (variable_name or "").strip(),
                        "property_path": (property_path or "").strip(),
                        "value": value,
                        "options": options or {},
                    },
                )
                out = _sots_ok(res, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_variable", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out

            out = _not_implemented(
                "manage_blueprint_variable",
                act or "(empty)",
                "Implemented: search_types/create/delete/list/get_info/get_property/set_property.",
                req_id,
            )
            return out

        if act in {"duplicate"}:
            if not asset_path:
                return _sots_err("manage_asset.duplicate requires asset_path", meta={"request_id": req_id})
            if not destination_path:
                return _sots_err("manage_asset.duplicate requires destination_path", meta={"request_id": req_id})

            guard = _bpgen_guard_mutation("asset_duplicate")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                "asset_duplicate",
                {
                    "asset_path": asset_path,
                    "destination_path": destination_path,
                    "new_name": (new_name or "").strip(),
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"delete"}:
            if not asset_path:
                return _sots_err("manage_asset.delete requires asset_path", meta={"request_id": req_id})

            guard = _bpgen_guard_mutation("asset_delete")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            # Bridge enforces dangerous_ok + safe-mode rules.
            res = _bpgen_call(
                "asset_delete",
                {
                    "asset_path": asset_path,
                    "force_delete": bool(force_delete),
                    "show_confirmation": bool(show_confirmation),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"import_texture"}:
            if not file_path:
                return _sots_err("manage_asset.import_texture requires file_path", meta={"request_id": req_id})
            if not destination_path:
                return _sots_err("manage_asset.import_texture requires destination_path", meta={"request_id": req_id})

            guard = _bpgen_guard_mutation("asset_import_texture")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                "asset_import_texture",
                {
                    "file_path": file_path,
                    "destination_path": destination_path,
                    "texture_name": texture_name,
                    "compression_settings": compression_settings,
                    "generate_mipmaps": bool(generate_mipmaps),
                    "validate_format": bool(validate_format),
                    "auto_optimize": bool(auto_optimize),
                    "convert_if_needed": bool(convert_if_needed),
                    "raster_size": raster_size,
                    "auto_convert_svg": bool(auto_convert_svg),
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"export_texture"}:
            if not asset_path:
                return _sots_err("manage_asset.export_texture requires asset_path", meta={"request_id": req_id})

            guard = _bpgen_guard_mutation("asset_export_texture")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                "asset_export_texture",
                {
                    "asset_path": asset_path,
                    "export_format": export_format,
                    "max_size": max_size,
                    "temp_folder": temp_folder,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"svg_to_png"}:
            if not svg_path:
                return _sots_err("manage_asset.svg_to_png requires svg_path", meta={"request_id": req_id})

            try:
                import cairosvg  # type: ignore
            except Exception as exc:
                return _sots_err(
                    f"svg_to_png requires cairosvg (not installed): {exc}",
                    meta={"request_id": req_id, "hint": "pip install cairosvg"},
                )

            out_path = output_path or ""
            if not out_path:
                return _sots_err("manage_asset.svg_to_png requires output_path", meta={"request_id": req_id})

            w = h = None
            if isinstance(size, list) and len(size) == 2:
                try:
                    w = int(size[0])
                    h = int(size[1])
                except Exception:
                    w = h = None

            try:
                cairosvg.svg2png(
                    url=svg_path,
                    write_to=out_path,
                    output_width=w,
                    output_height=h,
                    scale=float(scale) if scale else 1.0,
                    background_color=background,
                )
                out = _sots_ok({"output_path": out_path}, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out
            except Exception as exc:
                out = _sots_err(f"svg_to_png failed: {exc}", meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out

        hint = (
            "Implemented: search/open_in_editor/duplicate/delete/import_texture/export_texture. "
            "svg_to_png is Python-local and requires cairosvg. "
            "Note: delete is dangerous and will be blocked if BPGen safe mode is enabled."
        )
        return _not_implemented("manage_asset", act or "(empty)", hint, req_id)
    except Exception as exc:
        out = _sots_err(f"manage_asset failed: {exc}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "manage_asset", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out


@mcp.tool(
    name="manage_blueprint_component",
    description="VibeUE-compatible Blueprint component tool (partial; backed by BPGen bridge component ops).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "VibeUE Compat: manage_blueprint_component"},
)
def manage_blueprint_component(
    action: str,
    blueprint_name: str = "",
    component_name: str = "",
    component_type: str = "",
    parent_name: str = "root",
    property_name: str = "",
    property_value: Any = None,
    properties: Optional[Dict[str, Any]] = None,
    location: Optional[List[float]] = None,
    rotation: Optional[List[float]] = None,
    scale: Optional[List[float]] = None,
    remove_children: bool = True,
    include_property_values: bool = False,
    include_inherited: bool = True,
    category: str = "",
    search_text: str = "",
    base_class: str = "",
    include_abstract: bool = False,
    include_deprecated: bool = False,
    component_order: Optional[List[str]] = None,
    options: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    try:
        if act in {"search_types"}:
            res = _bpgen_call(
                "bp_component_search_types",
                {
                    "category": (category or "").strip(),
                    "search_text": (search_text or "").strip(),
                    "base_class": (base_class or "").strip(),
                    "include_abstract": bool(include_abstract),
                    "include_deprecated": bool(include_deprecated),
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"get_info"}:
            if not component_type:
                return _sots_err("manage_blueprint_component.get_info requires component_type", meta={"request_id": req_id})

            res = _bpgen_call(
                "bp_component_get_info",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "component_type": (component_type or "").strip(),
                    "component_name": (component_name or "").strip(),
                    "include_property_values": bool(include_property_values),
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"list"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_component.list requires blueprint_name", meta={"request_id": req_id})

            res = _bpgen_call("bp_component_list", {"blueprint_name": (blueprint_name or "").strip()})
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"get_property"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_component.get_property requires blueprint_name", meta={"request_id": req_id})
            if not component_name:
                return _sots_err("manage_blueprint_component.get_property requires component_name", meta={"request_id": req_id})
            if not property_name:
                return _sots_err("manage_blueprint_component.get_property requires property_name", meta={"request_id": req_id})

            res = _bpgen_call(
                "bp_component_get_property",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "component_name": (component_name or "").strip(),
                    "property_name": (property_name or "").strip(),
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"set_property", "set"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_component.set_property requires blueprint_name", meta={"request_id": req_id})
            if not component_name:
                return _sots_err("manage_blueprint_component.set_property requires component_name", meta={"request_id": req_id})
            if not property_name:
                return _sots_err("manage_blueprint_component.set_property requires property_name", meta={"request_id": req_id})

            guard = _bpgen_guard_mutation("bp_component_set_property")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                "bp_component_set_property",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "component_name": (component_name or "").strip(),
                    "property_name": (property_name or "").strip(),
                    "property_value": _ue_import_text(property_value),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"get_all_properties"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_component.get_all_properties requires blueprint_name", meta={"request_id": req_id})
            if not component_name:
                return _sots_err("manage_blueprint_component.get_all_properties requires component_name", meta={"request_id": req_id})

            res = _bpgen_call(
                "bp_component_get_all_properties",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "component_name": (component_name or "").strip(),
                    "include_inherited": bool(include_inherited),
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"compare_properties"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_component.compare_properties requires blueprint_name", meta={"request_id": req_id})
            if not component_name:
                return _sots_err("manage_blueprint_component.compare_properties requires component_name", meta={"request_id": req_id})

            opts = options or {}
            other_bp = (opts.get("compare_to_blueprint") or "").strip()
            other_comp = (opts.get("compare_to_component") or "").strip()
            if not other_bp or not other_comp:
                return _sots_err(
                    "compare_properties requires options.compare_to_blueprint and options.compare_to_component",
                    meta={"request_id": req_id},
                )

            src = _bpgen_call(
                "bp_component_get_all_properties",
                {"blueprint_name": blueprint_name, "component_name": component_name, "include_inherited": bool(include_inherited)},
            )
            dst = _bpgen_call(
                "bp_component_get_all_properties",
                {"blueprint_name": other_bp, "component_name": other_comp, "include_inherited": bool(include_inherited)},
            )

            if not src.get("ok", False):
                out = _sots_err("compare_properties source get_all_properties failed", data={"bpgen": src}, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out
            if not dst.get("ok", False):
                out = _sots_err("compare_properties target get_all_properties failed", data={"bpgen": dst}, meta={"request_id": req_id})
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
                return out

            src_props = (src.get("result") or {}).get("properties") or {}
            dst_props = (dst.get("result") or {}).get("properties") or {}

            def _norm(v: Any) -> str:
                if isinstance(v, (dict, list)):
                    try:
                        return json.dumps(v, sort_keys=True, ensure_ascii=False)
                    except Exception:
                        return str(v)
                return str(v)

            diffs: List[Dict[str, Any]] = []
            matching = 0
            for key in sorted(set(src_props.keys()) | set(dst_props.keys())):
                sv = src_props.get(key, None)
                tv = dst_props.get(key, None)
                if _norm(sv) == _norm(tv):
                    matching += 1
                else:
                    diffs.append({"property": key, "source_value": sv, "target_value": tv})

            result = {
                "matches": len(diffs) == 0,
                "matching_count": matching,
                "difference_count": len(diffs),
                "differences": diffs,
                "source": {"blueprint": blueprint_name, "component": component_name},
                "target": {"blueprint": other_bp, "component": other_comp},
            }

            out = _sots_ok(result, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"create"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_component.create requires blueprint_name", meta={"request_id": req_id})
            if not component_name:
                return _sots_err("manage_blueprint_component.create requires component_name", meta={"request_id": req_id})
            if not component_type:
                return _sots_err("manage_blueprint_component.create requires component_type", meta={"request_id": req_id})

            guard = _bpgen_guard_mutation("bp_component_create")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            init_props: Dict[str, str] = {}
            if properties:
                for k, v in properties.items():
                    key = (k or "").strip()
                    if not key:
                        continue
                    init_props[key] = _ue_import_text(v)

            parent = (parent_name or "").strip()
            res = _bpgen_call(
                "bp_component_create",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "component_name": (component_name or "").strip(),
                    "component_type": (component_type or "").strip(),
                    "parent_name": parent,
                    "location": location,
                    "rotation": rotation,
                    "scale": scale,
                    "properties": init_props,
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"delete"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_component.delete requires blueprint_name", meta={"request_id": req_id})
            if not component_name:
                return _sots_err("manage_blueprint_component.delete requires component_name", meta={"request_id": req_id})

            guard = _bpgen_guard_mutation("bp_component_delete")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                "bp_component_delete",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "component_name": (component_name or "").strip(),
                    "remove_children": bool(remove_children),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"reparent"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_component.reparent requires blueprint_name", meta={"request_id": req_id})
            if not component_name:
                return _sots_err("manage_blueprint_component.reparent requires component_name", meta={"request_id": req_id})
            if not parent_name:
                return _sots_err("manage_blueprint_component.reparent requires parent_name", meta={"request_id": req_id})

            guard = _bpgen_guard_mutation("bp_component_reparent")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                "bp_component_reparent",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "component_name": (component_name or "").strip(),
                    "parent_name": (parent_name or "").strip(),
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"reorder"}:
            if not blueprint_name:
                return _sots_err("manage_blueprint_component.reorder requires blueprint_name", meta={"request_id": req_id})
            if not component_order:
                return _sots_err("manage_blueprint_component.reorder requires component_order", meta={"request_id": req_id})

            guard = _bpgen_guard_mutation("bp_component_reorder")
            if guard:
                guard["meta"]["request_id"] = req_id
                _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
                return guard

            res = _bpgen_call(
                "bp_component_reorder",
                {
                    "blueprint_name": (blueprint_name or "").strip(),
                    "component_order": component_order,
                    "dangerous_ok": True,
                },
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"get_property_metadata"}:
            if not component_type:
                return _sots_err("manage_blueprint_component.get_property_metadata requires component_type", meta={"request_id": req_id})
            if not property_name:
                return _sots_err("manage_blueprint_component.get_property_metadata requires property_name", meta={"request_id": req_id})

            res = _bpgen_call(
                "bp_component_get_property_metadata",
                {"component_type": (component_type or "").strip(), "property_name": (property_name or "").strip()},
            )
            out = _sots_ok(res, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        hint = (
            "Implemented: search_types/get_info/list/get_property/set_property/get_all_properties/compare_properties/"
            "create/delete/reparent/reorder/get_property_metadata. "
            "Note: reorder is currently a no-op in the bridge (warning returned)."
        )
        out = _not_implemented("manage_blueprint_component", act or "(empty)", hint, req_id)
        _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    except Exception as exc:
        out = _sots_err(f"manage_blueprint_component failed: {exc}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "manage_blueprint_component", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out


@mcp.tool(
    name="manage_umg_widget",
    description="VibeUE-compatible UMG widget tool (partial; backed by BPGen UMG ensure/set actions).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "VibeUE Compat: manage_umg_widget"},
)
def manage_umg_widget(
    action: str,
    widget_name: str = "",
    component_name: str = "",
    component_type: str = "",
    parent_name: str = "root",
    property_name: str = "",
    property_value: Any = None,
    property_type: str = "auto",
    is_variable: bool = True,
    properties: Optional[Dict[str, Any]] = None,
    remove_children: bool = True,
    remove_from_variables: bool = True,
    category: str = "",
    search_text: str = "",
    include_custom: bool = True,
    include_engine: bool = True,
    parent_compatibility: str = "",
    include_inherited: bool = True,
    category_filter: str = "",
    input_events: Optional[Dict[str, str]] = None,
    options: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    try:
        if act in {"add_component", "add"}:
            if not widget_name:
                return _sots_err("manage_umg_widget.add_component requires widget_name (WidgetBlueprint asset path)", meta={"request_id": req_id})
            if not component_name:
                return _sots_err("manage_umg_widget.add_component requires component_name", meta={"request_id": req_id})
            if not component_type:
                return _sots_err("manage_umg_widget.add_component requires component_type", meta={"request_id": req_id})

            widget_class_path = _umg_class_path(component_type)
            if not widget_class_path:
                return _sots_err("Invalid component_type", meta={"request_id": req_id})

            opts = options or {}
            insert_index = int(opts.get("insert_index", -1)) if isinstance(opts.get("insert_index", -1), (int, float, str)) else -1

            widget_props, slot_props = _umg_split_props(properties)

            parent = (parent_name or "").strip()
            if parent.lower() == "root":
                parent = ""

            res = bpgen_ensure_widget(
                blueprint_asset_path=widget_name,
                widget_class_path=widget_class_path,
                widget_name=component_name,
                parent_name=parent,
                insert_index=insert_index,
                is_variable=is_variable,
                create_if_missing=bool(opts.get("create_if_missing", True)),
                update_if_exists=bool(opts.get("update_if_exists", True)),
                reparent_if_mismatch=bool(opts.get("reparent_if_mismatch", True)),
                properties=widget_props,
                slot_properties=slot_props,
                dry_run=bool(opts.get("dry_run", False)),
            )
            out = _sots_ok({"bpgen": res, "widget_name": widget_name, "component_name": component_name}, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_umg_widget", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        if act in {"set_property", "set"}:
            if not widget_name:
                return _sots_err("manage_umg_widget.set_property requires widget_name (WidgetBlueprint asset path)", meta={"request_id": req_id})
            if not component_name:
                return _sots_err("manage_umg_widget.set_property requires component_name", meta={"request_id": req_id})
            if not property_name:
                return _sots_err("manage_umg_widget.set_property requires property_name", meta={"request_id": req_id})

            # NOTE: property_type is currently advisory only. ImportText parsing is performed
            # by UE on the other end; for complex types, callers may need to pass an ImportText string.
            _ = property_type

            opts = options or {}
            props_map = {property_name: _umg_import_text(property_value)}
            res = bpgen_set_widget_properties(
                blueprint_asset_path=widget_name,
                widget_name=component_name,
                properties=props_map,
                fail_if_missing=bool(opts.get("fail_if_missing", True)),
                dry_run=bool(opts.get("dry_run", False)),
            )
            out = _sots_ok({"bpgen": res, "widget_name": widget_name, "component_name": component_name, "property": property_name}, meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "manage_umg_widget", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        hint = (
            "Implemented: add_component (ensure_widget), set_property (set_widget_properties). "
            "Not yet implemented: list_components/get_property/get_component_properties/remove_component/validate/search_types/get_available_events/bind_events/list_properties. "
            "Note: BPGen supports property bindings via bpgen_ensure_binding, but VibeUE bind_events targets input delegates and is not mapped yet."
        )
        return _not_implemented("manage_umg_widget", act or "(empty)", hint, req_id)
    except Exception as exc:
        out = _sots_err(f"manage_umg_widget failed: {exc}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "manage_umg_widget", "action": act, "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out


@mcp.tool(
    name="manage_enhanced_input",
    description="VibeUE-compatible Enhanced Input tool (not implemented in BPGen parity layer yet).",
    annotations={"readOnlyHint": True, "title": "VibeUE Compat: manage_enhanced_input"},
)
def manage_enhanced_input(action: str) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    return _not_implemented("manage_enhanced_input", act or "(empty)", "Enhanced Input ops require new Unreal-side editor services.", req_id)


@mcp.tool(
    name="manage_level_actors",
    description="VibeUE-compatible Level Actors tool (not implemented in BPGen parity layer yet).",
    annotations={"readOnlyHint": True, "title": "VibeUE Compat: manage_level_actors"},
)
def manage_level_actors(action: str) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    return _not_implemented("manage_level_actors", act or "(empty)", "Level actor ops are not part of BPGen today; requires new editor service + gate decisions.", req_id)


@mcp.tool(
    name="manage_material",
    description="VibeUE-compatible Material tool (not implemented in BPGen parity layer yet).",
    annotations={"readOnlyHint": True, "title": "VibeUE Compat: manage_material"},
)
def manage_material(action: str) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    return _not_implemented("manage_material", act or "(empty)", "Material ops require new Unreal-side editor services.", req_id)


@mcp.tool(
    name="manage_material_node",
    description="VibeUE-compatible Material Node tool (not implemented in BPGen parity layer yet).",
    annotations={"readOnlyHint": True, "title": "VibeUE Compat: manage_material_node"},
)
def manage_material_node(action: str) -> Dict[str, Any]:
    req_id = _new_request_id()
    act = _vibeue_action_str(action)
    return _not_implemented("manage_material_node", act or "(empty)", "Material graph ops require new Unreal-side editor services.", req_id)


@mcp.tool(
    name="check_unreal_connection",
    description="VibeUE-compatible connection check (implemented as BPGen bridge ping + unified server info).",
    annotations={"readOnlyHint": True, "title": "VibeUE Compat: check_unreal_connection"},
)
def check_unreal_connection() -> Dict[str, Any]:
    req_id = _new_request_id()
    try:
        ping = bpgen_ping_tool()
        info = server_info()
        ok = bool(ping.get("ok"))
        data = {
            "success": ok,
            "connection_status": "Connected" if ok else "Not Connected",
            "plugin_status": "BPGenBridge" if ok else "Unknown",
            "bpgen_ping": ping,
            "server_info": info,
            "troubleshooting": [
                "Ensure Unreal Editor is running.",
                "Ensure SOTS_BPGen_Bridge is running and reachable (SOTS_BPGEN_HOST/PORT).",
                "Check DevTools/python/logs/sots_mcp_server.log and the UE bridge logs.",
            ] if not ok else [],
        }
        out = _sots_ok(data, meta={"request_id": req_id})
        out["ok"] = ok
        _jsonl_log({"ts": time.time(), "tool": "check_unreal_connection", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    except Exception as exc:
        out = _sots_err(f"check_unreal_connection failed: {exc}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "check_unreal_connection", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out


@mcp.tool(
    name="get_help",
    description="VibeUE-compatible help entrypoint (topic routing over VibeUE resources + unified server index).",
    annotations={"readOnlyHint": True, "title": "VibeUE Compat: get_help"},
)
def get_help(topic: str = "overview") -> Dict[str, Any]:
    req_id = _new_request_id()
    try:
        def _find_vibeue_resources_root() -> Optional[Path]:
            candidates = [
                (PATHS.project_root / "Plugins" / "VibeUE" / "Content" / "Python" / "resources").resolve(),
                (PATHS.project_root / "WORKING" / "VibeUE-master" / "Content" / "Python" / "resources").resolve(),
            ]
            for cand in candidates:
                if cand.exists() and cand.is_dir():
                    return cand
            return None

        def _list_topics(resources_root: Path) -> List[str]:
            topics_dir = (resources_root / "topics").resolve()
            if not topics_dir.exists() or not topics_dir.is_dir():
                return []
            _ensure_under_root(topics_dir, resources_root)
            return sorted({p.stem for p in topics_dir.glob("*.md") if p.is_file()})

        def _read_md(p: Path, resources_root: Path) -> str:
            pp = p.resolve()
            _ensure_under_root(pp, resources_root)
            return pp.read_text(encoding="utf-8", errors="replace")

        requested = (topic or "overview").strip()
        topic_key = requested.lower().replace("_", "-")
        if not topic_key:
            topic_key = "overview"

        resources_root = _find_vibeue_resources_root()
        if not resources_root:
            out = _sots_err(
                "VibeUE help resources not found (expected Plugins/VibeUE/Content/Python/resources)",
                meta={"request_id": req_id},
            )
            _jsonl_log({"ts": time.time(), "tool": "get_help", "ok": out["ok"], "error": out["error"], "request_id": req_id})
            return out

        available_topics = _list_topics(resources_root)

        # Prefer the AI-optimized per-topic markdown under resources/topics/.
        topics_dir = (resources_root / "topics").resolve()
        topic_path = (topics_dir / f"{topic_key}.md").resolve()
        fallback_topics_index = (topics_dir / "topics.md").resolve()
        fallback_full_help = (resources_root / "help.md").resolve()

        warnings: List[str] = []
        resolved_topic = topic_key
        source_path: Optional[Path] = None
        content_md = ""

        if topic_key in {"help", "index"} and fallback_full_help.exists():
            resolved_topic = "help"
            source_path = fallback_full_help
            content_md = _read_md(source_path, resources_root)
        elif topic_path.exists():
            source_path = topic_path
            content_md = _read_md(source_path, resources_root)
        elif topic_key == "umg-guide":
            # Legacy capitalization in some VibeUE bundles; prefer topics/umg-guide.md if present.
            legacy_umg = (resources_root / "UMG-Guide.md").resolve()
            if legacy_umg.exists():
                source_path = legacy_umg
                content_md = _read_md(source_path, resources_root)
        
        if not content_md:
            warnings.append(f"Unknown help topic '{requested}'. Returning topic index.")
            resolved_topic = "topics"
            if fallback_topics_index.exists():
                source_path = fallback_topics_index
                content_md = _read_md(source_path, resources_root)
            elif fallback_full_help.exists():
                source_path = fallback_full_help
                content_md = _read_md(source_path, resources_root)
            else:
                content_md = "(No help content found on disk.)"

        rel_source = ""
        if source_path:
            try:
                rel_source = str(source_path.resolve().relative_to(PATHS.project_root.resolve())).replace("\\", "/")
            except Exception:
                rel_source = str(source_path)

        out = _sots_ok(
            {
                "topic": resolved_topic,
                "requested_topic": requested,
                "content_markdown": content_md,
                "source_path": rel_source,
                "available_topics": available_topics,
                "server_index": sots_help(),
            },
            warnings=warnings,
            meta={"request_id": req_id},
        )
        _jsonl_log({"ts": time.time(), "tool": "get_help", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    except Exception as exc:
        out = _sots_err(f"get_help failed: {exc}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "get_help", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out

# File-safe logging (never stdout)
_SERVER_LOGGER: Optional[logging.Logger] = None


def _get_server_logger() -> logging.Logger:
    global _SERVER_LOGGER
    if _SERVER_LOGGER is not None:
        return _SERVER_LOGGER

    logger = logging.getLogger("sots_mcp_server")
    logger.setLevel(logging.INFO)
    fmt = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")

    sh = logging.StreamHandler(stream=sys.stderr)
    sh.setFormatter(fmt)
    logger.addHandler(sh)

    try:
        log_dir = PATHS.devtools_py / "logs"
        log_dir.mkdir(parents=True, exist_ok=True)
        fh = logging.FileHandler(log_dir / "sots_mcp_server.log", encoding="utf-8")
        fh.setFormatter(fmt)
        logger.addHandler(fh)
    except Exception:
        pass

    logger.propagate = False
    _SERVER_LOGGER = logger
    return logger


def _new_request_id() -> str:
    return uuid.uuid4().hex[:12]


def _jsonl_log(entry: Dict[str, Any]) -> None:
    try:
        log_dir = PATHS.devtools_py / "logs"
        log_dir.mkdir(parents=True, exist_ok=True)
        p = log_dir / "sots_mcp_server.jsonl"
        entry2 = dict(entry)
        # Redact secrets
        def _redact(val: Any) -> Any:
            if isinstance(val, str) and ("OPENAI_API_KEY" in val or "Authorization" in val):
                return "[redacted]"
            return val
        for k in list(entry2.keys()):
            entry2[k] = _redact(entry2[k])
        with p.open("a", encoding="utf-8") as f:
            f.write(json.dumps(entry2, ensure_ascii=False) + "\n")
    except Exception:
        pass


# Agents orchestrator (lazy init)
_AGENTS_ORCH: Optional[SOTSAgentsOrchestrator] = None
_AGENTS_CFG: Optional[AgentsRunConfig] = None


def _build_agents_cfg() -> AgentsRunConfig:
    session_db_env = os.environ.get("SOTS_AGENTS_SESSION_DB")
    session_db_path = Path(session_db_env).expanduser() if session_db_env else PATHS.devtools_py / "sots_agents_sessions.db"
    session_db_path.parent.mkdir(parents=True, exist_ok=True)

    laws_env = os.environ.get("SOTS_LAWS_FILE")
    laws_path = Path(laws_env).expanduser() if laws_env else None
    if laws_path and not laws_path.exists():
        laws_path = None

    return AgentsRunConfig(
        repo_root=PATHS.project_root,
        mode=os.environ.get("SOTS_AGENTS_MODE", "auto"),
        session_id=os.environ.get("SOTS_AGENTS_SESSION_ID", "sots_mcp"),
        session_db_path=session_db_path,
        laws_file=laws_path,
        enable_subprocess=_env_bool("SOTS_AGENTS_ENABLE_SUBPROCESS", False),
        allow_apply=ALLOW_APPLY,
        server_name="sots-devtools-agents",
        server_version=os.environ.get("SOTS_AGENTS_VERSION", "0.1.0"),
    )


def _get_agents_orchestrator() -> SOTSAgentsOrchestrator:
    global _AGENTS_ORCH, _AGENTS_CFG
    if _AGENTS_ORCH is None:
        cfg = _build_agents_cfg()
        logger = setup_agents_logging(cfg.repo_root, name="sots_agents_mcp", mcp_mode=True)
        _AGENTS_CFG = cfg
        _AGENTS_ORCH = SOTSAgentsOrchestrator(cfg, logger=logger)
        _elog(f"[SOTS_MCP] Agents orchestrator ready (allow_apply={cfg.allow_apply}, mode={cfg.mode})")
        _get_server_logger().info(
            "Agents orchestrator ready allow_apply=%s mode=%s session_db=%s",
            cfg.allow_apply,
            cfg.mode,
            cfg.session_db_path,
        )
    return _AGENTS_ORCH


def _agents_info() -> Dict[str, Any]:
    cfg = _AGENTS_CFG or _build_agents_cfg()
    return {
        "repo_root": str(cfg.repo_root),
        "session_id": cfg.session_id,
        "session_db_path": str(cfg.session_db_path),
        "mode": cfg.mode,
        "allow_apply": cfg.allow_apply,
        "enable_subprocess": cfg.enable_subprocess,
    }


# BPGen bridge config + helpers
BPGEN_READ_ONLY_ACTIONS = {
    "ping",
    "discover_nodes",
    "get_spec_schema",
    "canonicalize_spec",
    "list_nodes",
    "describe_node",
    # Asset ops that are effectively read-only at the project level.
    "asset_search",
    "asset_export_texture",
    "asset_list_references",

    # Blueprint component ops (read-only)
    "bp_component_search_types",
    "bp_component_get_info",
    "bp_component_list",
    "bp_component_get_property",
    "bp_component_get_all_properties",
    "bp_component_get_property_metadata",

    # Blueprint function ops (read-only)
    "bp_function_list",
    "bp_function_get",
    "bp_function_list_params",
    "bp_function_list_locals",
}

BPGEN_CFG = {
    "host": os.environ.get("SOTS_BPGEN_HOST", BPGEN_DEFAULT_HOST),
    "port": int(os.environ.get("SOTS_BPGEN_PORT", str(BPGEN_DEFAULT_PORT))),
    "connect_timeout": float(os.environ.get("SOTS_BPGEN_CONNECT_TIMEOUT", str(BPGEN_DEFAULT_CONNECT_TIMEOUT))),
    "recv_timeout": float(os.environ.get("SOTS_BPGEN_TIMEOUT", str(BPGEN_DEFAULT_RECV_TIMEOUT))),
    "max_bytes": int(os.environ.get("SOTS_BPGEN_MAX_BYTES", str(BPGEN_DEFAULT_MAX_BYTES))),
}

BPGEN_MUTATION_ACTIONS = {
    "create_blueprint_asset",
    "apply_graph_spec",
    "compile_blueprint",
    "save_blueprint",
    "refresh_nodes",
    "delete_node_by_id",
    "delete_link",
    "replace_node",
    "ensure_function",
    "ensure_variable",

    # Blueprint component ops (mutations)
    "bp_component_create",
    "bp_component_delete",
    "bp_component_reparent",
    "bp_component_reorder",
    "bp_component_set_property",

    # Blueprint function ops (mutations)
    "bp_function_delete",
    "bp_function_add_param",
    "bp_function_remove_param",
    "bp_function_update_param",
    "bp_function_add_local",
    "bp_function_remove_local",
    "bp_function_update_local",
    "bp_function_update_properties",
}


def _bpgen_call(action: str, params: Optional[Dict[str, Any]]) -> Dict[str, Any]:
    return bpgen_call(
        action,
        params or {},
        host=BPGEN_CFG["host"],
        port=BPGEN_CFG["port"],
        connect_timeout=BPGEN_CFG["connect_timeout"],
        recv_timeout=BPGEN_CFG["recv_timeout"],
        max_bytes=BPGEN_CFG["max_bytes"],
    )


def _bpgen_guard_mutation(action: str) -> Optional[Dict[str, Any]]:
    if not _is_allowed_bpgen_mutation() and action not in BPGEN_READ_ONLY_ACTIONS:
        _get_server_logger().warning("Blocked bpgen mutation action=%s (SOTS_ALLOW_BPGEN_APPLY=0)", action)
        return _sots_err("Blocked by SOTS_ALLOW_BPGEN_APPLY=0", meta={"action": action})
    return None

# Allowlist of executable scripts (read-only operations)
ALLOWED_SCRIPTS: Dict[str, Path] = {
    "depmap": PATHS.devtools_py / "sots_plugin_depmap.py",
    "tag_usage": PATHS.devtools_py / "sots_tag_usage_report.py",
    "todo_backlog": PATHS.devtools_py / "sots_todo_backlog.py",
    "api_surface": PATHS.devtools_py / "sots_api_surface.py",
    "plugin_discovery": PATHS.devtools_py / "sots_plugin_discovery.py",
    # Optional (if present in your toolbox):
    "sots_tools": PATHS.devtools_py / "sots_tools.py",
}
ALLOWED_SCRIPTS_META: Dict[str, Dict[str, Any]] = {
    name: {"read_only": True} for name in ALLOWED_SCRIPTS.keys()
}

SAFE_SEARCH_ROOTS = {
    "Plugins": PATHS.project_root / "Plugins",
    "DevTools": PATHS.project_root / "DevTools",
    "Source": PATHS.project_root / "Source",
    "Config": PATHS.project_root / "Config",
    "Content": PATHS.project_root / "Content",
    "Repo": PATHS.project_root,
    "Exports": EXPORTS_ROOT,
    "Reports": REPORTS_DIR,
}

DEFAULT_EXTS = [".h", ".hpp", ".cpp", ".c", ".cc", ".inl", ".cs", ".uplugin", ".uproject", ".ini", ".py", ".md", ".txt", ".json"]
IMAGE_ALLOWED_EXTS = {".jpg", ".jpeg", ".png", ".webp"}
IMAGE_MAX_BYTES = 8 * 1024 * 1024  # 8MB pre-check
IMAGE_MIN_DIM = 256
IMAGE_MAX_DIM = 4096
IMAGE_DEFAULT_MAX_DIM = 1280
IMAGE_DEFAULT_QUALITY = 85
IMAGE_SANDBOX_ROOT = (PATHS.devtools_py / "SOTS_Capture").resolve()
IMAGE_DEFAULT_PATH = (IMAGE_SANDBOX_ROOT / "VisualDigest" / "latest" / "latest.jpg").resolve()

def _safe_relpath(p: Path, root: Path) -> str:
    try:
        return str(p.resolve().relative_to(root.resolve()))
    except Exception:
        return str(p.resolve())

def _get_root(root: str) -> Path:
    key = (root or "repo").strip().lower()
    if key not in ROOT_MAP:
        raise ValueError(f"Invalid root: {root}. Allowed: {sorted(ROOT_MAP.keys())}")
    return ROOT_MAP[key]


def _gate(name: str, default: bool = False) -> bool:
    return _env_bool(name, default)


def _is_allowed_bpgen_mutation() -> bool:
    return ALLOW_BPGEN_APPLY


def _is_allowed_devtools_run() -> bool:
    return ALLOW_DEVTOOLS_RUN

def _ensure_under_root(p: Path, root: Path) -> Path:
    pr = p.resolve()
    rr = root.resolve()
    if rr not in pr.parents and pr != rr:
        raise ValueError(f"Path escapes root. path={pr} root={rr}")
    return pr

def _run_python(script: Path, args: Sequence[str]) -> Dict[str, Any]:
    script = script.resolve()
    if not script.exists():
        return {"ok": False, "error": f"Script missing: {script}"}

    cmd = ["python", str(script), *list(args)]
    _elog(f"[SOTS_MCP] RUN: {' '.join(cmd)}  (cwd={PATHS.project_root})")

    p = subprocess.run(
        cmd,
        cwd=str(PATHS.project_root),
        capture_output=True,
        text=True,
    )

    # Never print stdout; return it to client.
    return {
        "ok": p.returncode == 0,
        "returncode": p.returncode,
        "stdout": (p.stdout or "")[-20000:],  # trim
        "stderr": (p.stderr or "")[-20000:],
    }

def _list_reports(max_items: int = 200) -> List[Dict[str, Any]]:
    reports: List[Dict[str, Any]] = []
    if not REPORTS_DIR.exists():
        return reports

    for p in sorted(REPORTS_DIR.rglob("*")):
        if not p.is_file():
            continue
        try:
            st = p.stat()
            reports.append({
                "path": _safe_relpath(p, REPORTS_DIR),
                "bytes": st.st_size,
                "mtime_epoch": int(st.st_mtime),
            })
        except Exception:
            continue

        if len(reports) >= max_items:
            break
    return reports

def _read_text_file(rel_path: str, max_chars: int = 20000) -> Dict[str, Any]:
    p = (REPORTS_DIR / rel_path).resolve()
    _ensure_under_root(p, REPORTS_DIR)
    if not p.exists() or not p.is_file():
        return {"ok": False, "error": f"File not found: {rel_path}"}

    reports_root = REPORTS_DIR.resolve()
    if reports_root not in p.parents and p != reports_root:
        return {"ok": False, "error": "Read denied: only DevTools/python/reports/* is readable via this tool."}

    try:
        txt = p.read_text(encoding="utf-8", errors="replace")
    except Exception as e:
        return {"ok": False, "error": f"Read failed: {e}"}

    if len(txt) > max_chars:
        txt = txt[:max_chars] + "\n...[truncated]..."
    return {"ok": True, "path": rel_path, "text": txt}

def _compile_pattern(query: str, regex: bool, case_sensitive: bool) -> re.Pattern:
    flags = 0 if case_sensitive else re.IGNORECASE
    pat = query if regex else re.escape(query)
    return re.compile(pat, flags)


def _clamp_int(value: int, lo: int, hi: int) -> int:
    return max(lo, min(hi, value))


def _mime_for_ext(ext: str) -> str:
    ext = ext.lower()
    if ext in {".jpg", ".jpeg"}:
        return "image/jpeg"
    if ext == ".png":
        return "image/png"
    if ext == ".webp":
        return "image/webp"
    guess, _ = mimetypes.guess_type("file" + ext)
    return guess or "application/octet-stream"


def _build_content_blocks(img_bytes: bytes, mime: str, meta_text: str) -> List[Any]:
    b64 = base64.b64encode(img_bytes).decode("ascii")
    if ImageContent and TextContent:
        try:
            return [
                ImageContent(type="image", data=b64, mimeType=mime),
                TextContent(type="text", text=meta_text),
            ]
        except Exception:
            pass
    return [
        {"type": "image", "data": b64, "mimeType": mime},
        {"type": "text", "text": meta_text},
    ]


def _load_image_with_optional_downscale(path: Path, max_dim: int, quality: int, force_jpeg: bool, request_id: str) -> Tuple[List[Any], Dict[str, Any]]:
    p = path.resolve()
    _ensure_under_root(p, PATHS.project_root)
    _ensure_under_root(p, IMAGE_SANDBOX_ROOT)  # stay under DevTools/python/SOTS_Capture

    if IMAGE_SANDBOX_ROOT not in p.parents and p != IMAGE_DEFAULT_PATH:
        raise ValueError(f"Image path not allowed; expected under {IMAGE_SANDBOX_ROOT}")

    if p.suffix.lower() not in IMAGE_ALLOWED_EXTS:
        raise ValueError(f"Extension not allowed: {p.suffix}. Allowed: {sorted(IMAGE_ALLOWED_EXTS)}")

    if not p.exists() or not p.is_file():
        raise FileNotFoundError(f"Image not found: {p}")

    st = p.stat()
    if st.st_size > IMAGE_MAX_BYTES and ImageContent is None:
        raise ValueError("Image exceeds max bytes and Pillow not available for downscale/convert. Install pillow.")

    meta: Dict[str, Any] = {
        "path": _safe_relpath(p, PATHS.project_root),
        "bytes": st.st_size,
        "mtime_iso": dt.datetime.fromtimestamp(st.st_mtime).astimezone().isoformat(timespec="seconds"),
        "force_jpeg": bool(force_jpeg),
        "request_id": request_id,
    }

    max_dim = _clamp_int(max_dim, IMAGE_MIN_DIM, IMAGE_MAX_DIM)
    quality = _clamp_int(quality, 40, 95)

    try:
        from PIL import Image  # type: ignore
    except Exception:
        meta["pillow"] = False
        if st.st_size > IMAGE_MAX_BYTES:
            raise ValueError("Image exceeds max bytes and Pillow not available.")
        data = p.read_bytes()
        mime = _mime_for_ext(p.suffix)
        meta["mode"] = "raw"
        meta["final_bytes"] = len(data)
        return _build_content_blocks(data, mime, json.dumps(meta, indent=2)), meta

    meta["pillow"] = True
    with Image.open(p) as im:
        orig_w, orig_h = im.size
        meta["orig_size"] = [orig_w, orig_h]
        target_max = max_dim
        if target_max > 0:
            scale = min(target_max / float(orig_w), target_max / float(orig_h), 1.0)
        else:
            scale = 1.0
        if scale < 1.0:
            new_w = max(1, int(orig_w * scale))
            new_h = max(1, int(orig_h * scale))
            im = im.resize((new_w, new_h))
            meta["resized"] = [new_w, new_h]
        fmt = "JPEG" if force_jpeg or im.format not in {"JPEG", "PNG", "WEBP"} else (im.format or "JPEG")
        mime = "image/jpeg" if fmt == "JPEG" else _mime_for_ext(f".{fmt.lower()}")
        buf = io.BytesIO()
        save_kwargs: Dict[str, Any] = {}
        if fmt == "JPEG":
            save_kwargs["quality"] = quality
            save_kwargs["optimize"] = True
        im.convert("RGB").save(buf, format=fmt, **save_kwargs)
        data = buf.getvalue()
        meta["final_bytes"] = len(data)
        meta["final_mime"] = mime
        return _build_content_blocks(data, mime, json.dumps(meta, indent=2)), meta


def _run_image_tool(path: Optional[str], max_dim: int, quality: int, force_jpeg: bool, tool_name: str) -> Any:
    req_id = _new_request_id()
    target = Path(path).expanduser().resolve() if path and path.strip() else IMAGE_DEFAULT_PATH
    try:
        contents, meta = _load_image_with_optional_downscale(target, max_dim, quality, force_jpeg, req_id)
        _jsonl_log({"ts": time.time(), "tool": tool_name, "ok": True, "error": None, "request_id": req_id, "path": str(target), "meta": meta})
        return contents
    except Exception as exc:
        res = _sots_err(str(exc), meta={"request_id": req_id, "path": str(target)})
        _jsonl_log({"ts": time.time(), "tool": tool_name, "ok": res.get("ok", False), "error": res.get("error"), "request_id": req_id, "path": str(target)})
        return res

def _search_files(
    query: str,
    root_key: str = "Plugins",
    exts: Optional[List[str]] = None,
    regex: bool = False,
    case_sensitive: bool = False,
    max_results: int = 200,
    max_file_bytes: int = 2_000_000,
) -> Dict[str, Any]:
    if root_key not in SAFE_SEARCH_ROOTS:
        return {"ok": False, "error": f"Invalid root_key. Allowed: {sorted(SAFE_SEARCH_ROOTS.keys())}"}

    root = SAFE_SEARCH_ROOTS[root_key]
    if not root.exists():
        return {"ok": False, "error": f"Root not found: {root_key} -> {root}"}

    exts = exts or DEFAULT_EXTS
    rx = _compile_pattern(query, regex=regex, case_sensitive=case_sensitive)

    results: List[Dict[str, Any]] = []
    files_scanned = 0

    for p in root.rglob("*"):
        if not p.is_file():
            continue
        if p.suffix.lower() not in set([e.lower() for e in exts]):
            continue

        try:
            st = p.stat()
            if st.st_size > max_file_bytes:
                continue
        except Exception:
            continue

        files_scanned += 1

        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except Exception:
            continue

        for m in rx.finditer(text):
            if len(results) >= max_results:
                break

            # line/col
            start = m.start()
            line = text.count("\n", 0, start) + 1
            last_nl = text.rfind("\n", 0, start)
            col = (start - (last_nl + 1)) + 1

            # snippet window
            snip_start = max(0, start - 80)
            snip_end = min(len(text), m.end() + 80)
            snippet = text[snip_start:snip_end].replace("\r", "")

            results.append({
                "file": _safe_relpath(p, PATHS.project_root),
                "line": line,
                "col": col,
                "match": m.group(0)[:200],
                "snippet": snippet[:500],
            })

        if len(results) >= max_results:
            break

    return {
        "ok": True,
        "root": root_key,
        "query": query,
        "regex": regex,
        "case_sensitive": case_sensitive,
        "files_scanned": files_scanned,
        "results": results,
        "truncated": len(results) >= max_results,
    }

# -----------------------------------------------------------------------------
# MCP server definition (FastMCP)
# -----------------------------------------------------------------------------

# NOTE: `mcp` is defined near the top of the file so that tool decorators can
# register correctly. Do not reassign it here.

@mcp.tool(
    name="sots_run_devtool",
    description="Run an allowlisted SOTS DevTools Python script and return stdout/stderr. Read-only.",
    annotations={"readOnlyHint": True, "title": "Run SOTS DevTool (allowlisted)"},
)
def run_devtool(name: str, args: Optional[List[str]] = None) -> Dict[str, Any]:
    """
    name: depmap | tag_usage | todo_backlog | api_surface | plugin_discovery | sots_tools
    args: forwarded CLI args (validated only as a list of strings)
    """
    req_id = _new_request_id()
    args = args or []

    if name not in ALLOWED_SCRIPTS:
        res = _sots_err(f"Devtool not allowed: {name}. Allowed: {sorted(ALLOWED_SCRIPTS.keys())}", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "sots_run_devtool", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res

    read_only_meta = ALLOWED_SCRIPTS_META.get(name, {})
    is_read_only = bool(read_only_meta.get("read_only"))

    if not _is_allowed_devtools_run() and not is_read_only:
        res = _sots_err("Blocked by SOTS_ALLOW_DEVTOOLS_RUN=0", meta={"request_id": req_id, "read_only": is_read_only})
        _jsonl_log({"ts": time.time(), "tool": "sots_run_devtool", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res

    script = ALLOWED_SCRIPTS[name]
    out = _run_python(script, args)
    res = _sots_ok(out, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "sots_run_devtool", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res

@mcp.tool(
    name="sots_list_dir",
    description="List directory entries under an allowed root (repo|exports). Read-only.",
    annotations={"readOnlyHint": True, "title": "List dir (repo/exports)"},
)
def sots_list_dir(path: str = ".", root: str = "repo") -> Dict[str, Any]:
    req_id = _new_request_id()
    try:
        base = _get_root(root)
        target = (base / path).resolve()
        _ensure_under_root(target, base)
        if not target.exists() or not target.is_dir():
            res = _sots_err(f"Not a directory: {path}", meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "sots_list_dir", "ok": res["ok"], "error": res["error"], "request_id": req_id})
            return res
        entries: List[Dict[str, Any]] = []
        for child in sorted(target.iterdir()):
            try:
                st = child.stat()
                entries.append({
                    "name": child.name,
                    "is_dir": child.is_dir(),
                    "bytes": st.st_size,
                    "mtime_epoch": int(st.st_mtime),
                })
            except Exception:
                continue
        res = _sots_ok({"entries": entries}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "sots_list_dir", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res
    except Exception as exc:
        _get_server_logger().error("sots_list_dir failed: %s", exc)
        res = _sots_err(str(exc), meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "sots_list_dir", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res


@mcp.tool(
    name="sots_read_file",
    description="Read a text file under an allowed root (repo|exports). Read-only.",
    annotations={"readOnlyHint": True, "title": "Read file (repo/exports)"},
)
def sots_read_file(path: str, start_line: int = 1, max_lines: int = 200, root: str = "repo") -> Dict[str, Any]:
    req_id = _new_request_id()
    try:
        base = _get_root(root)
        target = (base / path).resolve()
        _ensure_under_root(target, base)
        if not target.exists() or not target.is_file():
            res = _sots_err(f"File not found: {path}", meta={"request_id": req_id})
            _jsonl_log({"ts": time.time(), "tool": "sots_read_file", "ok": res["ok"], "error": res["error"], "request_id": req_id})
            return res
        data = target.read_text(encoding="utf-8", errors="replace").splitlines()
        start = max(0, start_line - 1)
        end = start + max(0, max_lines)
        lines = data[start:end]
        res = _sots_ok({"text": "\n".join(lines)}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "sots_read_file", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res
    except Exception as exc:
        _get_server_logger().error("sots_read_file failed: %s", exc)
        res = _sots_err(str(exc), meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "sots_read_file", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res


@mcp.tool(
    name="sots_read_image",
    description="Return an image content block from DevTools/python/SOTS_Capture (VisualDigest-safe) with optional downscale and JPEG re-encode. Read-only.",
    annotations={"readOnlyHint": True, "title": "Read image (VisualDigest)"},
)
def sots_read_image(path: str = "", max_dim: int = IMAGE_DEFAULT_MAX_DIM, quality: int = IMAGE_DEFAULT_QUALITY, force_jpeg: bool = True) -> Any:
    return _run_image_tool(path, max_dim, quality, force_jpeg, tool_name="sots_read_image")


@mcp.tool(
    name="sots_latest_digest_image",
    description="Convenience: return the VisualDigest latest image as an IMAGE content block (downscaled/encoded). Read-only.",
    annotations={"readOnlyHint": True, "title": "Read latest digest image"},
)
def sots_latest_digest_image(max_dim: int = IMAGE_DEFAULT_MAX_DIM, quality: int = IMAGE_DEFAULT_QUALITY, force_jpeg: bool = True) -> Any:
    return _run_image_tool(str(IMAGE_DEFAULT_PATH), max_dim, quality, force_jpeg, tool_name="sots_latest_digest_image")


@mcp.tool(
    name="sots_search_workspace",
    description="Search across the repo workspace (allowed roots) using the existing search helper. Read-only.",
    annotations={"readOnlyHint": True, "title": "Search workspace"},
)
def sots_search_workspace(
    query: str,
    exts: Optional[List[str]] = None,
    regex: bool = False,
    case_sensitive: bool = False,
    max_results: int = 200,
    root_key: str = "Repo",
) -> Dict[str, Any]:
    req_id = _new_request_id()
    res = _sots_ok(
        _search_files(
            query=query,
            root_key=root_key,
            exts=exts,
            regex=regex,
            case_sensitive=case_sensitive,
            max_results=max_results,
        ),
        meta={"request_id": req_id},
    )
    _jsonl_log({"ts": time.time(), "tool": "sots_search_workspace", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="sots_search_exports",
    description="Search across BEP exports (exports root). Read-only.",
    annotations={"readOnlyHint": True, "title": "Search exports"},
)
def sots_search_exports(
    query: str,
    exts: Optional[List[str]] = None,
    regex: bool = False,
    case_sensitive: bool = False,
    max_results: int = 200,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    res = _sots_ok(
        _search_files(
            query=query,
            root_key="Exports",
            exts=exts,
            regex=regex,
            case_sensitive=case_sensitive,
            max_results=max_results,
        ),
        meta={"request_id": req_id},
    )
    _jsonl_log({"ts": time.time(), "tool": "sots_search_exports", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="sots_agents_health",
    description="Check availability of SOTS Agents runner and API key.",
    annotations={"readOnlyHint": True, "title": "Agents: health"},
)
def sots_agents_health() -> Dict[str, Any]:
    req_id = _new_request_id()
    agents_importable = True
    notes: List[str] = []
    default_model = os.environ.get("SOTS_MODEL_CODE") or os.environ.get("SOTS_MODEL_DEVTOOLS") or os.environ.get("SOTS_MODEL_EDITOR") or ""
    try:
        import agents as _  # type: ignore  # noqa: F401
    except Exception as exc:
        agents_importable = False
        notes.append(f"import failed: {exc}")

    has_api_key = bool(os.environ.get("OPENAI_API_KEY"))
    ok = agents_importable and has_api_key

    _get_server_logger().info(
        "[agents_health] ok=%s importable=%s api_key=%s default_model=%s",
        ok,
        agents_importable,
        has_api_key,
        default_model or "unknown",
    )

    res = _sots_ok(
        {
            "agents_importable": agents_importable,
            "has_api_key": has_api_key,
            "default_model": default_model,
            "notes": notes,
        },
        meta={"request_id": req_id},
    )
    _jsonl_log({"ts": time.time(), "tool": "sots_agents_health", "ok": ok, "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="sots_agents_run",
    description="Run a prompt through the SOTS Agents runner (read-only).",
    annotations={"readOnlyHint": True, "title": "Agents: run (safe)"},
)
def sots_agents_run(
    prompt: str,
    system: str = "",
    model: str = "",
    reasoning_effort: str = "high",
) -> Dict[str, Any]:
    req_id = _new_request_id()
    prompt = (prompt or "").strip()
    if not prompt:
        res = _sots_err("prompt is empty", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "sots_agents_run", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res

    payload = {
        "prompt": prompt,
        "system": system or "",
        "model": model or "",
        "reasoning_effort": reasoning_effort or "",
        "mode": "plan",
        "session_id": "sots_mcp_inline",
        "session_db": str(PATHS.devtools_py / "sots_agents_sessions.db"),
        "repo": str(PATHS.project_root),
    }

    if not os.environ.get("OPENAI_API_KEY"):
        res = _sots_err("OPENAI_API_KEY is missing", meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "sots_agents_run", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res

    try:
        res = sots_agents_run_task(payload)
        meta = res.get("meta") or {}
        _get_server_logger().info(
            "[agents_run] ok=%s lane=%s mode=%s",
            res.get("ok"),
            meta.get("lane"),
            meta.get("mode"),
        )
        out = _sots_ok(res, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "sots_agents_run", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    except Exception as exc:
        _get_server_logger().exception("sots_agents_run failed")
        res = _sots_err(str(exc), meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "sots_agents_run", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res


@mcp.tool(
    name="sots_agents_help",
    description="Describe available Agents tools and parameters.",
    annotations={"readOnlyHint": True, "title": "Agents: help"},
)
def sots_agents_help() -> Dict[str, Any]:
    req_id = _new_request_id()
    res = _sots_ok(
        {
            "tools": {
                "sots_agents_health": {"params": [], "notes": "Checks import + OPENAI_API_KEY + default model."},
                "sots_agents_run": {
                    "params": ["prompt (required)", "system", "model", "reasoning_effort"],
                    "notes": "Runs via sots_agents_runner in plan/read-only mode.",
                },
                "agents_run_prompt": {"alias_for": "sots_agents_run"},
            }
        },
        meta={"request_id": req_id},
    )
    _jsonl_log({"ts": time.time(), "tool": "sots_agents_help", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="agents_run_prompt",
    description="Run a SOTS prompt through the safeguarded orchestrator (APPLY gated by SOTS_ALLOW_APPLY).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "Agents: run prompt"},
)
def agents_run_prompt(prompt: str, mode: Optional[str] = None, lane: Optional[str] = None) -> Dict[str, Any]:
    req_id = _new_request_id()
    prompt = (prompt or "").strip()
    if not prompt:
        res = _sots_err("prompt is empty", meta={"allow_apply": ALLOW_APPLY, "request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "agents_run_prompt", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res

    try:
        orch = _get_agents_orchestrator()
        res = orch.run_one(prompt, mode_override=mode, lane_override=lane)
        res["allow_apply"] = ALLOW_APPLY
        out = _sots_ok(res, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "agents_run_prompt", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    except Exception as exc:
        res = _sots_err(f"orchestrator failure: {exc}", meta={"allow_apply": ALLOW_APPLY, "request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "agents_run_prompt", "ok": res["ok"], "error": res["error"], "request_id": req_id})
        return res


@mcp.tool(
    name="agents_server_info",
    description="Return orchestrator config for agents tools.",
    annotations={"readOnlyHint": True, "title": "Agents: server info"},
)
def agents_server_info() -> Dict[str, Any]:
    info = _agents_info()
    info["allow_apply"] = ALLOW_APPLY
    req_id = _new_request_id()
    res = _sots_ok({"agents": info}, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "agents_server_info", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="bpgen_call",
    description="Forward an action to the BPGen bridge (mutations gated by SOTS_ALLOW_APPLY).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: call"},
)
def bpgen_call_tool(action: str, params: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    req_id = _new_request_id()
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_call", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    res = _bpgen_call(action, params)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_call", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_ping",
    description="Ping the BPGen bridge via TCP.",
    annotations={"readOnlyHint": True, "title": "BPGen: ping"},
)
def bpgen_ping_tool() -> Dict[str, Any]:
    req_id = _new_request_id()
    res = _sots_ok(_bpgen_call("ping", {}), meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_ping", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="bpgen_discover",
    description="Discover BPGen nodes.",
    annotations={"readOnlyHint": True, "title": "BPGen: discover"},
)
def bpgen_discover(
    blueprint_asset_path: str = "",
    search_text: str = "",
    max_results: int = 200,
    include_pins: bool = False,
) -> Dict[str, Any]:
    action = "discover_nodes"
    guard = _bpgen_guard_mutation(action)
    if guard:
        return guard
    return _bpgen_call(
        action,
        {
            "blueprint_asset_path": blueprint_asset_path,
            "search_text": search_text,
            "max_results": max_results,
            "include_pins": include_pins,
        },
    )


@mcp.tool(
    name="bpgen_list",
    description="List BPGen nodes for a function.",
    annotations={"readOnlyHint": True, "title": "BPGen: list"},
)
def bpgen_list(blueprint_asset_path: str, function_name: str, include_pins: bool = False) -> Dict[str, Any]:
    action = "list_nodes"
    guard = _bpgen_guard_mutation(action)
    if guard:
        return guard
    return _bpgen_call(
        action,
        {
            "blueprint_asset_path": blueprint_asset_path,
            "function_name": function_name,
            "include_pins": include_pins,
        },
    )


@mcp.tool(
    name="bpgen_describe",
    description="Describe a BPGen node.",
    annotations={"readOnlyHint": True, "title": "BPGen: describe"},
)
def bpgen_describe(blueprint_asset_path: str, function_name: str, node_id: str, include_pins: bool = True) -> Dict[str, Any]:
    action = "describe_node"
    guard = _bpgen_guard_mutation(action)
    if guard:
        return guard
    return _bpgen_call(
        action,
        {
            "blueprint_asset_path": blueprint_asset_path,
            "function_name": function_name,
            "node_id": node_id,
            "include_pins": include_pins,
        },
    )


@mcp.tool(
    name="bpgen_refresh",
    description="Refresh BPGen nodes.",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: refresh"},
)
def bpgen_refresh(blueprint_asset_path: str, function_name: str, include_pins: bool = False) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "refresh_nodes"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_refresh", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    res = _bpgen_call(
        action,
        {
            "blueprint_asset_path": blueprint_asset_path,
            "function_name": function_name,
            "include_pins": include_pins,
        },
    )
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_refresh", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_compile",
    description="Compile a Blueprint.",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: compile"},
)
def bpgen_compile(blueprint_asset_path: str) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "compile_blueprint"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_compile", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    res = _bpgen_call(action, {"blueprint_asset_path": blueprint_asset_path})
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_compile", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_save",
    description="Save a Blueprint.",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: save"},
)
def bpgen_save(blueprint_asset_path: str) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "save_blueprint"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_save", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    res = _bpgen_call(action, {"blueprint_asset_path": blueprint_asset_path})
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_save", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_apply",
    description="Apply a BPGen graph spec (mutating; gated by SOTS_ALLOW_APPLY). Supports dry_run.",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: apply"},
)
def bpgen_apply(blueprint_asset_path: str, function_name: str, graph_spec: Dict[str, Any], dry_run: bool = False) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "apply_graph_spec"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_apply", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    payload = {
        "blueprint_asset_path": blueprint_asset_path,
        "function_name": function_name,
        "graph_spec": graph_spec,
    }
    if dry_run:
        out = _sots_ok({"action": action, "params": payload, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_apply", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    res = _bpgen_call(action, payload)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_apply", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_apply_to_target",
    description="Apply a BPGen graph spec to a non-function target (mutating; gated). Supports dry_run.",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: apply to target"},
)
def bpgen_apply_to_target(graph_spec: Dict[str, Any], dry_run: bool = False) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "apply_graph_spec_to_target"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_apply_to_target", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    payload = {"graph_spec": graph_spec}
    if dry_run:
        out = _sots_ok({"action": action, "params": payload, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_apply_to_target", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    res = _bpgen_call(action, payload)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_apply_to_target", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_create_blueprint",
    description="Create a Blueprint asset (mutating; gated by SOTS_ALLOW_APPLY).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: create blueprint"},
)
def bpgen_create_blueprint(asset_path: str, parent_class_path: Optional[str] = None) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "create_blueprint_asset"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_create_blueprint", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard

    params: Dict[str, Any] = {
        "asset_path": asset_path,
        "AssetPath": asset_path,
    }
    if parent_class_path:
        params.update(
            {
                "parent_class_path": parent_class_path,
                "ParentClassPath": parent_class_path,
            }
        )
    res = _bpgen_call(action, params)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_create_blueprint", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_delete_node",
    description="Delete a node by id (mutating; gated).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: delete node"},
)
def bpgen_delete_node(blueprint_asset_path: str, function_name: str, node_id: str, compile: bool = True, dry_run: bool = False) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "delete_node_by_id"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_delete_node", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    payload = {
        "blueprint_asset_path": blueprint_asset_path,
        "function_name": function_name,
        "node_id": node_id,
        "compile": compile,
        "dry_run": dry_run,
    }
    if dry_run:
        out = _sots_ok({"action": action, "params": payload, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_delete_node", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    res = _bpgen_call(action, payload)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_delete_node", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_delete_link",
    description="Delete a link between nodes (mutating; gated).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: delete link"},
)
def bpgen_delete_link(
    blueprint_asset_path: str,
    function_name: str,
    from_node_id: str,
    from_pin: str,
    to_node_id: str,
    to_pin: str,
    compile: bool = True,
    dry_run: bool = False,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "delete_link"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_delete_link", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    payload = {
        "blueprint_asset_path": blueprint_asset_path,
        "function_name": function_name,
        "from_node_id": from_node_id,
        "from_pin": from_pin,
        "to_node_id": to_node_id,
        "to_pin": to_pin,
        "compile": compile,
        "dry_run": dry_run,
    }
    if dry_run:
        out = _sots_ok({"action": action, "params": payload, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_delete_link", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    res = _bpgen_call(action, payload)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_delete_link", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_replace_node",
    description="Replace a node (mutating; gated).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: replace node"},
)
def bpgen_replace_node(
    blueprint_asset_path: str,
    function_name: str,
    existing_node_id: str,
    new_node: Dict[str, Any],
    pin_remap: Optional[Dict[str, str]] = None,
    compile: bool = True,
    dry_run: bool = False,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "replace_node"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_replace_node", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    params: Dict[str, Any] = {
        "blueprint_asset_path": blueprint_asset_path,
        "function_name": function_name,
        "existing_node_id": existing_node_id,
        "new_node": new_node,
        "compile": compile,
        "dry_run": dry_run,
    }
    if pin_remap:
        params["pin_remap"] = pin_remap
    if dry_run:
        out = _sots_ok({"action": action, "params": params, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_replace_node", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    res = _bpgen_call(action, params)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_replace_node", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_ensure_function",
    description="Ensure a function exists (mutating; gated). Supports dry_run.",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: ensure function"},
)
def bpgen_ensure_function(blueprint_asset_path: str, function_name: str, signature: Optional[Dict[str, Any]] = None, create_if_missing: bool = True, update_if_exists: bool = True, dry_run: bool = False) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "ensure_function"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_function", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    payload = {
        "blueprint_asset_path": blueprint_asset_path,
        "function_name": function_name,
        "signature": signature or {},
        "create_if_missing": create_if_missing,
        "update_if_exists": update_if_exists,
    }
    if dry_run:
        out = _sots_ok({"action": action, "params": payload, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_function", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    res = _bpgen_call(action, payload)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_function", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_ensure_variable",
    description="Ensure a variable exists (mutating; gated).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: ensure variable"},
)
def bpgen_ensure_variable(
    blueprint_asset_path: str,
    variable_name: str,
    pin_type: Dict[str, Any],
    default_value: Optional[Any] = None,
    create_if_missing: bool = True,
    update_if_exists: bool = True,
    instance_editable: bool = True,
    expose_on_spawn: bool = False,
    dry_run: bool = False,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "ensure_variable"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_variable", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    payload = {
        "blueprint_asset_path": blueprint_asset_path,
        "variable_name": variable_name,
        "pin_type": pin_type,
        "default_value": default_value,
        "create_if_missing": create_if_missing,
        "update_if_exists": update_if_exists,
        "instance_editable": instance_editable,
        "expose_on_spawn": expose_on_spawn,
    }
    if dry_run:
        out = _sots_ok({"action": action, "params": payload, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_variable", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out
    res = _bpgen_call(action, payload)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_variable", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_ensure_widget",
    description="Ensure a widget component exists in a WidgetBlueprint (mutating; gated).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: ensure widget"},
)
def bpgen_ensure_widget(
    blueprint_asset_path: str,
    widget_class_path: str,
    widget_name: str,
    parent_name: str = "",
    insert_index: int = -1,
    is_variable: bool = True,
    create_if_missing: bool = True,
    update_if_exists: bool = True,
    reparent_if_mismatch: bool = True,
    properties: Optional[Dict[str, str]] = None,
    slot_properties: Optional[Dict[str, str]] = None,
    dry_run: bool = False,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "ensure_widget"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_widget", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard

    payload: Dict[str, Any] = {
        "blueprint_asset_path": blueprint_asset_path,
        "widget_class_path": widget_class_path,
        "widget_name": widget_name,
        "parent_name": parent_name,
        "insert_index": insert_index,
        "is_variable": is_variable,
        "create_if_missing": create_if_missing,
        "update_if_exists": update_if_exists,
        "reparent_if_mismatch": reparent_if_mismatch,
        "properties": properties or {},
        "slot_properties": slot_properties or {},
    }
    if dry_run:
        out = _sots_ok({"action": action, "params": payload, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_widget", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out

    res = _bpgen_call(action, payload)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_widget", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_set_widget_properties",
    description="Set widget properties by ImportText paths (mutating; gated).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: set widget properties"},
)
def bpgen_set_widget_properties(
    blueprint_asset_path: str,
    widget_name: str,
    properties: Dict[str, str],
    fail_if_missing: bool = True,
    dry_run: bool = False,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "set_widget_properties"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_set_widget_properties", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard

    payload: Dict[str, Any] = {
        "blueprint_asset_path": blueprint_asset_path,
        "widget_name": widget_name,
        "properties": properties,
        "fail_if_missing": fail_if_missing,
    }
    if dry_run:
        out = _sots_ok({"action": action, "params": payload, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_set_widget_properties", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out

    res = _bpgen_call(action, payload)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_set_widget_properties", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_ensure_binding",
    description="Ensure a widget property binding exists (mutating; gated).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: ensure binding"},
)
def bpgen_ensure_binding(
    blueprint_asset_path: str,
    widget_name: str,
    property_name: str,
    function_name: str,
    signature: Optional[Dict[str, Any]] = None,
    graph_spec: Optional[Dict[str, Any]] = None,
    apply_graph: bool = False,
    create_binding: bool = True,
    update_binding: bool = True,
    create_function: bool = True,
    update_function: bool = True,
    dry_run: bool = False,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "ensure_binding"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_binding", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard

    payload: Dict[str, Any] = {
        "blueprint_asset_path": blueprint_asset_path,
        "widget_name": widget_name,
        "property_name": property_name,
        "function_name": function_name,
        "signature": signature or {},
        "graph_spec": graph_spec or {},
        "apply_graph": apply_graph,
        "create_binding": create_binding,
        "update_binding": update_binding,
        "create_function": create_function,
        "update_function": update_function,
    }
    if dry_run:
        out = _sots_ok({"action": action, "params": payload, "dry_run": True}, meta={"request_id": req_id})
        _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_binding", "ok": out["ok"], "error": out["error"], "request_id": req_id})
        return out

    res = _bpgen_call(action, payload)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_ensure_binding", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_get_spec_schema",
    description="Get BPGen spec schema (read-only).",
    annotations={"readOnlyHint": True, "title": "BPGen: spec schema"},
)
def bpgen_get_spec_schema() -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "get_spec_schema"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_get_spec_schema", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    res = _bpgen_call(action, {})
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_get_spec_schema", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="bpgen_canonicalize_spec",
    description="Canonicalize a BPGen graph spec (read-only).",
    annotations={"readOnlyHint": True, "title": "BPGen: canonicalize"},
)
def bpgen_canonicalize_spec(graph_spec: Dict[str, Any], options: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    req_id = _new_request_id()
    action = "canonicalize_spec"
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "bpgen_canonicalize_spec", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    params: Dict[str, Any] = {"graph_spec": graph_spec}
    if options:
        params["options"] = options
    res = _bpgen_call(action, params)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "bpgen_canonicalize_spec", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="manage_bpgen",
    description="Generic BPGen action forwarder (mutations gated).",
    annotations={"readOnlyHint": not ALLOW_APPLY, "title": "BPGen: manage"},
)
def manage_bpgen(action: str, params: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    req_id = _new_request_id()
    guard = _bpgen_guard_mutation(action)
    if guard:
        guard["meta"]["request_id"] = req_id
        _jsonl_log({"ts": time.time(), "tool": "manage_bpgen", "ok": guard["ok"], "error": guard["error"], "request_id": req_id})
        return guard
    res = _bpgen_call(action, params)
    out = _sots_ok(res, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "manage_bpgen", "ok": out["ok"], "error": out["error"], "request_id": req_id})
    return out


@mcp.tool(
    name="sots_list_reports",
    description="List files under DevTools/reports. Read-only.",
    annotations={"readOnlyHint": True, "title": "List SOTS Reports"},
)
def list_reports(max_items: int = 200) -> Dict[str, Any]:
    req_id = _new_request_id()
    res = _sots_ok({"reports": _list_reports(max_items=max_items)}, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "sots_list_reports", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res

@mcp.tool(
    name="sots_read_report",
    description="Read a text report under DevTools/reports (UTF-8). Read-only.",
    annotations={"readOnlyHint": True, "title": "Read SOTS Report"},
)
def read_report(path: str, max_chars: int = 20000) -> Dict[str, Any]:
    # path must be relative to project root, and must live under DevTools/reports/*
    req_id = _new_request_id()
    res = _sots_ok(_read_text_file(path, max_chars=max_chars), meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "sots_read_report", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res

@mcp.tool(
    name="sots_search",
    description="Search for text across a safe subtree (Plugins/DevTools/Source/Config/Content). Read-only.",
    annotations={"readOnlyHint": True, "title": "Search SOTS Workspace"},
)
def search(
    query: str,
    root_key: str = "Plugins",
    exts: Optional[List[str]] = None,
    regex: bool = False,
    case_sensitive: bool = False,
    max_results: int = 200,
) -> Dict[str, Any]:
    req_id = _new_request_id()
    res = _sots_ok(
        _search_files(
            query=query,
            root_key=root_key,
            exts=exts,
            regex=regex,
            case_sensitive=case_sensitive,
            max_results=max_results,
        ),
        meta={"request_id": req_id},
    )
    _jsonl_log({"ts": time.time(), "tool": "sots_search", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res

@mcp.tool(
    name="sots_server_info",
    description="Return basic server info, roots, and allowlists. Read-only.",
    annotations={"readOnlyHint": True, "title": "SOTS MCP Server Info"},
)
def server_info() -> Dict[str, Any]:
    req_id = _new_request_id()
    res = _sots_ok(
        {
            "project_root": str(PATHS.project_root),
            "devtools_py": str(PATHS.devtools_py),
            "reports_dir": str(PATHS.reports_dir),
            "roots": {"repo": str(PATHS.project_root), "exports": str(EXPORTS_ROOT)},
            "allowed_devtools": sorted(ALLOWED_SCRIPTS.keys()),
            "safe_search_roots": {k: str(v) for k, v in SAFE_SEARCH_ROOTS.items()},
            "default_exts": DEFAULT_EXTS,
            "allow_apply": ALLOW_APPLY,
            "gates": {
                "SOTS_ALLOW_APPLY": {"value": ALLOW_APPLY, "note": "Mutating BPGen actions blocked when false."},
                "SOTS_ALLOW_BPGEN_APPLY": {"value": ALLOW_BPGEN_APPLY, "note": "BPGen mutations gated."},
                "SOTS_ALLOW_DEVTOOLS_RUN": {"value": ALLOW_DEVTOOLS_RUN, "note": "Devtools runs gated."},
            },
            "tool_aliases": TOOL_ALIASES,
            "agents": _agents_info(),
            "bpgen": {**BPGEN_CFG, "allow_apply": ALLOW_APPLY, "read_only_actions": sorted(BPGEN_READ_ONLY_ACTIONS)},
            "notes": [
                "APPLY is gated via SOTS_ALLOW_APPLY env var.",
                "Avoid stdout logging; tool results carry stdout/stderr from invoked scripts.",
            ],
        },
        meta={"request_id": req_id},
    )
    _jsonl_log({"ts": time.time(), "tool": "sots_server_info", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="sots_help",
    description="List canonical tools, aliases, gates, and roots.",
    annotations={"readOnlyHint": True, "title": "SOTS Help/Index"},
)
def sots_help() -> Dict[str, Any]:
    req_id = _new_request_id()
    canonical_tools = [
        # VibeUE upstream compatibility (initial scaffold)
        "manage_asset",
        "manage_blueprint",
        "manage_blueprint_component",
        "manage_blueprint_node",
        "manage_blueprint_function",
        "manage_blueprint_variable",
        "manage_umg_widget",
        "manage_enhanced_input",
        "manage_level_actors",
        "manage_material",
        "manage_material_node",
        "check_unreal_connection",
        "get_help",
        "sots_list_dir",
        "sots_read_file",
        "sots_search_workspace",
        "sots_search_exports",
        "sots_search",
        "sots_read_image",
        "sots_latest_digest_image",
        "sots_list_reports",
        "sots_read_report",
        "sots_run_devtool",
        "sots_agents_health",
        "sots_agents_run",
        "sots_agents_help",
        "agents_run_prompt",
        "agents_server_info",
        "bpgen_ping",
        "bpgen_call",
        "manage_bpgen",
        "bpgen_discover",
        "bpgen_list",
        "bpgen_describe",
        "bpgen_refresh",
        "bpgen_compile",
        "bpgen_save",
        "bpgen_apply",
        "bpgen_apply_to_target",
        "bpgen_delete_node",
        "bpgen_delete_link",
        "bpgen_replace_node",
        "bpgen_ensure_function",
        "bpgen_ensure_variable",
        "bpgen_ensure_widget",
        "bpgen_set_widget_properties",
        "bpgen_ensure_binding",
        "bpgen_get_spec_schema",
        "bpgen_canonicalize_spec",
        "sots_server_info",
        "sots_help",
        "sots_env_dump_safe",
        "sots_where_am_i",
        "sots_smoketest_all",
        "sots_last_error",
    ]
    res = _sots_ok(
        {
            "tools": canonical_tools,
            "aliases": TOOL_ALIASES,
            "gates": {
                "SOTS_ALLOW_APPLY": ALLOW_APPLY,
                "SOTS_ALLOW_BPGEN_APPLY": ALLOW_BPGEN_APPLY,
                "SOTS_ALLOW_DEVTOOLS_RUN": ALLOW_DEVTOOLS_RUN,
            },
            "roots": {"repo": str(PATHS.project_root), "exports": str(EXPORTS_ROOT)},
        },
        meta={"request_id": req_id},
    )
    _jsonl_log({"ts": time.time(), "tool": "sots_help", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="sots_last_error",
    description="Return the last error entry from the JSONL log (bounded scan).",
    annotations={"readOnlyHint": True, "title": "SOTS Last Error"},
)
def sots_last_error() -> Dict[str, Any]:
    req_id = _new_request_id()
    log_path = PATHS.devtools_py / "logs" / "sots_mcp_server.jsonl"
    if not log_path.exists():
        res = _sots_err("No log file found", meta={"request_id": req_id})
        return res
    last_err = None
    try:
        with log_path.open("r", encoding="utf-8") as f:
            dq = deque(f, maxlen=500)
        for line in reversed(dq):
            try:
                obj = json.loads(line)
            except Exception:
                continue
            if obj.get("ok") is False:
                last_err = obj
                break
        if last_err is None:
            res = _sots_err("No error entries found", meta={"request_id": req_id})
        else:
            res = _sots_ok({"last_error": last_err}, meta={"request_id": req_id})
    except Exception as exc:
        res = _sots_err(str(exc), meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "sots_last_error", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


def _safe_git_branch() -> str:
    head = PATHS.project_root / ".git" / "HEAD"
    if not head.exists():
        return "unknown"
    try:
        text = head.read_text(encoding="utf-8", errors="replace").strip()
        if text.startswith("ref:"):
            return text.split("/")[-1]
        return text[:32] or "unknown"
    except Exception:
        return "unknown"


@mcp.tool(
    name="sots_env_dump_safe",
    description="Return presence booleans for key env vars (no values).",
    annotations={"readOnlyHint": True, "title": "SOTS Env Dump (safe)"},
)
def sots_env_dump_safe() -> Dict[str, Any]:
    req_id = _new_request_id()
    keys = [
        "OPENAI_API_KEY",
        "SOTS_BPGEN_HOST",
        "SOTS_BPGEN_PORT",
        "SOTS_ALLOW_APPLY",
        "SOTS_ALLOW_BPGEN_APPLY",
        "SOTS_ALLOW_DEVTOOLS_RUN",
    ]
    presence = {k: (os.environ.get(k) is not None) for k in keys}
    res = _sots_ok({"env_present": presence}, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "sots_env_dump_safe", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="sots_where_am_i",
    description="Return repo locations and basic context (read-only).",
    annotations={"readOnlyHint": True, "title": "SOTS Where Am I"},
)
def sots_where_am_i() -> Dict[str, Any]:
    req_id = _new_request_id()
    data = {
        "repo_root": str(PATHS.project_root),
        "exports_root": str(EXPORTS_ROOT),
        "branch": _safe_git_branch(),
        "paths": {
            "DevTools": (PATHS.project_root / "DevTools").exists(),
            "Plugins": (PATHS.project_root / "Plugins").exists(),
            "Source": (PATHS.project_root / "Source").exists(),
            "Config": (PATHS.project_root / "Config").exists(),
            "Exports": EXPORTS_ROOT.exists(),
        },
    }
    res = _sots_ok(data, meta={"request_id": req_id})
    _jsonl_log({"ts": time.time(), "tool": "sots_where_am_i", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res


@mcp.tool(
    name="sots_smoketest_all",
    description="Run safe smoketests (server_info, agents_health, bpgen_ping) without mutations.",
    annotations={"readOnlyHint": True, "title": "SOTS Smoketest All"},
)
def sots_smoketest_all() -> Dict[str, Any]:
    req_id = _new_request_id()
    results: Dict[str, Any] = {}
    warnings: List[str] = []

    try:
        results["server_info"] = server_info()
    except Exception as exc:
        warnings.append(f"server_info failed: {exc}")
        results["server_info"] = _sots_err(str(exc))

    try:
        results["agents_health"] = sots_agents_health()
    except Exception as exc:
        warnings.append(f"sots_agents_health failed: {exc}")
        results["agents_health"] = _sots_err(str(exc))

    try:
        results["bpgen_ping"] = bpgen_ping_tool()
    except Exception as exc:
        warnings.append(f"bpgen_ping failed: {exc}")
        results["bpgen_ping"] = _sots_err(str(exc))

    ok = all(r.get("ok") for r in results.values() if isinstance(r, dict))
    res = _sots_ok({"results": results}, warnings=warnings, meta={"request_id": req_id})
    res["ok"] = ok
    _jsonl_log({"ts": time.time(), "tool": "sots_smoketest_all", "ok": res["ok"], "error": res["error"], "request_id": req_id})
    return res

def main() -> None:
    # Stdio transport is what VS Code uses for local MCP subprocesses.
    # FastMCP defaults to stdio when transport isn't specified in many examples,
    # but we set it explicitly for clarity.
    mcp.run(transport="stdio")

if __name__ == "__main__":
    main()