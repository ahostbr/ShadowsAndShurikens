from __future__ import annotations

import argparse
import dataclasses
import json
import os
import queue
import re
import sys
import time
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple

from agents import Agent, Runner, SQLiteSession, function_tool

# MCP is optional. Keep imports guarded so the script still runs without MCP config.
try:
    from agents.mcp import create_static_tool_filter
    from agents.mcp.server import MCPServerStreamableHttp, MCPServerSse, MCPServerStdio
except Exception:  # pragma: no cover
    create_static_tool_filter = None
    MCPServerStreamableHttp = None
    MCPServerSse = None
    MCPServerStdio = None


# -----------------------------
# Config / constants
# -----------------------------

HEADER_RE = re.compile(r"^\s*\[(SOTS_[A-Z0-9_]+)\]", re.MULTILINE)

DEFAULT_BLOCKED_SUBPROCESS = {
    # Hard blocks: no UE builds / packaging / project gen
    "UnrealBuildTool",
    "RunUAT",
    "GenerateProjectFiles",
    "msbuild",
    "devenv",
    "ubt",
    "uat",
    "Build.bat",
    "Build.sh",
}

DEFAULT_ALLOWED_SUBPROCESS_PREFIXES = [
    # Optional: allow python scripts (DevTools) if explicitly enabled
    "python",
    "py",
]


@dataclasses.dataclass
class RunConfig:
    repo_root: Path
    mode: str  # plan|apply|verify|auto
    session_id: str
    session_db_path: Path
    laws_file: Optional[Path] = None
    mcp_config_path: Optional[Path] = None
    enable_subprocess: bool = False  # Off by default (matches "manual DevTools first" culture)


@dataclasses.dataclass
class McpServerConfig:
    name: str
    transport: str  # stdio|sse|streamable_http
    url: Optional[str] = None
    command: Optional[str] = None
    args: Optional[List[str]] = None
    headers: Dict[str, str] = dataclasses.field(default_factory=dict)
    allowed_tools_plan: Optional[List[str]] = None
    allowed_tools_apply: Optional[List[str]] = None


# -----------------------------
# Local file tools
# -----------------------------

def _safe_rel(repo_root: Path, p: Path) -> str:
    try:
        return str(p.resolve().relative_to(repo_root.resolve()))
    except Exception:
        return str(p)

def _ensure_in_repo(repo_root: Path, p: Path) -> Path:
    rp = p.resolve()
    rr = repo_root.resolve()
    if rr not in rp.parents and rp != rr:
        raise ValueError(f"Path escapes repo_root: {p}")
    return rp

@function_tool
def fs_list_glob(repo_root: str, pattern: str, max_results: int = 200) -> List[str]:
    """List files under repo_root matching a glob pattern. (Read-only)"""
    rr = Path(repo_root)
    out: List[str] = []
    for p in rr.glob(pattern):
        if p.is_file():
            out.append(str(p))
            if len(out) >= max_results:
                break
    return out

@function_tool
def fs_read_text(repo_root: str, path: str, max_chars: int = 200_000) -> str:
    """Read a text file (clipped). (Read-only)"""
    rr = Path(repo_root)
    p = _ensure_in_repo(rr, Path(path))
    data = p.read_text(encoding="utf-8", errors="replace")
    if len(data) > max_chars:
        return data[:max_chars] + "\n\n[TRUNCATED]"
    return data

@function_tool
def fs_regex_search(repo_root: str, pattern: str, glob: str = "**/*.*", max_hits: int = 200) -> List[Dict[str, Any]]:
    """Regex search across repo files. (Read-only)"""
    rr = Path(repo_root)
    rx = re.compile(pattern)
    hits: List[Dict[str, Any]] = []
    for p in rr.glob(glob):
        if not p.is_file():
            continue
        # Skip common binary-ish extensions
        if p.suffix.lower() in {".png", ".jpg", ".jpeg", ".uasset", ".umap", ".dll", ".exe", ".pdb", ".zip"}:
            continue
        try:
            txt = p.read_text(encoding="utf-8", errors="replace")
        except Exception:
            continue
        for m in rx.finditer(txt):
            hits.append({
                "file": str(p),
                "span": [m.start(), m.end()],
                "match": m.group(0)[:400],
            })
            if len(hits) >= max_hits:
                return hits
    return hits

# --- Mutating tools (enabled only in APPLY mode) ---

@function_tool
def fs_write_text(repo_root: str, path: str, content: str, make_backup: bool = True) -> str:
    """Write a text file under repo_root. (APPLY only)"""
    rr = Path(repo_root)
    p = _ensure_in_repo(rr, Path(path))
    p.parent.mkdir(parents=True, exist_ok=True)
    if make_backup and p.exists():
        bak = p.with_suffix(p.suffix + ".bak")
        bak.write_text(p.read_text(encoding="utf-8", errors="replace"), encoding="utf-8")
    p.write_text(content, encoding="utf-8")
    return f"WROTE: {_safe_rel(rr, p)}"

