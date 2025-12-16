from __future__ import annotations

import json
import subprocess
import traceback
from datetime import datetime
from pathlib import Path

import devtools_header_utils as dhu
import inbox_router
from flask import Flask, request, jsonify


# ---------------------------------------------------------------------------
# Paths / dirs
# ---------------------------------------------------------------------------

THIS_FILE = Path(__file__).resolve()
PYTHON_ROOT = THIS_FILE.parent            # .../DevTools/python
DEVTOOLS_ROOT = PYTHON_ROOT.parent        # .../DevTools
PROJECT_ROOT = DEVTOOLS_ROOT.parent       # .../ShadowsAndShurikens

INBOX_DIR = PYTHON_ROOT / "chatgpt_inbox"
LOG_DIR = PYTHON_ROOT / "logs"
LOG_FILE = LOG_DIR / "sots_bridge_server.log"

INBOX_DIR.mkdir(parents=True, exist_ok=True)
LOG_DIR.mkdir(parents=True, exist_ok=True)


def _route_existing_inbox_entries() -> None:
    try:
        results = inbox_router.scan_inbox(str(INBOX_DIR), dry_run=False)
    except Exception as exc:  # pragma: no cover - defensive
        bridge_log(f"Inbox router startup scan failed: {exc}")
        return

    if not results:
        return

    moved = sum(1 for _, new_path, reason in results if new_path and reason.startswith("MOVED"))
    errors = sum(1 for _, _, reason in results if reason.startswith("ERROR"))
    skipped = len(results) - moved - errors
    bridge_log(
        f"Inbox router startup scan: moved={moved}, skipped={skipped}, errors={errors}, total={len(results)}"
    )


# ---------------------------------------------------------------------------
# Logging helper
# ---------------------------------------------------------------------------

