from __future__ import annotations
from pathlib import Path


def get_tools_root() -> Path:
    return Path(__file__).resolve().parent


def get_project_root() -> Path:
    # Assumes tools live under <ProjectRoot>/DevTools/python
    tools_root = get_tools_root()
    return tools_root.parent.parent


def get_plugins_dir() -> Path:
    return get_project_root() / "Plugins"


def find_plugin_uplugin(plugin_name: str) -> Path | None:
    """Locate a .uplugin file for the given plugin name under the Plugins directory."""
    plugins_dir = get_plugins_dir()
    candidate = plugins_dir / plugin_name / f"{plugin_name}.uplugin"
    if candidate.is_file():
        return candidate

    # Fallback: search recursively for a matching .uplugin
    try:
        for path in plugins_dir.rglob("*.uplugin"):
            if path.name.lower() == f"{plugin_name.lower()}.uplugin":
                return path
    except Exception:
        return None

    return None