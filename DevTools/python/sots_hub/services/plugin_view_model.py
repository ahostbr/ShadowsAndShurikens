from __future__ import annotations

from dataclasses import dataclass
from collections import Counter
from typing import Any, Dict, List, Optional

from .report_store import ReportStore


@dataclass
class PluginDetailPayload:
    name: str
    outbound: List[str]
    inbound: List[str]
    todo_entries: List[Dict[str, Any]]
    tag_entries: List[Dict[str, Any]]
    todo_count: int
    tag_count: int
    api: Dict[str, Any]


class PluginViewModel:
    def __init__(self, store: ReportStore) -> None:
        self.store = store

    def detail_payload(self, plugin_name: str) -> PluginDetailPayload:
        outbound = sorted(self.store.depmap_data.get(plugin_name, {}).get("Dependencies") or [])
        inbound_map = self._build_inbound_map()
        inbound = sorted(inbound_map.get(plugin_name, []))
        todo_entries = self._collect_entries(self.store.todo_data.get(plugin_name, {}))
        tag_entries = self._collect_entries(self.store.tag_data.get(plugin_name, {}))
        api_info = self._collect_api_info(plugin_name)

        return PluginDetailPayload(
            name=plugin_name,
            outbound=outbound,
            inbound=inbound,
            todo_entries=todo_entries,
            tag_entries=tag_entries,
            todo_count=len(todo_entries),
            tag_count=len(tag_entries),
            api=api_info,
        )

    def _build_inbound_map(self) -> Dict[str, List[str]]:
        inbound: Dict[str, List[str]] = {}
        for source, info in self.store.depmap_data.items():
            for target in info.get("Dependencies") or []:
                inbound.setdefault(target, []).append(source)
        return inbound

    def _collect_entries(self, nested: Dict[str, List[Dict[str, Any]]]) -> List[Dict[str, Any]]:
        entries: List[Dict[str, Any]] = []
        for file_path, records in nested.items():
            for record in records:
                entries.append(
                    {
                        "file": file_path,
                        "line": record.get("line"),
                        "text": record.get("text", "").strip(),
                    }
                )
        entries.sort(key=lambda item: (item["file"], item["line"] or 0))
        return entries

    def _collect_api_info(self, plugin_name: str) -> Dict[str, Any]:
        api_data = self.store.api_data
        if not api_data:
            return {
                "available": False,
                "note": "API surface report not loaded. Run sots_api_surface.py to capture this plugin.",
            }
        loaded_plugin = api_data.get("plugin")
        if loaded_plugin != plugin_name:
            return {
                "available": False,
                "note": f"API surface snapshot currently loaded for: {loaded_plugin or 'unknown'}. Run api surface scan for this plugin.",
            }

        type_counts: Counter[str] = Counter()
        functions = 0
        properties = 0
        for file_data in api_data.get("files", {}).values():
            for entry in file_data.get("types", []):
                kind = entry.get("kind")
                if kind:
                    type_counts[kind] += 1
            functions += len(file_data.get("functions", []))
            properties += len(file_data.get("properties", []))

        return {
            "available": True,
            "plugin": plugin_name,
            "headers": api_data.get("total_files", 0),
            "types": dict(type_counts),
            "functions": functions,
            "properties": properties,
            "note": "",
        }
