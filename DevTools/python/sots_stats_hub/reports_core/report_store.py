from __future__ import annotations

import json
import time
from pathlib import Path
from typing import Dict, List, Tuple

from .models import ApiRow, PluginRow, SearchRow, TagRow, TodoRow
from .paths import pins_path, report_paths
from .plugin_view import build_plugin_detail
from .search_index import build_search_index, build_hotspots


def _load_json(path: Path) -> Tuple[dict, float, str | None]:
    if not path.exists():
        return {}, 0.0, "missing"
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        mtime = path.stat().st_mtime
        return data, mtime, None
    except Exception as exc:  # noqa: BLE001
        return {}, time.time(), str(exc)


class ReportStore:
    def __init__(self, project_root: Path) -> None:
        self.project_root = project_root
        self.paths = report_paths(project_root)
        self.depmap: dict = {}
        self.todo: dict = {}
        self.tags: dict = {}
        self.api: dict = {}
        self.mtimes: Dict[str, float] = {}
        self.errors: Dict[str, str | None] = {k: "missing" for k in self.paths}
        self.pins = self._load_pins()
        self.plugin_rows: List[PluginRow] = []
        self.search_rows: List[SearchRow] = []
        self.hot_plugins: List[Tuple[str, int]] = []
        self.hot_files: List[Tuple[str, int]] = []
        self.refresh(force=True)

    def _load_pins(self) -> List[str]:
        p = pins_path(self.project_root)
        if not p.exists():
            return []
        try:
            obj = json.loads(p.read_text(encoding="utf-8"))
            if isinstance(obj, list):
                return [str(x) for x in obj]
        except Exception:
            pass
        return []

    def save_pins(self) -> None:
        p = pins_path(self.project_root)
        try:
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(json.dumps(self.pins, indent=2), encoding="utf-8")
        except Exception:
            pass

    def has_changes(self) -> bool:
        for key, path in self.paths.items():
            try:
                mtime = path.stat().st_mtime
            except Exception:
                mtime = 0.0
            if self.mtimes.get(key, 0.0) != mtime:
                return True
        return False

    def refresh(self, force: bool = False) -> bool:
        changed = False
        for key, path in self.paths.items():
            try:
                mtime = path.stat().st_mtime
            except Exception:
                mtime = 0.0
            if not force and self.mtimes.get(key, 0.0) == mtime:
                continue
            data, m, err = _load_json(path)
            setattr(self, key if key != "tags" else "tags", data)
            self.mtimes[key] = m
            self.errors[key] = err
            changed = True

        if changed or force:
            self._rebuild_views()
        return changed

    def _rebuild_views(self) -> None:
        depmap = self.depmap or {}
        todo = self.todo.get("backlog", {}) if isinstance(self.todo, dict) else {}
        tags = self.tags.get("report", {}) if isinstance(self.tags, dict) else {}
        api_data = self.api.get("files", {}) if isinstance(self.api, dict) else {}
        plugin_scores: Dict[str, Dict[str, int]] = {}
        inbound_map: Dict[str, int] = {}
        outbound_map: Dict[str, int] = {}

        for plugin, info in depmap.items():
            deps = info.get("Dependencies") or []
            outbound_map[plugin] = len(deps)
            for dep in deps:
                inbound_map[dep] = inbound_map.get(dep, 0) + 1

        todo_counts: Dict[str, int] = {}
        for plugin, files in todo.items():
            count = sum(len(v) for v in (files or {}).values())
            todo_counts[plugin] = count
            plugin_scores.setdefault(plugin, {})["todo"] = count

        tag_counts: Dict[str, int] = {}
        for plugin, files in tags.items():
            count = sum(len(v) for v in (files or {}).values())
            tag_counts[plugin] = count
            plugin_scores.setdefault(plugin, {})["tags"] = count

        plugins = set(depmap.keys()) | set(inbound_map.keys()) | set(todo_counts.keys()) | set(tag_counts.keys())
        rows: List[PluginRow] = []
        for name in sorted(plugins):
            inbound = inbound_map.get(name, 0)
            outbound = outbound_map.get(name, 0)
            todo_c = todo_counts.get(name, 0)
            tags_c = tag_counts.get(name, 0)
            score = todo_c + tags_c
            rows.append(PluginRow(name=name, inbound=inbound, outbound=outbound, todo=todo_c, tags=tags_c, score=score, pinned=name in self.pins))
        rows.sort(key=lambda r: (not r.pinned, -r.score, r.name.lower()))
        self.plugin_rows = rows

        todos: List[TodoRow] = []
        for plugin, files in todo.items():
            for fname, matches in (files or {}).items():
                for m in matches:
                    todos.append(TodoRow(plugin=plugin, file=fname, line=int(m.get("line", 0)), text=str(m.get("text", ""))))

        tag_rows: List[TagRow] = []
        for plugin, files in tags.items():
            for fname, matches in (files or {}).items():
                for m in matches:
                    tag_rows.append(TagRow(plugin=plugin, file=fname, line=int(m.get("line", 0)), text=str(m.get("text", ""))))

        api_rows: List[ApiRow] = []
        api_plugin = self.api.get("plugin") if isinstance(self.api, dict) else None
        for fname, entry in api_data.items():
            plugin = api_plugin or "API"
            for t in entry.get("types", []):
                api_rows.append(ApiRow(plugin=plugin, file=fname, kind=t.get("kind", ""), symbol=t.get("name", ""), header=""))
            for f in entry.get("functions", []):
                api_rows.append(ApiRow(plugin=plugin, file=fname, kind="Function", symbol=f.get("name", ""), header=f.get("declaration", "")))
            for p in entry.get("properties", []):
                api_rows.append(ApiRow(plugin=plugin, file=fname, kind="Property", symbol=p.get("declaration", ""), header=p.get("meta", "")))

        self.search_rows = build_search_index(todos, tag_rows, api_rows)
        self.hot_plugins, self.hot_files = build_hotspots(todos, tag_rows)

    def plugin_detail(self, plugin: str) -> dict:
        return build_plugin_detail(plugin, self.depmap or {}, self.todo.get("backlog", {}) if isinstance(self.todo, dict) else {}, self.tags.get("report", {}) if isinstance(self.tags, dict) else {}, self.api)
