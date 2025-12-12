from __future__ import annotations

import datetime
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)
LOG_FILE = LOG_DIR / "sots_bridge_runner.log"

DISPATCHER = ROOT / "sots_chatgpt_dispatcher.py"

# Optional: auto-open prompt file in editor as well
EDITOR_CMD = ["code"]  # or [] if you don't want auto-open


def log(msg: str) -> None:
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] [BRIDGE_RUNNER] {msg}"
    print(line)
    with LOG_FILE.open("a", encoding="utf-8") as f:
        f.write(line + "\n")


def open_in_editor(prompt_path: Path) -> None:
    if not EDITOR_CMD:
        return
    try:
        cmd = [*EDITOR_CMD, str(prompt_path)]
        log(f"Launching editor: {' '.join(cmd)}")
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"WARNING: Failed to open editor: {e}")


def run_dispatcher(prompt_path: Path) -> None:
    if not DISPATCHER.is_file():
        log(f"Dispatcher not found at {DISPATCHER}, skipping.")
        return

    cmd = [sys.executable, str(DISPATCHER), "--prompt_file", str(prompt_path)]
    log(f"Launching dispatcher: {' '.join(cmd)}")
    try:
        subprocess.Popen(cmd, cwd=str(ROOT))
    except Exception as e:
        log(f"ERROR launching dispatcher: {e}")


def main() -> None:
    if len(sys.argv) < 2:
        log("ERROR: No prompt file argument.")
        return

    prompt_path = Path(sys.argv[1]).resolve()
    if not prompt_path.is_file():
        log(f"ERROR: Prompt file not found: {prompt_path}")
        return

    text = prompt_path.read_text(encoding="utf-8", errors="replace")
    log(f"Loaded prompt from {prompt_path} (length={len(text)} chars)")

    open_in_editor(prompt_path)
    run_dispatcher(prompt_path)


if __name__ == "__main__":
    main()
