"""
SOTS MCP Server (read-only)
- Exposes safe, allowlisted DevTools entrypoints + scoped search + report reading
- Designed for VS Code MCP (stdio). Do NOT print to stdout (breaks JSON-RPC).
- Buddy edits files; this server only reads / runs allowlisted scripts.
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple

from mcp.server.fastmcp import FastMCP  # official MCP Python SDK

# -----------------------------------------------------------------------------
# IMPORTANT for stdio MCP:
# Never write to stdout (print). Use stderr for logs.
# -----------------------------------------------------------------------------

def _elog(msg: str) -> None:
    sys.stderr.write(msg.rstrip() + "\n")
    sys.stderr.flush()

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

SAFE_SEARCH_ROOTS = {
    "Plugins": PATHS.project_root / "Plugins",
    "DevTools": PATHS.project_root / "DevTools",
    "Source": PATHS.project_root / "Source",
    "Config": PATHS.project_root / "Config",
    "Content": PATHS.project_root / "Content",
}

DEFAULT_EXTS = [".h", ".hpp", ".cpp", ".c", ".cc", ".inl", ".cs", ".uplugin", ".uproject", ".ini", ".py", ".md", ".txt", ".json"]

def _safe_relpath(p: Path, root: Path) -> str:
    try:
        return str(p.resolve().relative_to(root.resolve()))
    except Exception:
        return str(p.resolve())

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
    if not PATHS.reports_dir.exists():
        return reports

    for p in sorted(PATHS.reports_dir.rglob("*")):
        if not p.is_file():
            continue
        try:
            st = p.stat()
            reports.append({
                "path": _safe_relpath(p, PATHS.project_root),
                "bytes": st.st_size,
                "mtime_epoch": int(st.st_mtime),
            })
        except Exception:
            continue

        if len(reports) >= max_items:
            break
    return reports

def _read_text_file(rel_path: str, max_chars: int = 20000) -> Dict[str, Any]:
    p = (PATHS.project_root / rel_path).resolve()
    _ensure_under_root(p, PATHS.project_root)
    if not p.exists() or not p.is_file():
        return {"ok": False, "error": f"File not found: {rel_path}"}

    # Only allow reading from DevTools/reports by default (safest)
    reports_root = PATHS.reports_dir.resolve()
    if reports_root not in p.parents and p != reports_root:
        return {"ok": False, "error": "Read denied: only DevTools/reports/* is readable via this tool."}

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
    "SOTS DevTools (read-only)",
    # json_response=True helps some clients interpret dict outputs consistently.
    json_response=True,
)

@mcp.tool(
    name="sots.run_devtool",
    description="Run an allowlisted SOTS DevTools Python script and return stdout/stderr. Read-only.",
    annotations={"readOnlyHint": True, "title": "Run SOTS DevTool (allowlisted)"},
)
def run_devtool(name: str, args: Optional[List[str]] = None) -> Dict[str, Any]:
    """
    name: depmap | tag_usage | todo_backlog | api_surface | plugin_discovery | sots_tools
    args: forwarded CLI args (validated only as a list of strings)
    """
    args = args or []
    if name not in ALLOWED_SCRIPTS:
        return {"ok": False, "error": f"Devtool not allowed: {name}. Allowed: {sorted(ALLOWED_SCRIPTS.keys())}"}

    script = ALLOWED_SCRIPTS[name]
    return _run_python(script, args)

@mcp.tool(
    name="sots.list_reports",
    description="List files under DevTools/reports. Read-only.",
    annotations={"readOnlyHint": True, "title": "List SOTS Reports"},
)
def list_reports(max_items: int = 200) -> Dict[str, Any]:
    return {"ok": True, "reports": _list_reports(max_items=max_items)}

@mcp.tool(
    name="sots.read_report",
    description="Read a text report under DevTools/reports (UTF-8). Read-only.",
    annotations={"readOnlyHint": True, "title": "Read SOTS Report"},
)
def read_report(path: str, max_chars: int = 20000) -> Dict[str, Any]:
    # path must be relative to project root, and must live under DevTools/reports/*
    return _read_text_file(path, max_chars=max_chars)

@mcp.tool(
    name="sots.search",
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
    return _search_files(
        query=query,
        root_key=root_key,
        exts=exts,
        regex=regex,
        case_sensitive=case_sensitive,
        max_results=max_results,
    )

@mcp.tool(
    name="sots.server_info",
    description="Return basic server info, roots, and allowlists. Read-only.",
    annotations={"readOnlyHint": True, "title": "SOTS MCP Server Info"},
)
def server_info() -> Dict[str, Any]:
    return {
        "ok": True,
        "project_root": str(PATHS.project_root),
        "devtools_py": str(PATHS.devtools_py),
        "reports_dir": str(PATHS.reports_dir),
        "allowed_devtools": sorted(ALLOWED_SCRIPTS.keys()),
        "safe_search_roots": {k: str(v) for k, v in SAFE_SEARCH_ROOTS.items()},
        "default_exts": DEFAULT_EXTS,
        "notes": [
            "Read-only server: no file writes.",
            "Avoid stdout logging; tool results carry stdout/stderr from invoked scripts.",
        ],
    }

def main() -> None:
    # Stdio transport is what VS Code uses for local MCP subprocesses.
    # FastMCP defaults to stdio when transport isn't specified in many examples,
    # but we set it explicitly for clarity.
    mcp.run(transport="stdio")

if __name__ == "__main__":
    main()
