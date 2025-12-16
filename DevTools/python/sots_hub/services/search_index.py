from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Tuple


@dataclass
class SearchRow:
    id: str
    type: str
    plugin: str
    file: str
    line: int | None
    text: str


def build_search_rows(store, limit: int = 1000) -> List[SearchRow]:
    rows: List[SearchRow] = []
    idx = 0

    for plugin, files in (store.todo_data or {}).items():
        for file_path, entries in files.items():
            for entry in entries:
                idx += 1
                rows.append(
                    SearchRow(
                        id=f"todo-{idx}",
                        type="TODO",
                        plugin=plugin,
                        file=file_path,
                        line=entry.get("line"),
                        text=str(entry.get("text", "")).strip(),
                    )
                )
                if len(rows) >= limit:
                    return rows

    for plugin, files in (store.tag_data or {}).items():
        for file_path, entries in files.items():
            for entry in entries:
                idx += 1
                rows.append(
                    SearchRow(
                        id=f"tag-{idx}",
                        type="TAG",
                        plugin=plugin,
                        file=file_path,
                        line=entry.get("line"),
                        text=str(entry.get("text", "")).strip(),
                    )
                )
                if len(rows) >= limit:
                    return rows

    api_data = getattr(store, "api_data", None) or {}
    if isinstance(api_data, dict) and api_data.get("files"):
        plugin_name = str(api_data.get("plugin") or "unknown")
        for file_path, file_info in (api_data.get("files") or {}).items():
            if not isinstance(file_info, dict):
                continue
            for type_entry in file_info.get("types") or []:
                idx += 1
                kind = str(type_entry.get("kind") or "API")
                name = str(type_entry.get("name") or "").strip()
                rows.append(
                    SearchRow(
                        id=f"api-{idx}",
                        type="API",
                        plugin=plugin_name,
                        file=str(file_path),
                        line=None,
                        text=f"{kind} {name}".strip(),
                    )
                )
                if len(rows) >= limit:
                    return rows

            for fn in file_info.get("functions") or []:
                idx += 1
                name = str(fn.get("name") or "").strip()
                meta = str(fn.get("meta") or "").strip()
                rows.append(
                    SearchRow(
                        id=f"api-{idx}",
                        type="API",
                        plugin=plugin_name,
                        file=str(file_path),
                        line=None,
                        text=f"UFUNCTION {name} ({meta})".strip(),
                    )
                )
                if len(rows) >= limit:
                    return rows

            for prop in file_info.get("properties") or []:
                idx += 1
                decl = str(prop.get("declaration") or "").strip()
                meta = str(prop.get("meta") or "").strip()
                rows.append(
                    SearchRow(
                        id=f"api-{idx}",
                        type="API",
                        plugin=plugin_name,
                        file=str(file_path),
                        line=None,
                        text=f"UPROPERTY {decl} ({meta})".strip(),
                    )
                )
                if len(rows) >= limit:
                    return rows

    return rows


@dataclass
class HotspotEntry:
    plugin: str
    file: str | None
    todo_count: int
    tag_count: int

    @property
    def score(self) -> int:
        return (self.todo_count * 3) + self.tag_count


@dataclass
class HotspotStats:
    top_plugins: List[HotspotEntry]
    top_files: List[HotspotEntry]


def _plugin_counts(store) -> List[HotspotEntry]:
    entries: List[HotspotEntry] = []
    plugin_names = set(store.todo_data.keys()) | set(store.tag_data.keys())
    for plugin in plugin_names:
        todo_count = sum(len(v or []) for v in store.todo_data.get(plugin, {}).values())
        tag_count = sum(len(v or []) for v in store.tag_data.get(plugin, {}).values())
        entries.append(HotspotEntry(plugin=plugin, file=None, todo_count=todo_count, tag_count=tag_count))
    return entries


def _file_counts(store) -> List[HotspotEntry]:
    counts: Dict[Tuple[str, str], HotspotEntry] = {}

    for plugin, files in (store.todo_data or {}).items():
        for file_path, entries in files.items():
            key = (plugin, file_path)
            entry = counts.setdefault(key, HotspotEntry(plugin=plugin, file=file_path, todo_count=0, tag_count=0))
            entry.todo_count += len(entries or [])

    for plugin, files in (store.tag_data or {}).items():
        for file_path, entries in files.items():
            key = (plugin, file_path)
            entry = counts.setdefault(key, HotspotEntry(plugin=plugin, file=file_path, todo_count=0, tag_count=0))
            entry.tag_count += len(entries or [])

    return list(counts.values())


def build_hotspots(store, limit: int = 10) -> HotspotStats:
    plugins = sorted(_plugin_counts(store), key=lambda e: e.score, reverse=True)[:limit]
    files = sorted(_file_counts(store), key=lambda e: e.score, reverse=True)[:limit]
    return HotspotStats(top_plugins=plugins, top_files=files)


def build_hotspot_entries(store) -> tuple[List[HotspotEntry], List[HotspotEntry]]:
    return _plugin_counts(store), _file_counts(store)


def filter_hotspots(entries: List[HotspotEntry], sots_only: bool, contains: str | None, sort_by: str) -> List[HotspotEntry]:
    filtered = []
    needle = (contains or "").lower().strip()
    for entry in entries:
        if sots_only and not entry.plugin.startswith("SOTS_"):
            continue
        if needle and needle not in entry.plugin.lower() and (entry.file and needle not in entry.file.lower()):
            continue
        filtered.append(entry)

    sort_key = {
        "score": lambda e: e.score,
        "todo": lambda e: e.todo_count,
        "tags": lambda e: e.tag_count,
    }.get(sort_by, lambda e: e.score)

    filtered.sort(key=sort_key, reverse=True)
    return filtered
