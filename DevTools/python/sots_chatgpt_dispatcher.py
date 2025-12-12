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
    Returns False if the header explicitly requests manual mode.
    """
    mode = config.get("mode", "").strip().lower()
    if mode in {"manual", "manual_only", "manual-run"}:
        return False
    return True


# ---------------------------------------------------------------------------
# Tool runners
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



def run_quick_search(config: dict[str, str]) -> None:
    """
    Dispatch to quick_search.py using 'search' and optional 'exts'.
    """
    search = config.get("search")
    if not search:
        log("quick_search requested but 'search' key is missing.")
        return

    exts = config.get("exts")  # passed straight through to --exts

    script = ROOT / "quick_search.py"
    if not script.is_file():
        log(f"quick_search.py not found at {script}")
        return

    cmd = [sys.executable, str(script), "--search", search]
    if exts:
        cmd += ["--exts", exts]

    log(f"Launching quick_search: {' '.join(cmd)}")
    try:
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"ERROR running quick_search: {e}")


def run_regex_replace(config: dict[str, str]) -> None:
    """
    Dispatch to regex_replace.py with search/replace/exts/root/dry_run.
    """
    search = config.get("search")
    replace = config.get("replace")
    if not search or replace is None:
        log("regex_replace requested but 'search' or 'replace' is missing.")
        return

    exts = config.get("exts")  # regex over extensions, e.g. \.cpp|\.h
    root = config.get("root")  # optional override
    dry_run = str_to_bool(config.get("dry_run"), default=True)

    script = ROOT / "regex_replace.py"
    if not script.is_file():
        log(f"regex_replace.py not found at {script}")
        return

    cmd = [sys.executable, str(script), "--search", search, "--replace", replace]
    if exts:
        cmd += ["--exts", exts]
    if root:
        cmd += ["--root", root]
    if dry_run:
        cmd += ["--dry-run"]

    log(f"Launching regex_replace: {' '.join(cmd)}")
    try:
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"ERROR running regex_replace: {e}")


def split_paths(value: str) -> list[str]:
    # Split on ; or , and trim
    parts = re.split(r"[;,]", value)
    return [p.strip() for p in parts if p.strip()]


def run_delete_paths(config: dict[str, str]) -> None:
    """
    Dispatch to delete_paths.py with list of paths and dry_run.
    """
    paths_str = config.get("paths")
    if not paths_str:
        log("delete_paths requested but 'paths' is missing.")
        return

    paths = split_paths(paths_str)
    if not paths:
        log("delete_paths: no valid paths after parsing.")
        return

    dry_run = str_to_bool(config.get("dry_run"), default=True)

    script = ROOT / "delete_paths.py"
    if not script.is_file():
        log(f"delete_paths.py not found at {script}")
        return

    cmd = [sys.executable, str(script), "--paths", *paths]
    if dry_run:
        cmd += ["--dry-run"]

    log(f"Launching delete_paths: {' '.join(cmd)}")
    try:
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"ERROR running delete_paths: {e}")


# ---------------------------------------------------------------------------
# Dispatch helpers (exported for sots_tools.py)
# ---------------------------------------------------------------------------

def _dispatch_config(config: dict[str, str], prompt_path: Path, *, force: bool) -> None:
    if not config:
        log("No config; nothing to dispatch.")
        return

    if not force and not should_auto_dispatch(config):
        log("Header mode=manual; skipping auto-dispatch. Use sots_tools.py for manual run.")
        return

    tool = config.get("tool", "").lower()
    log(f"Dispatching tool: {tool!r}, force={force}")

    if tool == "write_files":
        run_write_files(config, prompt_path)
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
    Public entrypoint for other scripts (bridge_runner, sots_tools).
    - Reads header from prompt_path.
    - Respects mode=manual unless force=True.
    """
    if not prompt_path.is_file():
        log(f"ERROR: Prompt file not found: {prompt_path}")
        return

    log(f"Processing prompt file: {prompt_path} (force={force})")
    config = parse_header(prompt_path)
    _dispatch_config(config, prompt_path, force=force)


# ---------------------------------------------------------------------------
# CLI entrypoint
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(description="SOTS ChatGPT DevTools dispatcher")
    parser.add_argument(
        "--prompt_file",
        type=Path,
        required=True,
        help="Path to the prompt .txt file from ChatGPT inbox",
    )
    args = parser.parse_args()

    dispatch_file(args.prompt_file, force=False)


if __name__ == "__main__":
    main()