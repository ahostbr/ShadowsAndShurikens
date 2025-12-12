from __future__ import annotations
from pathlib import Path
from typing import Iterable, List, Optional

from project_paths import find_plugin_uplugin, get_plugins_dir
from file_ops import edit_json_file, load_json


def ensure_plugin_dependencies(
    uplugin_path: Path,
    dependencies: Iterable[str],
    verbose: bool = True,
    dry_run: bool = False,
) -> bool:
    deps_list = list(dependencies)

    def editor(data: dict) -> bool:
        changed = False
        plugins_array = data.get("Plugins")
        if plugins_array is None:
            plugins_array = []
            data["Plugins"] = plugins_array
            changed = True

        existing = {p.get("Name") for p in plugins_array if isinstance(p, dict)}

        for dep in deps_list:
            if dep in existing:
                continue
            if verbose:
                print(f"  - Adding dependency '{dep}'")
            plugins_array.append({"Name": dep, "Enabled": True})
            changed = True

        return changed

    return edit_json_file(uplugin_path, editor, dry_run=dry_run)


def ensure_plugin_dependencies_by_name(
    plugin_name: str,
    dependencies: Iterable[str],
    verbose: bool = True,
    dry_run: bool = False,
) -> bool:
    uplugin_path = find_plugin_uplugin(plugin_name)
    if not uplugin_path:
        print(f"[WARN] Could not find .uplugin for plugin '{plugin_name}' under {get_plugins_dir()}")
        return False

    if verbose:
        print(f"[EDIT] {uplugin_path}")
    return ensure_plugin_dependencies(uplugin_path, dependencies, verbose=verbose, dry_run=dry_run)


def get_plugin_version_name(plugin_name: str) -> Optional[str]:
    uplugin_path = find_plugin_uplugin(plugin_name)
    if not uplugin_path:
        return None

    data = load_json(uplugin_path)
    version_name = data.get("VersionName")
    if isinstance(version_name, str):
        return version_name
    return None