@function_tool
def fs_delete_dir(repo_root: str, path: str) -> str:
    """Delete a directory tree (e.g., plugin Binaries/ Intermediate/). (APPLY only)"""
    rr = Path(repo_root)
    p = _ensure_in_repo(rr, Path(path))
    if not p.exists():
        return f"SKIP (missing): {_safe_rel(rr, p)}"
    if not p.is_dir():
        raise ValueError(f"Not a directory: {p}")
    # careful recursive delete
    for child in sorted(p.rglob("*"), reverse=True):
        try:
            if child.is_file() or child.is_symlink():
                child.unlink()
            elif child.is_dir():
                child.rmdir()
        except Exception:
            pass
    try:
        p.rmdir()
    except Exception:
        pass
    return f"DELETED: {_safe_rel(rr, p)}"

@function_tool
def sys_run_subprocess(repo_root: str, argv: List[str], timeout_sec: int = 120) -> Dict[str, Any]:
    """
    Run a subprocess in repo_root. (APPLY only; also gated by --enable-subprocess)
    Safety:
      - Blocks UE builds / project gen / msbuild etc.
      - Intended for running DevTools python scripts when you explicitly opt in.
    """
    import subprocess

    rr = Path(repo_root)
    if not argv:
        raise ValueError("argv empty")

    # Block dangerous commands
    joined = " ".join(argv)
    for bad in DEFAULT_BLOCKED_SUBPROCESS:
        if bad.lower() in joined.lower():
            raise ValueError(f"Blocked subprocess (build-related): {bad}")

    # Soft allowlist: require python-like prefix
    if argv[0].lower() not in DEFAULT_ALLOWED_SUBPROCESS_PREFIXES:
        raise ValueError("Blocked subprocess: only python/py allowed by default")

    proc = subprocess.run(
        argv,
        cwd=str(rr),
        capture_output=True,
        text=True,
        timeout=timeout_sec,
    )
    return {
        "returncode": proc.returncode,
        "stdout": proc.stdout[-50_000:],
        "stderr": proc.stderr[-50_000:],
    }


# -----------------------------
# MCP helpers
# -----------------------------

def load_mcp_servers(cfg_path: Path, mode: str) -> List[Any]:
    """
    Returns a list of MCP server instances (context managers).
    Mode decides tool allowlist (plan vs apply).
    """
    if cfg_path is None or not cfg_path.exists():
        return []

    if create_static_tool_filter is None:
        raise RuntimeError("MCP imports unavailable. Ensure openai-agents is installed with MCP support.")

    raw = json.loads(cfg_path.read_text(encoding="utf-8"))
    servers: List[McpServerConfig] = []
    for s in raw.get("servers", []):
        servers.append(McpServerConfig(**s))

    out: List[Any] = []
    for s in servers:
        allowed = None
        if mode == "plan":
            allowed = s.allowed_tools_plan
        elif mode == "apply":
            allowed = s.allowed_tools_apply

        tool_filter = create_static_tool_filter(allowed_tool_names=allowed) if allowed else None

        if s.transport == "streamable_http":
            if MCPServerStreamableHttp is None:
                raise RuntimeError("MCPServerStreamableHttp unavailable")
            out.append(
                MCPServerStreamableHttp(
                    name=s.name,
                    params={"url": s.url, "headers": s.headers or {}},
                    cache_tools_list=True,
                    tool_filter=tool_filter,
                    client_session_timeout_seconds=20,
                )
            )
        elif s.transport == "sse":
            if MCPServerSse is None:
                raise RuntimeError("MCPServerSse unavailable")
            out.append(
                MCPServerSse(
                    name=s.name,
                    params={"url": s.url, "headers": s.headers or {}},
                    cache_tools_list=True,
                    tool_filter=tool_filter,
                )
            )
        elif s.transport == "stdio":
            if MCPServerStdio is None:
                raise RuntimeError("MCPServerStdio unavailable")
            out.append(
                MCPServerStdio(
                    name=s.name,
                    params={"command": s.command, "args": s.args or []},
                    cache_tools_list=True,
                    tool_filter=tool_filter,
                )
            )
        else:
            raise ValueError(f"Unknown MCP transport: {s.transport}")
    return out


# -----------------------------
# Prompt routing
# -----------------------------

def detect_header(prompt: str) -> Optional[str]:
    m = HEADER_RE.search(prompt)
    return m.group(1) if m else None

