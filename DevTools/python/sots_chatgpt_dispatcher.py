from __future__ import annotations

import argparse
import datetime
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)
LOG_FILE = LOG_DIR / "chatgpt_dispatcher.log"


def log(msg: str) -> None:
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] [DISPATCHER] {msg}"
    print(line)
    with LOG_FILE.open("a", encoding="utf-8") as f:
        f.write(line + "\n")


# ---------------------------------------------------------------------------
# Header parsing
# ---------------------------------------------------------------------------

def parse_header(prompt_path: Path) -> dict[str, str]:
    """
    Parse the [SOTS_DEVTOOLS] header block from the prompt file.
    Returns a dict of key -> value. If no header, returns {}.
    """
    text = prompt_path.read_text(encoding="utf-8", errors="replace")
    start_tag = "[SOTS_DEVTOOLS]"
    end_tag = "[/SOTS_DEVTOOLS]"

    start_idx = text.find(start_tag)
    end_idx = text.find(end_tag)

    if start_idx == -1 or end_idx == -1 or end_idx < start_idx:
        log("No SOTS_DEVTOOLS header found.")
        return {}

    header_text = text[start_idx + len(start_tag) : end_idx]
    config: dict[str, str] = {}

    for raw_line in header_text.splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        # Allow "key: value" or "key=value"
        if ":" in line:
            key, value = line.split(":", 1)
        elif "=" in line:
            key, value = line.split("=", 1)
        else:
            log(f"Skipping malformed header line: {line!r}")
            continue
        config[key.strip().lower()] = value.strip()

    log(f"Parsed header config: {config}")
    return config


def str_to_bool(value: str | None, default: bool = True) -> bool:
    if value is None:
        return default
    v = value.strip().lower()
    if v in {"1", "true", "yes", "y"}:
        return True
    if v in {"0", "false", "no", "n"}:
        return False
    return default


def should_auto_dispatch(config: dict[str, str]) -> bool:
    """
    Return True if this prompt should be auto-dispatched.

    If a prompt declares:
        auto_dispatch: false
    we skip running it automatically.
    """
    return str_to_bool(config.get("auto_dispatch"), default=True)


# ---------------------------------------------------------------------------
# Tool dispatchers
# ---------------------------------------------------------------------------

def run_write_files(config: dict[str, str], prompt_path: Path) -> None:
    """Dispatch to write_files.py using the full prompt file as --source.

    Expected header shape:

        [SOTS_DEVTOOLS]
        tool: write_files
        mode: ...
        [/SOTS_DEVTOOLS]

    The write_files.py script will:
      - Parse the body for FILE: ... / === END FILE === blocks
      - Write those files under PROJECT_ROOT
      - Log a summary in DevTools/python/logs/write_files_*.log
    """
    script = ROOT / "write_files.py"
    if not script.is_file():
        log(f"write_files.py not found at {script}")
        return

    cmd = [sys.executable, str(script), "--source", str(prompt_path)]
    log(f"Launching write_files: {' '.join(cmd)}")

    try:
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"ERROR running write_files: {e}")


def run_save_context_anchor(config: dict[str, str], prompt_path: Path) -> None:
    """Dispatch to save_context_anchor.py using the full prompt file as --source.

    Expected body contains one or more [CONTEXT_ANCHOR] blocks.

    The tool will:
      - Extract anchor blocks
      - Infer plugin(s) from Lock_<PLUGIN> or plugin fields
      - Write copies into Plugins/<Plugin>/Docs/Anchor/
      - Write a central copy into DevTools/python/Saved/ContextAnchors/
      - Log to DevTools/python/logs/context_anchor.log
    """
    script = ROOT / "save_context_anchor.py"
    if not script.is_file():
        log(f"save_context_anchor.py not found at {script}")
        return

    cmd = [sys.executable, str(script), "--source", str(prompt_path)]
    log(f"Launching save_context_anchor: {' '.join(cmd)}")

    try:
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"ERROR running save_context_anchor: {e}")


