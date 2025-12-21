from __future__ import annotations

from typing import Dict, List


def build_plugin_detail(plugin: str, depmap: dict, todo: dict, tags: dict, api: dict) -> dict:
    info = depmap.get(plugin, {}) if isinstance(depmap, dict) else {}
    outbound = info.get("Dependencies") or []
    inbound: List[str] = []
    for name, entry in (depmap or {}).items():
        deps = entry.get("Dependencies") or []
        if plugin in deps:
            inbound.append(name)

    todo_files = todo.get(plugin, {}) if isinstance(todo, dict) else {}
    tag_files = tags.get(plugin, {}) if isinstance(tags, dict) else {}
    api_files = {}
    files_section = api.get("files") if isinstance(api, dict) else {}
    if plugin == api.get("plugin"):
        api_files = files_section or {}

    return {
        "plugin": plugin,
        "outbound": sorted(outbound),
        "inbound": sorted(inbound),
        "todo": todo_files,
        "tags": tag_files,
        "api": api_files,
    }
