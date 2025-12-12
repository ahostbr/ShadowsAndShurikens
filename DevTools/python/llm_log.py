from __future__ import annotations

import json
import os
import platform
import uuid
import inspect
from datetime import datetime
from pathlib import Path
from typing import Any, Dict

# We *optionally* use project_paths to include the project root in logs.
# If it's not available for some reason, we degrade gracefully.
try:
    from project_paths import get_project_root  # type: ignore
except Exception:  # pragma: no cover - defensive
    get_project_root = None  # type: ignore


# ---------- Internal helpers ----------

def _get_logs_dir() -> Path:
    """
    Returns DevTools/python/Logs (next to this file), creating it if needed.
    """
    logs_dir = Path(__file__).resolve().parent / "Logs"
    logs_dir.mkdir(parents=True, exist_ok=True)
    return logs_dir


def _safe_serialize(value: Any) -> Any:
    """
    Make values JSON-safe. Paths, enums, etc. become strings.
    """
    if isinstance(value, Path):
        return str(value)
    try:
        json.dumps(value)
        return value
    except TypeError:
        return str(value)


def _get_environment_snapshot() -> Dict[str, Any]:
    """
    Capture everything about the environment that is generally useful for
    debugging a SOTS tool run.
    """
    # Who called us?
    try:
        # [0] is this function, [1] is print_llm_summary, [2] is the caller
        frame = inspect.stack()[2]
        caller_file = Path(frame.filename).name
    except Exception:
        caller_file = "UNKNOWN"

    # Project root (if project_paths is available)
    project_root: str | None = None
    try:
        if get_project_root is not None:
            project_root = str(get_project_root())
    except Exception:
        project_root = None

    env: Dict[str, Any] = {
        "env_timestamp_utc": datetime.utcnow().isoformat(timespec="seconds") + "Z",
        "env_run_id": str(uuid.uuid4()),
        "env_caller_script": caller_file,
        "env_cwd": str(Path.cwd()),
        "env_python_version": platform.python_version(),
        "env_platform": platform.system(),
        "env_platform_release": platform.release(),
        "env_machine": platform.machine(),
        "env_process_id": os.getpid(),
        "env_project_root": project_root,
    }

    return env


def _write_jsonl_entry(log_data: Dict[str, Any]) -> None:
    """
    Append a JSON entry to llm_summary_log.jsonl.
    One line per tool invocation, safe to grow over time.
    """
    logs_dir = _get_logs_dir()
    jsonl_path = logs_dir / "llm_summary_log.jsonl"

    # Ensure all values are JSON-safe
    safe_data = {k: _safe_serialize(v) for k, v in log_data.items()}

    try:
        with jsonl_path.open("a", encoding="utf-8") as f:
            f.write(json.dumps(safe_data, ensure_ascii=False))
            f.write("\n")
    except Exception as e:
        print(f"[WARN] Failed to write JSONL log: {e}")


def _write_text_entry(tool: str, lines: str) -> None:
    """
    Append a human-readable text entry to:
      - llm_summary_log.txt (global)
      - <tool>.log (per-tool)
    """
    logs_dir = _get_logs_dir()
    global_txt = logs_dir / "llm_summary_log.txt"
    tool_txt = logs_dir / f"{tool}.log"

    try:
        for path in (global_txt, tool_txt):
            with path.open("a", encoding="utf-8") as f:
                f.write(lines)
                if not lines.endswith("\n"):
                    f.write("\n")
    except Exception as e:
        print(f"[WARN] Failed to write text log(s): {e}")


# ---------- Public API ----------

def print_llm_summary(
    tool: str,
    status: str = "OK",
    **fields: Any,
) -> None:
    """
    Central logging helper used by all SOTS DevTools scripts.

    - Prints an LLM-friendly block to stdout:
        [LLM_LOG_START]
        TIMESTAMP=...
        RUN_ID=...
        TOOL=...
        STATUS=...
        ENV_* / USER FIELDS...
        [LLM_LOG_END]

    - Writes the same info to:
        DevTools/python/Logs/llm_summary_log.jsonl  (structured JSONL)
        DevTools/python/Logs/llm_summary_log.txt   (human-readable text)
        DevTools/python/Logs/<tool>.log            (per-tool text log)

    'tool' should be a short identifier, e.g. "ensure_plugin_modules",
    "clean_build_caches_and_regen_vs", "run_build_and_analyze", etc.

    'status' is a coarse summary: "OK", "WARN", "ERROR", "SKIP", etc.

    Additional keyword arguments become fields on the log entry.
    """

    # Environment snapshot (everything I'd want to know about the run context)
    env = _get_environment_snapshot()

    # Main metadata
    timestamp = env["env_timestamp_utc"]
    run_id = env["env_run_id"]

    # Merge user fields
    user_fields: Dict[str, Any] = dict(fields)

    # Prepare text block (for stdout + text files)
    lines = [
        "[LLM_LOG_START]",
        f"TIMESTAMP={timestamp}",
        f"RUN_ID={run_id}",
        f"TOOL={tool}",
        f"STATUS={status}",
    ]

    # Environment fields
    for key, value in env.items():
        lines.append(f"{key}={value}")

    # User fields (tool-specific data)
    for key, value in user_fields.items():
        # For text, stringify complex values via JSON to stay parseable
        if isinstance(value, (dict, list)):
            val_str = json.dumps(_safe_serialize(value), ensure_ascii=False)
        else:
            val_str = str(value)
        lines.append(f"{key}={val_str}")

    lines.append("[LLM_LOG_END]")
    text_block = "\n".join(lines)

    # Print to console for immediate visibility
    print(text_block)

    # Build JSON object for JSONL log
    json_data: Dict[str, Any] = {
        "timestamp": timestamp,
        "run_id": run_id,
        "tool": tool,
        "status": status,
    }
    json_data.update(env)
    json_data.update(user_fields)

    # Persist logs
    _write_jsonl_entry(json_data)
    _write_text_entry(tool, text_block + "\n")
