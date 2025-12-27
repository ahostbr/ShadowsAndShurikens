from __future__ import annotations

import argparse
import dataclasses
import json
import logging
import os
import re
import sys
import time
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple

from agents import Agent, Runner, SQLiteSession, function_tool

# MCP is optional. Keep imports guarded so the script still runs without MCP installed.
try:
    import anyio
    from mcp.server import InitializationOptions, NotificationOptions, Server as McpServer
    from mcp.server.stdio import stdio_server
    from mcp.types import TextContent, Tool
except Exception:  # pragma: no cover
    anyio = None
    McpServer = None
    stdio_server = None
    TextContent = None
    Tool = None


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
    enable_subprocess: bool = False  # Off by default (matches "manual DevTools first" culture)

    # MCP-server safety toggles
    allow_apply: bool = False  # MCP tool calls cannot do APPLY unless explicitly allowed at server start
    server_name: str = "sots-agents-runner"
    server_version: str = "0.1.0"


# -----------------------------
# Logging (MCP-safe)
# -----------------------------

def setup_logging(repo_root: Path, name: str = "sots_agents_runner", *, mcp_mode: bool) -> logging.Logger:
    """
    In MCP stdio mode, DO NOT print to stdout (breaks protocol). We log to:
      - repo_root/DevTools/logs/<name>.log  (if possible)
      - stderr (safe for MCP)
    """
    logger = logging.getLogger(name)
    if logger.handlers:
        return logger

    logger.setLevel(logging.INFO)
    fmt = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")

    # stderr handler (safe even in MCP stdio)
    sh = logging.StreamHandler(stream=sys.stderr)
    sh.setFormatter(fmt)
    logger.addHandler(sh)

    # file handler
    try:
        log_dir = repo_root / "DevTools" / "logs"
        log_dir.mkdir(parents=True, exist_ok=True)
        fh = logging.FileHandler(log_dir / f"{name}.log", encoding="utf-8")
        fh.setFormatter(fmt)
        logger.addHandler(fh)
    except Exception:
        # if we can't write logs, stderr is still fine
        pass

    try:
        py_log_dir = repo_root / "DevTools" / "python" / "logs"
        py_log_dir.mkdir(parents=True, exist_ok=True)
        fh_py = logging.FileHandler(py_log_dir / f"{name}.log", encoding="utf-8")
        fh_py.setFormatter(fmt)
        logger.addHandler(fh_py)
    except Exception:
        pass

    if not mcp_mode:
        logger.info("Logging initialized (non-MCP mode).")
    else:
        logger.info("Logging initialized (MCP stdio mode).")

    return logger


# -----------------------------
# Local file tools (agent-callable)
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

    joined = " ".join(argv)
    for bad in DEFAULT_BLOCKED_SUBPROCESS:
        if bad.lower() in joined.lower():
            raise ValueError(f"Blocked subprocess (build-related): {bad}")

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
    if header in {"SOTS_BPGEN_PLAN", "SOTS_BPGEN", "SOTS_VIBEUE_PLAN"}:
        return "editor"
    if header.startswith("SOTS_BUDDY") or (header.startswith("SOTS_") and "BUDDY" in header):
        return "code"
    if "SOTS_BUDDY_PASS" in header:
        return "code"
    return None