def lane_from_header(header: Optional[str]) -> Optional[str]:
    if not header:
        return None
    if header in {"SOTS_DEVTOOLS"}:
        return "devtools"
    if header in {"SOTS_BPGEN_PLAN", "SOTS_VIBEUE_PLAN"}:
        return "editor"
    if header.startswith("SOTS_BUDDY") or header.startswith("SOTS_") and "BUDDY" in header:
        return "code"
    # Also treat SOTS_BUDDY_PASS blocks as code lane
    if "SOTS_BUDDY_PASS" in header:
        return "code"
    return None

def infer_mode(mode_arg: str, prompt: str) -> str:
    if mode_arg in {"plan", "apply", "verify"}:
        return mode_arg
    # auto mode:
    # If prompt explicitly says APPLY/VERIFY in its header block, use it.
    m = re.search(r"^\s*pass:\s*(PLAN|APPLY|VERIFY)\b", prompt, re.IGNORECASE | re.MULTILINE)
    if m:
        return m.group(1).lower()
    # Default safest
    return "plan"


# -----------------------------
# Agents
# -----------------------------

BASE_SYSTEM = """You are the SOTS Orchestrator.
Non-negotiables:
- Add-only changes unless explicitly told otherwise.
- No Unreal C++ builds/runs. No packaging. No project regeneration.
- Prefer: READ -> PLAN -> APPLY -> VERIFY.
- If touching plugin code in APPLY: delete that plugin's Binaries/ and Intermediate/ afterwards (tool available).
- When uncertain: stop and report the smallest reproducible unknown.

Output formatting:
- If the input contains a bracketed SOTS header (e.g. [SOTS_DEVTOOLS], [SOTS_BPGEN_PLAN], [SOTS_BUDDY_PASS]), preserve that workflow style.
- Keep responses directly usable (copy/paste).
"""

TRIAGE_SYSTEM = BASE_SYSTEM + """
Your job: decide which lane should handle the prompt.
Return ONLY a JSON object with:
  lane: "code"|"devtools"|"editor"
  reason: short string
  suggested_mode: "plan"|"apply"|"verify"
"""

CODE_SYSTEM = BASE_SYSTEM + """
You are Buddy-in-code-mode for SOTS plugin work (C++/docs).
Default to: scan/search/read -> propose minimal patch -> apply only if in APPLY mode.
When applying, update/add a worklog file under the plugin Docs/Worklogs.
"""

DEVTOOLS_SYSTEM = BASE_SYSTEM + """
You are DevTools-pack mode for SOTS.
Primary output is a [SOTS_DEVTOOLS] prompt pack (write_files or instructions) unless user already provided one.
Do NOT execute DevTools scripts unless explicitly asked AND subprocess is enabled.
"""

EDITOR_SYSTEM = BASE_SYSTEM + """
You are Editor-ops mode (BPGen/VibeUE style).
If MCP tools are available, you may call them; otherwise produce a precise PLAN pack of actions.
Prefer tool-driven discovery/introspection before edits.
"""

def build_agents(mcp_servers: List[Any], tools_read: List[Any], tools_apply: List[Any]) -> Dict[str, Agent]:
    # Triage agent (no tools)
    triage = Agent(
        name="SOTS_Triage",
        instructions=TRIAGE_SYSTEM,
        # Keep it cheap and deterministic-ish
        model=os.getenv("SOTS_MODEL_TRIAGE", "gpt-5.2-mini"),
    )

    # Lane agents
    code = Agent(
        name="SOTS_CodeBuddy",
        instructions=CODE_SYSTEM,
        tools=tools_read,  # swapped at runtime by mode
        mcp_servers=mcp_servers or None,
        model=os.getenv("SOTS_MODEL_CODE", "gpt-5.2-codex"),
    )

    devtools = Agent(
        name="SOTS_DevTools",
        instructions=DEVTOOLS_SYSTEM,
        tools=tools_read,
        mcp_servers=mcp_servers or None,
        model=os.getenv("SOTS_MODEL_DEVTOOLS", "gpt-5.2"),
    )

    editor = Agent(
        name="SOTS_EditorOps",
        instructions=EDITOR_SYSTEM,
        tools=tools_read,
        mcp_servers=mcp_servers or None,
        model=os.getenv("SOTS_MODEL_EDITOR", "gpt-5.2-codex"),
    )

    return {"triage": triage, "code": code, "devtools": devtools, "editor": editor}


# -----------------------------
# IO sources
# -----------------------------

def read_prompt_from_args(args: argparse.Namespace) -> str:
    if args.text:
        return args.text
    if args.file:
        return Path(args.file).read_text(encoding="utf-8", errors="replace")
    # stdin fallback
    if not sys.stdin.isatty():
        return sys.stdin.read()
    raise SystemExit("No input provided. Use --text, --file, or pipe stdin.")

