from __future__ import annotations

import json
import logging
from collections import Counter
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional

from .config import REPORT_SPECS, resolve_report_path


@dataclass
class ReportStatus:
    key: str
    title: str
    json_path: Path
    log_path: Path
    present: bool = False
    error: Optional[str] = None
    data: Any = None
    log_text: Optional[str] = None
    summary: Dict[str, Any] = field(default_factory=dict)


@dataclass
class PluginSnapshot:
    name: str
    dependencies: int = 0
    modules: int = 0
    todo_items: int = 0
    tag_matches: int = 0


@dataclass
class SearchItem:
    id: str
    source: str
    plugin: str
    file: str
    text: str


@dataclass
class ReportBundle:
    reports: Dict[str, ReportStatus]
    plugin_rows: List[PluginSnapshot]
    search_items: List[SearchItem]
    errors: List[str]
    loaded_at: datetime


def _read_json(path: Path) -> tuple[Optional[Any], Optional[str]]:
    if not path.exists():
        return None, "Not found"
    try:
        return json.loads(path.read_text(encoding="utf-8")), None
    except Exception as exc:  # pragma: no cover - defensive
        return None, f"Failed to parse JSON: {exc}"


def _read_log(path: Path) -> Optional[str]:
    if not path.exists():
        return None
    try:
        txt = path.read_text(encoding="utf-8", errors="ignore")
        if len(txt) > 8000:
            return txt[:8000] + "\n...[truncated]..."
        return txt
    except Exception:
        return None


def _summarize_depmap(data: dict) -> Dict[str, Any]:
    plugin_count = len(data)
    dependency_edges = sum(len(info.get("Dependencies") or []) for info in data.values())
    module_count = sum(len(info.get("Modules") or []) for info in data.values())
    return {
        "plugins": plugin_count,
        "dependencies": dependency_edges,
        "modules": module_count,
    }


def _summarize_todo(data: dict) -> Dict[str, Any]:
    return {
        "total_files": data.get("total_files", 0),
        "total_matches": data.get("total_matches", 0),
        "items": sum(len(v) for files in data.get("backlog", {}).values() for v in files.values()),
    }


def _summarize_tags(data: dict) -> Dict[str, Any]:
    return {
        "total_files": data.get("total_files", 0),
        "total_matches": data.get("total_matches", 0),
        "tag_root": data.get("tag_root"),
        "items": sum(len(v) for files in data.get("report", {}).values() for v in files.values()),
    }


def _summarize_api_surface(data: dict) -> Dict[str, Any]:
    return {
        "plugin": data.get("plugin"),
        "total_files": data.get("total_files", 0),
        "files_with_api": len(data.get("files", {})),
    }


def _summarize(key: str, data: dict) -> Dict[str, Any]:
    if key == "depmap":
        return _summarize_depmap(data)
    if key == "todo":
        return _summarize_todo(data)
    if key == "tags":
        return _summarize_tags(data)
    if key == "api_surface":
        return _summarize_api_surface(data)
    return {}


def _build_plugin_rows(reports: Dict[str, ReportStatus]) -> List[PluginSnapshot]:
    depmap = reports.get("depmap")
    todo = reports.get("todo")
    tags = reports.get("tags")

    depmap_data = depmap.data if depmap and depmap.present else {}
    todo_data = todo.data.get("backlog", {}) if todo and todo.present else {}
    tag_data = tags.data.get("report", {}) if tags and tags.present else {}

    plugin_names = set(depmap_data.keys()) | set(todo_data.keys()) | set(tag_data.keys())
    rows: List[PluginSnapshot] = []

    for plugin in sorted(plugin_names):
        deps = depmap_data.get(plugin, {}).get("Dependencies") or []
        modules = depmap_data.get(plugin, {}).get("Modules") or []
        todo_count = sum(len(v) for v in todo_data.get(plugin, {}).values())
        tag_count = sum(len(v) for v in tag_data.get(plugin, {}).values())
        rows.append(
            PluginSnapshot(
                name=plugin,
                dependencies=len(deps),
                modules=len(modules),
                todo_items=todo_count,
                tag_matches=tag_count,
            )
        )

    return rows


def _build_search_items(reports: Dict[str, ReportStatus], limit: int = 500) -> List[SearchItem]:
    items: List[SearchItem] = []
    idx = 0

    todo = reports.get("todo")
    if todo and todo.present:
        for plugin, files in todo.data.get("backlog", {}).items():
            for file_path, entries in files.items():
                for entry in entries:
                    idx += 1
                    items.append(
                        SearchItem(
                            id=f"todo-{idx}",
                            source="TODO",
                            plugin=plugin,
                            file=file_path,
                            text=str(entry.get("text", "")).strip(),
                        )
                    )
                    if len(items) >= limit:
                        return items

    tags = reports.get("tags")
    if tags and tags.present:
        for plugin, files in tags.data.get("report", {}).items():
            for file_path, entries in files.items():
                for entry in entries:
                    idx += 1
                    items.append(
                        SearchItem(
                            id=f"tag-{idx}",
                            source="TAG",
                            plugin=plugin,
                            file=file_path,
                            text=str(entry.get("text", "")).strip(),
                        )
                    )
                    if len(items) >= limit:
                        return items

    return items


def build_depmap_inbound_option(reports: Dict[str, ReportStatus]) -> Optional[dict]:
    depmap = reports.get("depmap")
    if not depmap or not depmap.present or not isinstance(depmap.data, dict):
        return None

    inbound = Counter()
    for info in depmap.data.values():
        for dep in info.get("Dependencies") or []:
            inbound[dep] += 1

    if not inbound:
        return None

    top = inbound.most_common(10)
    labels = [plugin for plugin, _ in reversed(top)]
    values = [count for _, count in reversed(top)]

    return {
        "tooltip": {"trigger": "axis"},
        "grid": {"left": "30%", "top": "10%", "bottom": "10%"},
        "xAxis": {"type": "value", "minInterval": 1},
        "yAxis": {"type": "category", "data": labels},
        "series": [
            {
                "name": "Inbound edges",
                "type": "bar",
                "data": values,
                "label": {"show": True, "position": "right"},
            }
        ],
    }


def load_reports(logger: logging.Logger) -> ReportBundle:
    reports: Dict[str, ReportStatus] = {}
    errors: List[str] = []

    for key, spec in REPORT_SPECS.items():
        json_path = resolve_report_path(spec["filename"])
        log_path = json_path.with_suffix(".log")
        status = ReportStatus(
            key=key,
            title=spec["title"],
            json_path=json_path,
            log_path=log_path,
        )

        data, err = _read_json(json_path)
        status.data = data
        status.log_text = _read_log(log_path)

        if err:
            status.error = err
            status.present = False
            errors.append(f"{spec['title']}: {err}")
            logger.info("[Hub] %s missing (%s)", spec["title"], err)
        else:
            status.present = True
            status.summary = _summarize(key, data or {})
            logger.info("[Hub] %s loaded (%s)", spec["title"], json_path)

        reports[key] = status

    bundle = ReportBundle(
        reports=reports,
        plugin_rows=_build_plugin_rows(reports),
        search_items=_build_search_items(reports),
        errors=errors,
        loaded_at=datetime.now(),
    )
    logger.info("[Hub] Reports snapshot prepared with %d plugin rows and %d search items", len(bundle.plugin_rows), len(bundle.search_items))
    return bundle