def infer_mode(mode_arg: str, prompt: str) -> str:
    if mode_arg in {"plan", "apply", "verify"}:
        return mode_arg
    m = re.search(r"^\s*pass:\s*(PLAN|APPLY|VERIFY)\b", prompt, re.IGNORECASE | re.MULTILINE)
    if m:
        return m.group(1).lower()
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
Prefer: tool-driven discovery/introspection before edits.
If you cannot mutate directly (no editor tools), produce a precise PLAN pack of actions.
"""


MODEL_ALIASES: Dict[str, str] = {
    # SOTS convention: uppercase identifiers using underscores.
    # These map to OpenAI model IDs.
    "GPT_5_2": "gpt-5.2",
    "GPT_5_2_MINI": "gpt-5.2-mini",
    "GPT_5_2_CODEX": "gpt-5.2-codex",
}


def normalize_model_name(model: str) -> str:
    """Normalize a model name or alias to an OpenAI model id.

    Accepts either:
    - Real model ids like "gpt-5.2", "gpt-5.2-codex"
    - Alias constants like "GPT_5_2", "GPT_5_2_CODEX"

    Unknown values are returned unchanged so the upstream API can validate.
    """
    m = (model or "").strip()
    if not m:
        return ""
    if m.startswith("gpt-") or m.startswith("o"):
        return m
    key = re.sub(r"[^A-Za-z0-9]+", "_", m).upper()

    # Built-in aliases first.
    resolved = MODEL_ALIASES.get(key)
    if resolved:
        return resolved

    # Optional: treat the provided name as an env-defined alias.
    # Preferred (namespaced) pattern:
    #   SOTS_MODEL_GPT_5_2_CODEX=gpt-5.2-codex
    #   SOTS_MODEL_CODE=GPT_5_2_CODEX
    env_namespaced = (os.getenv(f"SOTS_MODEL_{key}") or "").strip()
    if env_namespaced:
        return env_namespaced

    # Back-compat: allow un-namespaced env aliases too.
    env_defined = (os.getenv(key) or "").strip()
    if env_defined:
        return env_defined

    return m


def build_agents(tools_read: List[Any], tools_apply: List[Any]) -> Dict[str, Agent]:
    triage = Agent(
        name="SOTS_Triage",
        instructions=TRIAGE_SYSTEM,
        model=normalize_model_name(os.getenv("SOTS_MODEL_TRIAGE", "gpt-5.2-mini")),
    )

    code = Agent(
        name="SOTS_CodeBuddy",
        instructions=CODE_SYSTEM,
        tools=tools_read,  # swapped at runtime
        model=normalize_model_name(os.getenv("SOTS_MODEL_CODE", "gpt-5.2-codex")),
    )

    devtools = Agent(
        name="SOTS_DevTools",
        instructions=DEVTOOLS_SYSTEM,
        tools=tools_read,
        model=normalize_model_name(os.getenv("SOTS_MODEL_DEVTOOLS", "gpt-5.2")),
    )

    editor = Agent(
        name="SOTS_EditorOps",
        instructions=EDITOR_SYSTEM,
        tools=tools_read,
        model=normalize_model_name(os.getenv("SOTS_MODEL_EDITOR", "gpt-5.2-codex")),
    )

    return {"triage": triage, "code": code, "devtools": devtools, "editor": editor}


# -----------------------------
# Orchestrator (shared by CLI + MCP server)
# -----------------------------

class SOTSAgentsOrchestrator:
    def __init__(self, cfg: RunConfig, *, logger: logging.Logger) -> None:
        self.cfg = cfg
        self.logger = logger

        self.tools_read = [fs_list_glob, fs_read_text, fs_regex_search]
        self.tools_apply = self.tools_read + [fs_write_text, fs_delete_dir]
        if cfg.enable_subprocess:
            self.tools_apply.append(sys_run_subprocess)

        self.agents = build_agents(tools_read=self.tools_read, tools_apply=self.tools_apply)
        self.session = SQLiteSession(cfg.session_id, str(cfg.session_db_path))

        self._laws_snip_cache: Optional[str] = None

        self.logger.info(f"Orchestrator ready: repo_root={cfg.repo_root} mode={cfg.mode} session_id={cfg.session_id}")

    def _get_laws_snip(self) -> Optional[str]:
        if not self.cfg.laws_file:
            return None
        p = self.cfg.laws_file
        if not p.exists():
            return None
        if self._laws_snip_cache is not None:
            return self._laws_snip_cache
        try:
            self._laws_snip_cache = p.read_text(encoding="utf-8", errors="replace")[:40_000]
            return self._laws_snip_cache
        except Exception:
            return None

    def run_one(self, prompt: str, *, mode_override: Optional[str] = None, lane_override: Optional[str] = None) -> Dict[str, Any]:
        """
        Returns a structured dict:
          { ok, lane, mode, output, header, errors[] }
        """
        errors: List[str] = []

        header = detect_header(prompt)
        lane = lane_override or lane_from_header(header)

        mode = infer_mode(self.cfg.mode, prompt)
        if mode_override:
            mode = infer_mode(mode_override, prompt)

        if mode == "apply" and not self.cfg.allow_apply:
            return {
                "ok": False,
                "lane": lane or "unknown",
                "mode": mode,
                "header": header or "",
                "output": "",
                "errors": ["APPLY is disabled for this MCP server. Restart server with --allow-apply to enable."],
            }

        # Choose lane by triage if needed
        if lane is None:
            triage_json = Runner.run_sync(self.agents["triage"], prompt, session=self.session).final_output
            try:
                j = json.loads(triage_json)
                lane = j.get("lane") or "code"
                if self.cfg.mode == "auto" and not mode_override:
                    mode = j.get("suggested_mode") or mode
            except Exception:
                lane = "code"

        lane_agent = self.agents.get(lane, self.agents["code"])

        # Apply toolset
        lane_agent.tools = self.tools_apply if mode == "apply" else self.tools_read

        laws_snip = self._get_laws_snip()
        prompt2 = prompt
        if laws_snip:
            prompt2 = "Reference laws (clipped):\n" + laws_snip + "\n\n---\n\nUser prompt:\n" + prompt

        self.logger.info(f"RUN: lane={lane} mode={mode} header={header or ''}")

        result = Runner.run_sync(lane_agent, prompt2, session=self.session)
        out = result.final_output

        return {
            "ok": True,
            "lane": lane,
            "mode": mode,
            "header": header or "",
            "output": out,
            "errors": errors,
        }


# -----------------------------
# Library entrypoint for MCP tool calls
# -----------------------------

def sots_agents_run_task(payload: Dict[str, Any]) -> Dict[str, Any]:
    payload = payload or {}
    repo_root = resolve_repo_root(payload.get("repo"))
    logger = setup_logging(repo_root, name="sots_agents_runner", mcp_mode=True)

    api_key = os.getenv("OPENAI_API_KEY") or ""
    if not api_key:
        return {
            "ok": False,
            "output_text": "",
            "error": "OPENAI_API_KEY is missing",
            "meta": {"repo_root": str(repo_root)},
        }

    prompt = str(payload.get("prompt", "")).strip()
    if not prompt:
        return {"ok": False, "output_text": "", "error": "prompt is empty", "meta": {"repo_root": str(repo_root)}}

    system = str(payload.get("system") or "")
    model_override = normalize_model_name(str(payload.get("model") or ""))
    reasoning_effort = str(payload.get("reasoning_effort") or "high")
    mode = str(payload.get("mode") or "plan")
    lane = payload.get("lane")
    session_id = str(payload.get("session_id") or "sots_mcp_inline")
    session_db = payload.get("session_db") or "sots_agents_sessions.db"
    laws_file_raw = payload.get("laws_file")
    laws_file = Path(laws_file_raw).expanduser() if laws_file_raw else None
    session_db_path = Path(session_db)
    if not session_db_path.is_absolute():
        session_db_path = repo_root / session_db_path

    cfg = RunConfig(
        repo_root=repo_root,
        mode=mode,
        session_id=session_id,
        session_db_path=session_db_path,
        laws_file=laws_file if laws_file and laws_file.exists() else None,
        enable_subprocess=False,
        allow_apply=False,
    )

    try:
        orch = SOTSAgentsOrchestrator(cfg, logger=logger)
        if model_override:
            for ag in orch.agents.values():
                ag.model = model_override
        prompt_final = prompt if not system else f"{system.strip()}\n\n{prompt}"
        res = orch.run_one(prompt_final, mode_override=mode, lane_override=lane)
        error_text = None
        if not res.get("ok") and res.get("errors"):
            error_text = "; ".join(res.get("errors"))
        meta = {
            "mode": res.get("mode") or mode,
            "lane": res.get("lane") or lane,
            "session_id": session_id,
            "model": model_override,
            "reasoning_effort": reasoning_effort,
        }
        return {
            "ok": bool(res.get("ok")),
            "output_text": res.get("output", ""),
            "error": error_text,
            "meta": meta,
        }
    except Exception as exc:
        logger.exception("sots_agents_run_task failed")
        return {
            "ok": False,
            "output_text": "",
            "error": str(exc),
            "meta": {
                "mode": mode,
                "lane": lane,
                "session_id": session_id,
                "model": model_override,
                "reasoning_effort": reasoning_effort,
            },
        }


# -----------------------------
# CLI helpers
# -----------------------------

def read_prompt_from_args(args: argparse.Namespace) -> str:
    if args.text:
        return args.text
    if args.file:
        return Path(args.file).read_text(encoding="utf-8", errors="replace")
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
# MCP stdio server (this is what VS Code starts)
# -----------------------------

async def serve_mcp_stdio(cfg: RunConfig) -> None:
    if anyio is None or McpServer is None or stdio_server is None:
        raise RuntimeError("MCP dependencies not available. Install `mcp` and `anyio` in this python environment.")

    logger = setup_logging(cfg.repo_root, name="sots_agents_mcp", mcp_mode=True)
    orch = SOTSAgentsOrchestrator(cfg, logger=logger)

    server = McpServer(cfg.server_name)

    @server.list_tools()
    async def list_tools() -> list[Tool]:
        return [
            Tool(
                name="ping",
                description="Health check for the SOTS Agents MCP server.",
                inputSchema={"type": "object", "properties": {}, "required": []},
            ),
            Tool(
                name="run_prompt",
                description="Run a SOTS prompt through the safeguarded orchestrator (PLAN by default; APPLY only if enabled at server start).",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "prompt": {"type": "string"},
                        "mode": {"type": "string", "description": "auto|plan|apply|verify (apply may be blocked)"},
                        "lane": {"type": "string", "description": "optional override: code|devtools|editor"},
                    },
                    "required": ["prompt"],
                },
            ),
        ]

    @server.call_tool()
    async def call_tool(name: str, arguments: dict | None) -> list[TextContent]:
        arguments = arguments or {}
        if name == "ping":
            return [TextContent(type="text", text="pong")]

        if name == "run_prompt":
            prompt = str(arguments.get("prompt", "")).strip()
            if not prompt:
                return [TextContent(type="text", text=json.dumps({"ok": False, "errors": ["prompt is empty"]}))]

            mode = arguments.get("mode")
            lane = arguments.get("lane")

            def _run() -> Dict[str, Any]:
                return orch.run_one(prompt, mode_override=mode, lane_override=lane)

            # Run in thread so we don't block the MCP event loop
            res = await anyio.to_thread.run_sync(_run)
            return [TextContent(type="text", text=json.dumps(res, ensure_ascii=False))]

        raise ValueError(f"Unknown tool: {name}")

    async with stdio_server() as (read, write):
        init_opts = InitializationOptions(
            server_name=cfg.server_name,
            server_version=cfg.server_version,
            capabilities=server.get_capabilities(
                notification_options=NotificationOptions(),
                experimental_capabilities={},
            ),
        )
        logger.info(f"Starting MCP stdio server: {cfg.server_name} v{cfg.server_version}")
        await server.run(read, write, initialization_options=init_opts)


# -----------------------------
# Main
# -----------------------------

def resolve_repo_root(repo_arg: Optional[str]) -> Path:
    if repo_arg:
        return Path(repo_arg)
    # Prefer env var if running under MCP host
    env = os.getenv("SOTS_PROJECT_ROOT") or os.getenv("SOTS_PROJECT_ROOT".upper()) or os.getenv("SOTS_PROJECT_ROOT".lower())
    if env:
        return Path(env)
    env2 = os.getenv("SOTS_PROJECT_ROOT") or os.getenv("SOTS_PROJECT_ROOT".upper())
    if env2:
        return Path(env2)
    # Fall back to cwd
    return Path.cwd()

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=None, help="Repo root, e.g. E:\\SAS\\ShadowsAndShurikens (optional in MCP mode if env is set)")
    ap.add_argument("--mode", default="auto", choices=["auto", "plan", "apply", "verify"])
    ap.add_argument("--session-id", default="sots_default")
    ap.add_argument("--session-db", default="sots_agents_sessions.db")
    ap.add_argument("--laws-file", default=None, help="Optional: path to SOTS_Suite_ExtendedMemory_LAWs.txt")
    ap.add_argument("--enable-subprocess", action="store_true", help="Allow running python DevTools scripts (still blocked for builds)")
    ap.add_argument("--allow-apply", action="store_true", help="Allow APPLY mode via MCP tool calls (off by default)")

    # CLI input modes
    ap.add_argument("--text", default=None)
    ap.add_argument("--file", default=None)
    ap.add_argument("--watch", default=None, help="Watch a folder for new prompt files (*.txt/*.md)")

    # MCP mode
    ap.add_argument("--mcp-stdio", action="store_true", help="Run as an MCP stdio server (VS Code starts this).")

    args = ap.parse_args()

    repo_root = resolve_repo_root(args.repo)
    repo_root = repo_root.resolve()

    # Setup logger (non-mcp)
    logger = setup_logging(repo_root, name="sots_agents_runner", mcp_mode=bool(args.mcp_stdio))

    cfg = RunConfig(
        repo_root=repo_root,
        mode=args.mode,
        session_id=args.session_id,
        session_db_path=Path(args.session_db),
        laws_file=Path(args.laws_file) if args.laws_file else None,
        enable_subprocess=bool(args.enable_subprocess),
        allow_apply=bool(args.allow_apply),
    )

    # MCP server path
    if args.mcp_stdio:
        if anyio is None:
            raise SystemExit("Missing MCP deps. Install: pip install mcp anyio")
        anyio.run(serve_mcp_stdio, cfg)
        return

    # Normal CLI runner path
    orch = SOTSAgentsOrchestrator(cfg, logger=logger)

    # Watch mode
    if args.watch:
        folder = Path(args.watch)
        folder.mkdir(parents=True, exist_ok=True)
        for p, txt in iter_watch_prompts(folder):
            res = orch.run_one(txt)
            out_path = p.with_suffix(p.suffix + ".out.txt")
            out_path.write_text(res.get("output", ""), encoding="utf-8")
            logger.info(f"[OK] {p.name} -> {out_path.name}")
        return

    # Single-shot
    prompt = read_prompt_from_args(args)
    res = orch.run_one(prompt)
    print(res.get("output", ""))


if __name__ == "__main__":
    main()