def iter_watch_prompts(folder: Path, poll_sec: float = 0.75) -> Iterable[Tuple[Path, str]]:
    seen: set[Path] = set()
    while True:
        for p in sorted(folder.glob("*.txt")) + sorted(folder.glob("*.md")):
            if p in seen:
                continue
            try:
                txt = p.read_text(encoding="utf-8", errors="replace")
            except Exception:
                continue
            seen.add(p)
            yield p, txt
        time.sleep(poll_sec)


# -----------------------------
# Main runner
# -----------------------------

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", required=True, help="Repo root, e.g. E:\\SAS\\ShadowsAndShurikens")
    ap.add_argument("--mode", default="auto", choices=["auto", "plan", "apply", "verify"])
    ap.add_argument("--session-id", default="sots_default")
    ap.add_argument("--session-db", default="sots_agents_sessions.db")
    ap.add_argument("--laws-file", default=None, help="Optional: path to SOTS_Suite_ExtendedMemory_LAWs.txt")
    ap.add_argument("--mcp-config", default=None, help="Optional: MCP servers json config")
    ap.add_argument("--enable-subprocess", action="store_true", help="Allow running python DevTools scripts (still blocked for builds)")
    ap.add_argument("--text", default=None)
    ap.add_argument("--file", default=None)
    ap.add_argument("--watch", default=None, help="Watch a folder for new prompt files (*.txt/*.md)")
    args = ap.parse_args()

    cfg = RunConfig(
        repo_root=Path(args.repo),
        mode=args.mode,
        session_id=args.session_id,
        session_db_path=Path(args.session_db),
        laws_file=Path(args.laws_file) if args.laws_file else None,
        mcp_config_path=Path(args.mcp_config) if args.mcp_config else None,
        enable_subprocess=bool(args.enable_subprocess),
    )

    # Build tool sets
    tools_read = [fs_list_glob, fs_read_text, fs_regex_search]
    tools_apply = tools_read + [fs_write_text, fs_delete_dir]
    if cfg.enable_subprocess:
        tools_apply.append(sys_run_subprocess)

    # MCP servers (optional). We attach them to agents; they will be used as tools automatically.
    # NOTE: Servers are async context managers; we open them around each run.
    # For simplicity: load once here; open later in run loop.
    mcp_servers = load_mcp_servers(cfg.mcp_config_path, mode="plan" if cfg.mode != "apply" else "apply") if cfg.mcp_config_path else []

    agents = build_agents(mcp_servers=mcp_servers, tools_read=tools_read, tools_apply=tools_apply)

    # Session memory
    session = SQLiteSession(cfg.session_id, str(cfg.session_db_path))

    def run_one(prompt: str) -> str:
        header = detect_header(prompt)
        lane = lane_from_header(header)

        mode = infer_mode(cfg.mode, prompt)

        # Swap toolsets based on mode
        lane_agent = None
        if lane is None:
            # Ask triage agent
            triage_json = Runner.run_sync(agents["triage"], prompt, session=session).final_output
            try:
                j = json.loads(triage_json)
                lane = j.get("lane") or "code"
                if cfg.mode == "auto":
                    mode = j.get("suggested_mode") or mode
            except Exception:
                lane = "code"

        lane_agent = agents.get(lane, agents["code"])

        # Apply tools for this mode
        if mode == "apply":
            lane_agent.tools = tools_apply
        else:
            lane_agent.tools = tools_read

        # If laws file provided, prepend a clipped snippet (keeps token use sane)
        if cfg.laws_file and cfg.laws_file.exists():
            try:
                laws_snip = cfg.laws_file.read_text(encoding="utf-8", errors="replace")[:40_000]
                prompt2 = (
                    "Reference laws (clipped):\n"
                    + laws_snip
                    + "\n\n---\n\nUser prompt:\n"
                    + prompt
                )
            except Exception:
                prompt2 = prompt
        else:
            prompt2 = prompt

        result = Runner.run_sync(lane_agent, prompt2, session=session)
        return result.final_output

    # Watch mode
    if args.watch:
        folder = Path(args.watch)
        folder.mkdir(parents=True, exist_ok=True)
        for p, txt in iter_watch_prompts(folder):
            out = run_one(txt)
            out_path = p.with_suffix(p.suffix + ".out.txt")
            out_path.write_text(out, encoding="utf-8")
            print(f"[OK] {p.name} -> {out_path.name}")
        return

    # Single-shot
    prompt = read_prompt_from_args(args)
    out = run_one(prompt)
    print(out)


if __name__ == "__main__":
    main()