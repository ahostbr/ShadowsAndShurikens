#!/usr/bin/env python
"""
enable_vscode_shell_integration.py (simple version)

- Finds your PowerShell $PROFILE
- Ensures this line is present in the file:

    if ($env:TERM_PROGRAM -eq "vscode") { . "$(code --locate-shell-integration-path pwsh)" }

- Writes detailed logs to enable_vscode_shell_integration.log
"""

import shutil
import subprocess
import sys
from pathlib import Path
from datetime import datetime

SCRIPT_PATH = Path(__file__).resolve()
LOG_PATH = SCRIPT_PATH.with_suffix(".log")


# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------

def log(message: str, level: str = "INFO") -> None:
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}][{level}] {message}"
    print(line)
    try:
        with LOG_PATH.open("a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception as e:
        print(f"[{ts}][LOG-ERROR] Failed to write to log file {LOG_PATH}: {e}")


def start_log_header() -> None:
    sep = "-" * 70
    log(sep)
    log(f"New run of enable_vscode_shell_integration.py (Python {sys.version.split()[0]})")
    log(f"Script: {SCRIPT_PATH}")
    log(f"Log:    {LOG_PATH}")
    log(sep)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def run_cmd(cmd):
    log(f"Running command: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True, shell=False)
    rc = result.returncode
    out = result.stdout.strip()
    err = result.stderr.strip()
    log(f"Command return code: {rc}")
    log(f"Command stdout:\n{out or '<empty>'}")
    log(f"Command stderr:\n{err or '<empty>'}")
    return rc, out, err


def get_powershell_profile(shell_exe: str) -> Path | None:
    """
    Ask the given PowerShell executable for its $PROFILE path.
    """
    log(f"Querying $PROFILE from '{shell_exe}'...")
    rc, out, err = run_cmd([shell_exe, "-NoLogo", "-NoProfile", "-Command", "$PROFILE"])
    if rc != 0 or not out:
        log(f"Failed to get $PROFILE from {shell_exe}.", level="WARN")
        return None

    profile_path = Path(out.strip('"')).expanduser()
    log(f"{shell_exe} profile path: {profile_path}", level="OK")
    return profile_path


def ensure_profile_contains_line(profile_path: Path, line: str) -> None:
    """
    Ensure the given line exists in the profile file.
    """
    log(f"Ensuring profile directory exists: {profile_path.parent}")
    profile_path.parent.mkdir(parents=True, exist_ok=True)

    if profile_path.exists():
        log(f"Existing profile found at: {profile_path}")
        original = profile_path.read_text(encoding="utf-8", errors="ignore")

        if line in original:
            log("Shell integration line already present in profile. Nothing to change.", level="OK")
            return

        backup = profile_path.with_suffix(profile_path.suffix + ".bak")
        backup.write_text(original, encoding="utf-8")
        log(f"Backup of existing profile created at: {backup}", level="OK")

        new_text = original.rstrip() + "\n\n# Added by enable_vscode_shell_integration.py\n" + line + "\n"
        profile_path.write_text(new_text, encoding="utf-8")
        log("Shell integration line appended to existing profile.", level="OK")
    else:
        log(f"No existing profile found. Creating new profile at: {profile_path}")
        new_text = (
            "# PowerShell profile created by enable_vscode_shell_integration.py\n"
            + line
            + "\n"
        )
        profile_path.write_text(new_text, encoding="utf-8")
        log("New profile created with shell integration line.", level="OK")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    start_log_header()
    log("=== VS Code + PowerShell Shell Integration Helper (simple) ===")

    if sys.platform != "win32":
        log("This script is intended for Windows only.", level="ERROR")
        sys.exit(1)

    # We keep this check so the inserted line isn't obviously broken.
    if shutil.which("code") is None:
        log("VS Code CLI 'code' not found in PATH.", level="ERROR")
        log("In VS Code: run Command Palette → 'Shell Command: Install \"code\" command in PATH'", level="ERROR")
        sys.exit(1)
    else:
        log("Found VS Code CLI 'code' in PATH.", level="OK")

    profile_path = None
    for shell in ("pwsh", "powershell"):
        if shutil.which(shell):
            log(f"Trying shell: {shell}")
            profile_path = get_powershell_profile(shell)
            if profile_path is not None:
                log(f"Using profile from shell: {shell}")
                break
        else:
            log(f"Shell '{shell}' not found on PATH, skipping.", level="INFO")

    if profile_path is None:
        log("Could not resolve a PowerShell profile ($PROFILE).", level="ERROR")
        sys.exit(1)

    integration_line = 'if ($env:TERM_PROGRAM -eq "vscode") { . "$(code --locate-shell-integration-path pwsh)" }'
    ensure_profile_contains_line(profile_path, integration_line)

    log("=== All done ===", level="OK")
    log("Next steps:", level="INFO")
    log("1. Fully close ALL VS Code windows.", level="INFO")
    log("2. Reopen VS Code.", level="INFO")
    log("3. Open a NEW PowerShell terminal in VS Code.", level="INFO")
    log("4. Hover the terminal tab: it should say Shell integration: Rich or Basic.", level="INFO")
    log("If Cline still nags, but the terminal tab shows Rich/Basic, the integration is actually working.", level="INFO")


if __name__ == "__main__":
    main()
