from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from pathlib import Path
from typing import List, Tuple

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


TARGET_DIR_NAMES = {"Binaries", "Intermediate"}


def load_config(tools_root: Path) -> dict:
    cfg_path = tools_root / "sots_tools_config.json"
    if not cfg_path.exists():
        return {}
    try:
        with cfg_path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"[WARN] Failed to read config: {e}")
        return {}


def find_plugin_trash(plugin_root: Path) -> List[Path]:
    """Find all Binaries/Intermediate folders under Plugins/."""
    if not plugin_root.exists():
        return []

    results: List[Path] = []
    for p in plugin_root.rglob("*"):
        if p.is_dir() and p.name in TARGET_DIR_NAMES:
            results.append(p)
    return results


def get_root_trash(project_root: Path) -> Tuple[List[Path], List[Path]]:
    """Returns (root_dirs, root_files) that should be deleted at the project root."""
    dirs = [
        project_root / ".vs",
        project_root / "Binaries",
        project_root / "DerivedDataCache",
        project_root / "Intermediate",
    ]

    files = [
        project_root / ".vsconfig",
    ]

    root_dirs = [d for d in dirs if d.exists()]
    root_files = [f for f in files if f.exists()]

    return root_dirs, root_files


def delete_path(path: Path, dry_run: bool) -> None:
    if dry_run:
        print(f"[DRY-RUN] Would delete: {path}")
        return

    if path.is_dir():
        print(f"[DEL] Directory: {path}")
        shutil.rmtree(path, ignore_errors=True)
    elif path.is_file():
        print(f"[DEL] File: {path}")
        try:
            path.unlink()
        except Exception as e:
            print(f"[WARN] Failed to delete {path}: {e}")


def find_uproject(project_root: Path) -> Path | None:
    ups = list(project_root.glob("*.uproject"))
    if not ups:
        return None
    # Assume main project is the first one
    return ups[0]


def infer_generate_command_from_build(build_command: str, uproject: Path) -> str | None:
    """
    Try to infer a GenerateProjectFiles.bat call from the configured build_command.
    Assumes build_command contains 'Build.bat'.
    """
    if not build_command:
        return None

    lower = build_command.lower()
    idx = lower.find("build.bat")
    if idx == -1:
        return None

    prefix = build_command[:idx]  # up to the 'Build.bat'
    prefix = prefix.strip().strip('"').rstrip("\\/")

    normalized = prefix.replace("\\", "/").lower()
    token = "/engine/build/batchfiles"
    engine_root: str

    if normalized.endswith(token):
        engine_root = prefix[: -len(token)]
    else:
        # Fallback: strip trailing '\Build.bat' only
        engine_root = prefix.rsplit("Build", 1)[0]

    generate_bat = Path(engine_root) / "GenerateProjectFiles.bat"
    return f"\"{generate_bat}\" -project=\"{uproject}\" -game -engine"


def _extract_executable_path_from_command(command: str) -> Path | None:
    """
    Best-effort: pull out the executable path from a shell command string.
    Handles:
      - \"C:/Path/To/Exe.exe\" args...
      - C:/Path/To/Exe.exe args...
    """
    cmd = command.strip()
    if not cmd:
        return None

    if cmd[0] in ("\"", "'"):
        quote = cmd[0]
        try:
            end = cmd.index(quote, 1)
        except ValueError:
            return None
        exe = cmd[1:end]
        return Path(exe)
    else:
        # Up to first space
        parts = cmd.split()
        if not parts:
            return None
        return Path(parts[0])


