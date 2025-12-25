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

mcp = FastMCP(
    "SOTS DevTools (unified)",
    # json_response=True helps some clients interpret dict outputs consistently.
    json_response=True,
)

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
        "bpgen_delete_node",
        "bpgen_delete_link",
        "bpgen_replace_node",
        "bpgen_ensure_function",
        "bpgen_ensure_variable",
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