def bridge_log(msg: str) -> None:
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] [SOTS_BRIDGE] {msg}"
    print(line, flush=True)
    try:
        with LOG_FILE.open("a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        # Logging must never kill the server
        pass


def bridge_log_exception(exc: BaseException, context: str = "") -> None:
    context_prefix = f"{context}: " if context else ""
    bridge_log(f"{context_prefix}{exc}")
    try:
        trace = "".join(traceback.format_exception(type(exc), exc, exc.__traceback__))
        for line in trace.rstrip().splitlines():
            bridge_log(line)
    except Exception:
        # Never let logging raise.
        pass


    # Run inbox routing once logging is available so errors are captured.
    _route_existing_inbox_entries()


# ---------------------------------------------------------------------------
# Core helpers
# ---------------------------------------------------------------------------

def sanitize_label(label: str) -> str:
    safe = []
    for ch in label:
        if ch.isalnum() or ch in "-_":
            safe.append(ch)
        else:
            safe.append("_")
    return "".join(safe) or "chatgpt_prompt"


def _header_filename_from_prompt(prompt: str) -> str | None:
    header = dhu.parse_header_block(prompt)
    if not header:
        return None

    plugin_raw = header.get("plugin") or header.get("module")
    pass_raw = header.get("pass") or header.get("phase") or header.get("pass_type")
    if not plugin_raw or not pass_raw:
        return None

    plugin_name = sanitize_label(plugin_raw)
    pass_name = sanitize_label(pass_raw)
    if not plugin_name or not pass_name:
        return None

    return f"{plugin_name}_{pass_name}"


def _normalize_meta(label: str, meta: dict) -> dict[str, str]:
    pass_value = (
        meta.get("pass")
        or meta.get("phase")
        or meta.get("pass_type")
        or "BRIDGE"
    )
    override: dict[str, str] = {
        "tool": str(meta.get("tool", "chatgpt_bridge")),
        "mode": str(meta.get("mode", "auto")),
        "label": label,
        "category": str(meta.get("category", "chatgpt_bridge")),
        "plugin": str(meta.get("plugin", "SOTS_ChatGPTBridge")),
        "pass": str(pass_value),
    }
    if meta.get("url"):
        override["url"] = str(meta["url"])
    if meta.get("type"):
        override["type"] = str(meta["type"])
    return override


def _route_prompt_file(path: Path, label: str, meta: dict) -> Path:
    try:
        _, new_path, reason = inbox_router.route_file(
            str(path), str(INBOX_DIR), dry_run=False,
        )
    except Exception as exc:  # pragma: no cover - defensive
        bridge_log(f"Failed to route inbox file: {exc}")
        return path

    if new_path:
        bridge_log(f"Prompt routed to {new_path} ({reason})")
        return Path(new_path)

    if not reason.startswith("SKIP"):
        bridge_log(f"Inbox router left prompt in place: {reason}")
        return path

    # Attempt fallback routing using bridge metadata (e.g. no header present).
    fallback_override = _normalize_meta(label, meta)
    try:
        _, new_path, reason = inbox_router.route_file(
            str(path), str(INBOX_DIR), dry_run=False,
            header_override=fallback_override,
        )
    except Exception as exc:  # pragma: no cover - defensive
        bridge_log(f"Failed to route inbox file with fallback metadata: {exc}")
        return path

    if new_path:
        bridge_log(f"Prompt routed to {new_path} ({reason})")
        return Path(new_path)

    bridge_log(f"Inbox router left prompt in place: {reason}")
    return path


def store_prompt_to_inbox(prompt: str, label: str, meta: dict) -> Path:
    """Write the prompt text to chatgpt_inbox as a timestamped .txt file."""
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    header_name = _header_filename_from_prompt(prompt)
    if header_name:
        filename = f"{header_name}_{ts}.txt"
    else:
        safe_label = sanitize_label(label)
        filename = f"{ts}_{safe_label}.txt"

    out_path = INBOX_DIR / filename

    content = prompt
    if not content.endswith("\n"):
        content += "\n"

    out_path.write_text(content, encoding="utf-8")
    bridge_log(f"Stored prompt -> {out_path}")

    return _route_prompt_file(out_path, label, meta)


def handle_open_file(devtools_path: str) -> tuple[dict, int]:
    """
    Handle action='open_file' from the userscript.

    devtools_path is expected to look like:
        DevTools/python/quick_search.py
    """
    if not devtools_path:
        bridge_log("open_file: missing devtools_path")
        return {"ok": False, "error": "missing devtools_path"}, 400

    norm = devtools_path.replace("\\", "/")
    if not norm.lower().startswith("devtools/"):
        bridge_log(f"open_file: rejecting non-DevTools path: {devtools_path}")
        return {
            "ok": False,
            "error": "devtools_path must start with 'DevTools/'",
        }, 400

    # Strip the leading "DevTools/" and resolve under DEVTOOLS_ROOT
    rel_part = norm.split("/", 1)[1] if "/" in norm else ""
    abs_path = DEVTOOLS_ROOT / rel_part
    exists = abs_path.exists()
    bridge_log(f"open_file: resolved {devtools_path!r} -> {abs_path} (exists={exists})")

    if not exists:
        return {
            "ok": False,
            "error": f"file not found: {abs_path}",
            "path": str(abs_path),
        }, 404

    # Try to open with VS Code CLI (code / code.cmd). Non-blocking.
    launched = False
    err_msg = None
    for cmd in ("code", "code.cmd"):
        try:
            subprocess.Popen([cmd, str(abs_path)], cwd=str(PROJECT_ROOT))
            launched = True
            break
        except FileNotFoundError:
            continue
        except Exception as e:
            err_msg = str(e)
            break

    if not launched:
        bridge_log(
            f"open_file: failed to launch VS Code for {abs_path} "
            f"(err={err_msg!r})"
        )
        return {
            "ok": False,
            "error": "VS Code CLI (code / code.cmd) not found or failed to launch.",
            "path": str(abs_path),
        }, 500

    bridge_log(f"open_file: launched VS Code for {abs_path}")
    return {
        "ok": True,
        "launched": True,
        "path": str(abs_path),
    }, 200


# ---------------------------------------------------------------------------
# Flask app
# ---------------------------------------------------------------------------

app = Flask(__name__)


@app.route("/sots/run_prompt", methods=["POST"])
def run_prompt() -> tuple:
    """
    Main entrypoint for ChatGPT → SOTS bridge.

    Supports:
      - Normal prompt files (last markdown, code blocks, etc.)
      - open_file style actions coming from DevTools labels
    """
    try:
        data = request.get_json(force=True, silent=True) or {}

        action = (data.get("action") or "").strip()
        label = (data.get("label") or "chatgpt_prompt").strip()
        meta = data.get("meta") or {}

        # devtools_path may live in meta or at the top level
        devtools_path = meta.get("devtools_path") or data.get("devtools_path", "")

        bridge_log(
            f"run_prompt: action={action!r}, label={label!r}, "
            f"has_devtools_path={bool(devtools_path)}"
        )

        # --- Any request that looks like an open-file intent ---
        if action == "open_file" or devtools_path:
            bridge_log(f"Received open_file-style request for {devtools_path!r}")
            payload, status = handle_open_file(devtools_path)
            return jsonify(payload), status

        # --- Normal prompt path (last message, code blocks, etc.) ---
        prompt = (data.get("prompt") or "").rstrip()
        if not prompt:
            bridge_log(f"Rejected request: empty prompt (action={action!r})")
            return jsonify({"ok": False, "error": "empty prompt"}), 400

        bridge_log(f"Received prompt (label={label}, len={len(prompt)})")

        out_path = store_prompt_to_inbox(prompt, label, meta)
        resp = {
            "ok": True,
            "file": str(out_path),
            "label": label,
        }
        return jsonify(resp), 200
    except Exception as exc:
        bridge_log_exception(exc, context="run_prompt")
        return jsonify({"ok": False, "error": str(exc)}), 500


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    try:
        bridge_log("SOTS DevTools Flask bridge starting on http://127.0.0.1:5050")
        app.run(host="127.0.0.1", port=5050, debug=False)
    except Exception as exc:
        bridge_log_exception(exc, context="app.run")
