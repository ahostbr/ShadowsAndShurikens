from __future__ import annotations

import os
import platform
from pathlib import Path

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def find_uproject(project_root: Path) -> Path | None:
    """
    Find the .uproject file for this project.

    Preference:
      1) <project_root.name>.uproject at the root
      2) First *.uproject found at the root
    """
    root_name = project_root.name
    preferred = project_root / f"{root_name}.uproject"
    if preferred.exists():
        return preferred

    candidates = list(project_root.glob("*.uproject"))
    if candidates:
        return candidates[0]

    return None


def main() -> None:
    confirm_start("run_unreal_project")

    project_root = get_project_root()
    print(f"[INFO] Project root: {project_root}")

    uproject = find_uproject(project_root)
    if not uproject:
        print("[ERROR] No .uproject file found at project root.")
        print_llm_summary(
            "run_unreal_project",
            status="ERROR",
            PROJECT_ROOT=str(project_root),
            UPROJECT_FOUND=False,
        )
        confirm_exit()
        return

    print(f"[INFO] Using .uproject: {uproject}")

    system_name = platform.system()
    launched = False
    error_msg: str | None = None

    if system_name == "Windows":
        try:
            # Use default Windows association for .uproject to launch Unreal Editor
            os.startfile(str(uproject))
            launched = True
            print("[INFO] Unreal project launch requested via os.startfile().")
        except Exception as exc:  # noqa: BLE001
            error_msg = f"{type(exc).__name__}: {exc}"
            print(f"[ERROR] Failed to launch .uproject: {error_msg}")
    else:
        # Non-Windows: we don't know the UnrealEditor path; just log a warning.
        error_msg = (
            "Non-Windows platform detected. "
            "Automatic launch via .uproject is only implemented for Windows. "
            "Please open the .uproject manually or via your UnrealEditor binary."
        )
        print(f"[WARN] {error_msg}")

    status = "OK" if launched else "WARN"

    print_llm_summary(
        "run_unreal_project",
        status=status,
        PROJECT_ROOT=str(project_root),
        UPROJECT_PATH=str(uproject),
        PLATFORM=system_name,
        LAUNCHED=launched,
        ERROR=error_msg,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
