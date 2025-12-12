from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Dict, List, Tuple

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


# ---------- Plugin scanning ----------

def find_plugins(plugin_root: Path) -> List[Path]:
    """
    Return a list of .uplugin file paths under plugin_root.
    """
    if not plugin_root.exists():
        return []
    return list(plugin_root.glob("*/**/*.uplugin"))


def analyze_plugin_binaries(plugin_root: Path, uplugins: List[Path]) -> Tuple[int, int, int, int]:
    """
    For each plugin, determine if it has Binaries/ and/or Intermediate/.
    Returns:
        total_plugins,
        plugins_with_binaries,
        plugins_with_intermediate,
        total_bin_int_folders
    """
    total = len(uplugins)
    with_bin = 0
    with_intermediate = 0
    total_bin_int = 0

    for uplug in uplugins:
        plugin_dir = uplug.parent
        binaries = plugin_dir / "Binaries"
        intermediate = plugin_dir / "Intermediate"

        has_bin = binaries.exists()
        has_int = intermediate.exists()

        if has_bin:
            with_bin += 1
            total_bin_int += 1
        if has_int:
            with_intermediate += 1
            total_bin_int += 1

    return total, with_bin, with_intermediate, total_bin_int


def analyze_runtime_modules(plugin_root: Path, uplugins: List[Path]) -> Dict[str, int]:
    """
    Scan each plugin's .uplugin for runtime modules and check if they have an
    IMPLEMENT_MODULE for that module name somewhere in Source/<ModuleName>/Private/*.cpp.

    Returns a dict with:
      RUNTIME_MODULES_TOTAL
      RUNTIME_MODULES_OK
      RUNTIME_MODULES_MISSING_PRIVATE_DIR
      RUNTIME_MODULES_MISSING_CPP
    """
    runtime_modules_total = 0
    runtime_modules_ok = 0
    runtime_modules_missing_private = 0
    runtime_modules_missing_cpp = 0

    for uplug in uplugins:
        plugin_dir = uplug.parent
        try:
            data = json.loads(uplug.read_text(encoding="utf-8"))
        except Exception:
            # Skip malformed or non-JSON plugin descriptors
            continue

        modules = data.get("Modules", [])
        for mod in modules:
            if mod.get("Type") != "Runtime":
                continue

            module_name = mod.get("Name")
            if not module_name:
                continue

            runtime_modules_total += 1

            module_source_dir = plugin_dir / "Source" / module_name
            private_dir = module_source_dir / "Private"

            if not private_dir.exists():
                runtime_modules_missing_private += 1
                continue

            found_impl = False
            for cpp in private_dir.glob("*.cpp"):
                try:
                    text = cpp.read_text(encoding="utf-8", errors="ignore")
                except Exception:
                    continue
                if "IMPLEMENT_MODULE" in text and module_name in text:
                    found_impl = True
                    break

            if found_impl:
                runtime_modules_ok += 1
            else:
                runtime_modules_missing_cpp += 1

    return {
        "RUNTIME_MODULES_TOTAL": runtime_modules_total,
        "RUNTIME_MODULES_OK": runtime_modules_ok,
        "RUNTIME_MODULES_MISSING_PRIVATE_DIR": runtime_modules_missing_private,
        "RUNTIME_MODULES_MISSING_CPP": runtime_modules_missing_cpp,
    }


# ---------- FSOTS struct scanning ----------

FSOTS_STRUCT_RE = re.compile(r"\bstruct\s+(FSOTS_[A-Za-z0-9_]+)")


def scan_fsots_structs(root: Path) -> Tuple[int, Dict[str, List[Path]]]:
    """
    Scan for 'struct FSOTS_*' definitions under root in .h/.hpp/.cpp files.

    Returns:
      total_unique_structs,
      name_to_files_map
    """
    name_to_files: Dict[str, List[Path]] = {}

    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in {".h", ".hpp", ".cpp"}:
            continue

        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue

        for match in FSOTS_STRUCT_RE.finditer(text):
            name = match.group(1)
            name_to_files.setdefault(name, []).append(path)

    total_unique = len(name_to_files)
    return total_unique, name_to_files