def run_generate_project_files(project_root: Path,
                               tools_root: Path,
                               dry_run: bool) -> tuple[bool, str]:
    """
    Attempt to run GenerateProjectFiles for the .uproject.
    Returns (ran_successfully, status_code_string)
    """
    uproject = find_uproject(project_root)
    if not uproject:
        print("[WARN] No .uproject found at project root; skipping GenerateProjectFiles.")
        return False, "NO_UPROJECT"

    cfg = load_config(tools_root)
    gen_cmd = cfg.get("generate_project_files_command", "").strip()

    if not gen_cmd:
        # Try to infer from build_command, if present
        build_cmd = cfg.get("build_command", "").strip()
        inferred = infer_generate_command_from_build(build_cmd, uproject)
        if inferred:
            gen_cmd = inferred
            print(f"[INFO] Inferred GenerateProjectFiles command from build_command:\n  {gen_cmd}")
        else:
            print("[WARN] No generate_project_files_command and could not infer from build_command; skipping GenerateProjectFiles.")
            return False, "NO_COMMAND"
    else:
        print(f"[INFO] Using configured GenerateProjectFiles command:\n  {gen_cmd}")

    exe_path = _extract_executable_path_from_command(gen_cmd)
    if exe_path is not None and not exe_path.exists():
        print(f"[WARN] GenerateProjectFiles executable not found at {exe_path}.")
        print("[WARN] Skipping GenerateProjectFiles step (command is configured but exe doesn't exist).")
        return False, "EXE_MISSING"

    if dry_run:
        print("[DRY-RUN] Would run GenerateProjectFiles now.")
        return False, "DRY_RUN"

    try:
        result = subprocess.run(gen_cmd, shell=True)
        if result.returncode != 0:
            print(f"[WARN] GenerateProjectFiles returned code {result.returncode}")
            return False, f"EXIT_{result.returncode}"
    except Exception as e:
        print(f"[WARN] Failed to run GenerateProjectFiles: {e}")
        return False, "EXCEPTION"

    print("[INFO] GenerateProjectFiles completed successfully.")
    return True, "OK"


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Clean project & plugin build/cache folders and regenerate VS project files."
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Only show what would be deleted / run; do not actually delete or regenerate.",
    )
    args = parser.parse_args()

    confirm_start("clean_build_caches_and_regen_vs")

    project_root = get_project_root()
    tools_root = Path(__file__).resolve().parent
    plugin_root = project_root / "Plugins"

    print(f"[INFO] Project root: {project_root}")
    print(f"[INFO] Plugin root:  {plugin_root}")
    print(f"[INFO] Dry run:      {args.dry_run}")
    print("=== Scanning for trash ===")

    # Root-level targets
    root_dirs, root_files = get_root_trash(project_root)

    # Plugin-level targets
    plugin_trash = find_plugin_trash(plugin_root)

    # Report
    print(f"[INFO] Root dirs to delete: {len(root_dirs)}")
    for d in root_dirs:
        print(f"  - {d}")

    print(f"[INFO] Root files to delete: {len(root_files)}")
    for f in root_files:
        print(f"  - {f}")

    print(f"[INFO] Plugin Binaries/Intermediate folders: {len(plugin_trash)}")
    for d in plugin_trash:
        print(f"  - {d}")

    # Delete
    total_deleted = 0

    for d in root_dirs:
        delete_path(d, args.dry_run)
        total_deleted += 1

    for f in root_files:
        delete_path(f, args.dry_run)
        total_deleted += 1

    for d in plugin_trash:
        delete_path(d, args.dry_run)
        total_deleted += 1

    print("\n[STEP] Regenerating Visual Studio project files...")
    gen_ok, gen_status = run_generate_project_files(project_root, tools_root, args.dry_run)

    print("\n[SUMMARY]")
    print(f"  Root dirs found:           {len(root_dirs)}")
    print(f"  Root files found:          {len(root_files)}")
    print(f"  Plugin dirs found:         {len(plugin_trash)}")
    print(f"  Total items processed:     {total_deleted}")
    print(f"  Dry run:                   {args.dry_run}")
    print(f"  GenerateProjectFiles run:  {gen_ok} (status={gen_status})")

    print_llm_summary(
        "clean_build_caches_and_regen_vs",
        PROJECT_ROOT=str(project_root),
        PLUGIN_ROOT=str(plugin_root),
        ROOT_DIRS=len(root_dirs),
        ROOT_FILES=len(root_files),
        PLUGIN_DIRS=len(plugin_trash),
        ITEMS_PROCESSED=total_deleted,
        DRY_RUN=args.dry_run,
        GEN_RAN=gen_ok,
        GEN_STATUS=gen_status,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