def run_quick_search(config: dict[str, str]) -> None:
    """
    Dispatch to quick_search.py using 'search' and optional 'exts'.
    """
    search = config.get("search")
    if not search:
        log("quick_search requires 'search' in header.")
        return

    exts = config.get("exts", "").strip()
    script = ROOT / "quick_search.py"
    if not script.is_file():
        log(f"quick_search.py not found at {script}")
        return

    cmd = [sys.executable, str(script), "--search", search]
    if exts:
        cmd.extend(["--exts", exts])

    log(f"Launching quick_search: {' '.join(cmd)}")
    try:
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"ERROR running quick_search: {e}")


def run_regex_replace(config: dict[str, str]) -> None:
    """
    Dispatch to regex_replace.py.

    Expected header keys:
      - search: regex pattern
      - replace: replacement
      - exts: e.g. .h,.cpp,.ini
    """
    search = config.get("search")
    replace = config.get("replace", "")
    exts = config.get("exts", "")

    if not search:
        log("regex_replace requires 'search' in header.")
        return

    script = ROOT / "regex_replace.py"
    if not script.is_file():
        log(f"regex_replace.py not found at {script}")
        return

    cmd = [sys.executable, str(script), "--search", search, "--replace", replace]
    if exts:
        cmd.extend(["--exts", exts])

    log(f"Launching regex_replace: {' '.join(cmd)}")
    try:
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"ERROR running regex_replace: {e}")


def run_delete_paths(config: dict[str, str]) -> None:
    """
    Dispatch to delete_paths.py.

    Expected header keys:
      - paths: semicolon-separated paths
    """
    paths = config.get("paths", "")
    if not paths:
        log("delete_paths requires 'paths' in header.")
        return

    script = ROOT / "delete_paths.py"
    if not script.is_file():
        log(f"delete_paths.py not found at {script}")
        return

    cmd = [sys.executable, str(script), "--paths", paths]
    log(f"Launching delete_paths: {' '.join(cmd)}")
    try:
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"ERROR running delete_paths: {e}")


def dispatch_tool(prompt_path: Path, *, force: bool = False) -> None:
    config = parse_header(prompt_path)
    if not config:
        log("No header config; nothing to dispatch.")
        return

    if not force and not should_auto_dispatch(config):
        log("auto_dispatch=false (manual); skipping auto-dispatch. Use sots_tools.py for manual run.")
        return

    tool = config.get("tool", "").lower()
    log(f"Dispatching tool: {tool!r}, force={force}")

    if tool == "write_files":
        run_write_files(config, prompt_path)
    elif tool == "save_context_anchor":
        run_save_context_anchor(config, prompt_path)
    elif tool == "quick_search":
        run_quick_search(config)
    elif tool == "regex_replace":
        run_regex_replace(config)
    elif tool == "delete_paths":
        run_delete_paths(config)
    else:
        log(f"Unknown or unsupported tool: {tool!r}")


def dispatch_file(prompt_path: Path, *, force: bool = False) -> None:
    """
    Dispatch a single file that contains a [SOTS_DEVTOOLS] header.
    """
    log(f"Dispatching file: {prompt_path}")
    dispatch_tool(prompt_path, force=force)


def find_latest_prompt(inbox_root: Path) -> Path | None:
    """
    Find the newest file in the inbox tree (by mtime).
    """
    best: Path | None = None
    best_mtime = -1.0
    for p in inbox_root.rglob("*"):
        if not p.is_file():
            continue
        if p.suffix.lower() not in {".txt", ".md"}:
            continue
        try:
            mtime = p.stat().st_mtime
        except OSError:
            continue
        if mtime > best_mtime:
            best_mtime = mtime
            best = p
    return best


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--file", help="Dispatch a specific prompt file path.")
    ap.add_argument("--latest", action="store_true", help="Dispatch the newest file in chatgpt_inbox.")
    ap.add_argument("--force", action="store_true", help="Force dispatch even if auto_dispatch=false.")
    args = ap.parse_args(argv)

    if args.file:
        dispatch_file(Path(args.file), force=args.force)
        return 0

    if args.latest:
        inbox = ROOT / "chatgpt_inbox"
        latest = find_latest_prompt(inbox)
        if not latest:
            log("No prompt files found in chatgpt_inbox.")
            return 0
        dispatch_file(latest, force=args.force)
        return 0

    ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
