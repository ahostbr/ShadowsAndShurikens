from __future__ import annotations
import json
import shlex
import subprocess
from datetime import datetime
from pathlib import Path

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit
from analyze_build_log import extract_errors, guess_plugins_from_errors


def load_config() -> dict:
    cfg_path = Path(__file__).resolve().parent / "sots_tools_config.json"
    if not cfg_path.exists():
        return {}
    try:
        with cfg_path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"[WARN] Failed to read config: {e}")
        return {}


def main() -> None:
    confirm_start("run_build_and_analyze")

    tools_root = Path(__file__).resolve().parent
    project_root = get_project_root()
    cfg = load_config()

    build_cmd = cfg.get("build_command", "").strip()
    if not build_cmd:
        print("[ERROR] No build_command set in sots_tools_config.json")
        print('Example (cmd.exe): "build_command": "Engine/Build/BatchFiles/Build.bat ShadowsAndShurikensEditor Win64 Development"')
        print_llm_summary(
            "run_build_and_analyze",
            BUILD_SUCCESS=False,
            ERROR="NO_BUILD_COMMAND",
        )
        confirm_exit()
        return

    working_dir_str = cfg.get("build_working_dir", "").strip()
    if working_dir_str:
        working_dir = Path(working_dir_str).resolve()
    else:
        working_dir = project_root

    log_root_str = cfg.get("build_log_output", "").strip()
    if log_root_str:
        log_root = Path(log_root_str).resolve()
    else:
        log_root = project_root / "Saved" / "Logs" / "DevTools"

    log_root.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = log_root / f"SOTS_DevTools_Build_{stamp}.log"

    print(f"[INFO] Project root: {project_root}")
    print(f"[INFO] Working dir:  {working_dir}")
    print(f"[INFO] Command:      {build_cmd}")
    print(f"[INFO] Log output:   {log_path}")

    try:
        proc = subprocess.run(
            shlex.split(build_cmd),
            cwd=working_dir,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError:
        print("[ERROR] Build command executable not found.")
        print_llm_summary(
            "run_build_and_analyze",
            BUILD_SUCCESS=False,
            ERROR="FILE_NOT_FOUND",
            CMD=build_cmd,
        )
        confirm_exit()
        return

    combined = (proc.stdout or "") + "\n" + (proc.stderr or "")
    log_path.write_text(combined, encoding="utf-8", errors="ignore")

    success = (proc.returncode == 0)

    print("")
    print("=== BUILD RESULT ===")
    print(f"Return code: {proc.returncode}")
    print(f"Log file:    {log_path}")
    print(f"Success:     {success}")

    errors = []
    plugins = set()
    if not success:
        lines = combined.splitlines()
        errors = extract_errors(lines, max_errors=40)
        plugins = guess_plugins_from_errors(errors)

        if errors:
            print("\n[ERRORS (first few)]")
            for e in errors[:10]:
                print("  ", e)

        if plugins:
            print("\n[PLUGINS REFERENCED]")
            for p in sorted(plugins):
                print("  ", p)

    print_llm_summary(
        "run_build_and_analyze",
        BUILD_SUCCESS=success,
        RETURN_CODE=proc.returncode,
        LOG=str(log_path),
        ERROR_COUNT=len(errors),
        PLUGINS=",".join(sorted(plugins)) if plugins else "",
    )

    confirm_exit()


if __name__ == "__main__":
    main()