def summarize_fsots_dupes(name_to_files: Dict[str, List[Path]]) -> Tuple[bool, int, List[str]]:
    """
    Given name_to_files map, determine if there are duplicates.
    Returns:
      has_dupes,
      duplicate_count,
      duplicate_names (list)
    """
    dup_names: List[str] = []
    for name, files in name_to_files.items():
        if len(files) > 1:
            dup_names.append(name)

    has_dupes = len(dup_names) > 0
    dup_count = len(dup_names)
    dup_names_sorted = sorted(dup_names)
    return has_dupes, dup_count, dup_names_sorted


# ---------- Main health report ----------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate a high-level health report for the SOTS project."
    )
    parser.add_argument(
        "--root",
        type=str,
        default="",
        help="Project root (defaults to auto-detected project root).",
    )
    args = parser.parse_args()

    confirm_start("project_health_report")

    project_root = Path(args.root).resolve() if args.root else get_project_root()
    plugin_root = project_root / "Plugins"

    print(f"[INFO] Project root: {project_root}")
    print(f"[INFO] Plugin root:  {plugin_root}")

    # --- Plugin scan ---
    uplugins = find_plugins(plugin_root)
    total_plugins, with_bin, with_int, total_bin_int = analyze_plugin_binaries(plugin_root, uplugins)
    module_stats = analyze_runtime_modules(plugin_root, uplugins)

    # --- FSOTS scan ---
    fsots_total, name_to_files = scan_fsots_structs(project_root)
    fsots_has_dupes, fsots_dup_count, fsots_dup_names = summarize_fsots_dupes(name_to_files)

    # --- Human-readable summary ---
    print("\n=== Project Health Summary ===")
    print(f"Project root: {project_root}")
    print(f"Plugins found (.uplugin): {total_plugins}")
    print(f"Plugins with Binaries: {with_bin}/{total_plugins}")
    print(f"Plugins with Intermediate: {with_int}/{total_plugins}")
    print(f"Total Binaries/Intermediate folders: {total_bin_int}")

    print("\nRuntime modules:")
    print(f"  Total runtime modules: {module_stats['RUNTIME_MODULES_TOTAL']}")
    print(f"  OK (have IMPLEMENT_MODULE): {module_stats['RUNTIME_MODULES_OK']}")
    print(f"  Missing Private/ dir: {module_stats['RUNTIME_MODULES_MISSING_PRIVATE_DIR']}")
    print(f"  Missing module IMPLEMENT_MODULE: {module_stats['RUNTIME_MODULES_MISSING_CPP']}")

    print("\nFSOTS_* struct definitions:")
    print(f"  Unique struct names: {fsots_total}")
    print(f"  Duplicate struct names: {fsots_dup_count}")
    print(f"  Has duplicates: {fsots_has_dupes}")
    if fsots_has_dupes:
        # Show a small sample of duplicates
        sample = fsots_dup_names[:10]
        print("  Duplicate names (sample):")
        for name in sample:
            files = ", ".join(str(p.relative_to(project_root)) for p in name_to_files[name])
            print(f"    {name}: {files}")

    # --- Status determination ---
    status = "OK"
    if fsots_has_dupes or module_stats["RUNTIME_MODULES_MISSING_CPP"] > 0 or module_stats["RUNTIME_MODULES_MISSING_PRIVATE_DIR"] > 0:
        status = "WARN"

    # --- LLM log summary ---
    print()
    print_llm_summary(
        "project_health_report",
        status=status,
        PROJECT_ROOT=str(project_root),
        PLUGIN_ROOT=str(plugin_root),
        TOTAL_PLUGINS=total_plugins,
        PLUGINS_WITH_BINARIES=with_bin,
        PLUGINS_WITH_INTERMEDIATE=with_int,
        BIN_INT_FOLDERS=total_bin_int,
        RUNTIME_MODULES_TOTAL=module_stats["RUNTIME_MODULES_TOTAL"],
        RUNTIME_MODULES_OK=module_stats["RUNTIME_MODULES_OK"],
        RUNTIME_MODULES_MISSING_PRIVATE_DIR=module_stats["RUNTIME_MODULES_MISSING_PRIVATE_DIR"],
        RUNTIME_MODULES_MISSING_CPP=module_stats["RUNTIME_MODULES_MISSING_CPP"],
        FSOTS_SCAN_ROOT=str(project_root),
        FSOTS_STRUCTS_TOTAL=fsots_total,
        FSOTS_DUPLICATE_COUNT=fsots_dup_count,
        FSOTS_DUPLICATE_NAMES=fsots_dup_names,
        FSOTS_HAS_DUPES=fsots_has_dupes,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
