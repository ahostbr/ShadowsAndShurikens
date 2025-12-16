from __future__ import annotations

import json
import logging
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional, Set

from ..config import REPORT_SPECS
from ..hub_config import REPORTS_DIR


@dataclass
class ReportResult:
    key: str
    title: str
    path: Path
    log_path: Path
    data: Optional[Any] = None
    error: Optional[str] = None


@dataclass
class ReportSummary:
    plugins_count: int = 0
    dependency_edges: int = 0
    todo_total_items: int = 0
    todo_total_files: int = 0
    tag_total_matches: int = 0
    tag_total_files: int = 0
    api_plugin: Optional[str] = None
    api_headers_scanned: int = 0
    errors: Dict[str, str] = field(default_factory=dict)


class ReportStore:
    def __init__(self) -> None:
        self.results: Dict[str, ReportResult] = {}
        self.errors: Dict[str, str] = {}
        self.file_mtimes: Dict[str, Optional[float]] = {}
        self.changed_keys: Set[str] = set()
        self.depmap_data: Dict[str, Any] = {}
        self.todo_data: Dict[str, Dict[str, Any]] = {}
        self.tag_data: Dict[str, Dict[str, Any]] = {}
        self.api_data: Dict[str, Any] = {}

    def _get_mtime(self, path: Path) -> Optional[float]:
        try:
            return path.stat().st_mtime
        except FileNotFoundError:
            return None

    def _load_json(self, path: Path) -> tuple[Optional[Any], Optional[str]]:
        if not path.exists():
            return None, "Missing report artifact"
        try:
            return json.loads(path.read_text(encoding="utf-8")), None
        except json.JSONDecodeError as exc:
            return None, f"Malformed JSON ({exc})"
        except Exception as exc:
            return None, f"Failed to read JSON ({exc})"

    def refresh(self, logger: logging.Logger, force: bool = False) -> Set[str]:
        prev_mtimes = dict(self.file_mtimes)
        changed_keys: Set[str] = set()
        self.results.clear()
        self.errors.clear()

        for key, spec in REPORT_SPECS.items():
            path = REPORTS_DIR / spec["filename"]
            log_path = path.with_suffix(".log")
            result = ReportResult(key=key, title=spec["title"], path=path, log_path=log_path)

            mtime = self._get_mtime(path)
            self.file_mtimes[key] = mtime
            if force or mtime != prev_mtimes.get(key):
                changed_keys.add(key)

            data, error = self._load_json(path)
            result.data = data
            if error:
                result.error = error
                self.errors[key] = error
                logger.warning("[Hub] %s: %s", spec["title"], error)
            else:
                logger.info("[Hub] %s loaded (%s)", spec["title"], path)

            self.results[key] = result

        self.depmap_data = self.results.get("depmap").data or {}
        todo_result = self.results.get("todo")
        self.todo_data = (
            todo_result.data.get("backlog", {}) if todo_result and isinstance(todo_result.data, dict) else {}
        )

        tags_result = self.results.get("tags")
        self.tag_data = (
            tags_result.data.get("report", {}) if tags_result and isinstance(tags_result.data, dict) else {}
        )

        api_result = self.results.get("api_surface")
        self.api_data = api_result.data if api_result and isinstance(api_result.data, dict) else {}

        self.changed_keys = changed_keys
        logger.info("[Hub] Report store refreshed (changes: %s)", ", ".join(sorted(changed_keys)) or "none")
        return changed_keys

    def summarize(self) -> ReportSummary:
        summary = ReportSummary()

        if isinstance(self.depmap_data, dict):
            summary.plugins_count = len(self.depmap_data)
            summary.dependency_edges = sum(
                len(info.get("Dependencies") or []) for info in self.depmap_data.values()
            )

        summary.todo_total_items = sum(
            len(entries or []) for plugin in self.todo_data.values() for entries in plugin.values()
        )
        summary.todo_total_files = sum(len(plugin) for plugin in self.todo_data.values())

        summary.tag_total_matches = sum(
            len(entries or []) for plugin in self.tag_data.values() for entries in plugin.values()
        )
        summary.tag_total_files = sum(len(plugin) for plugin in self.tag_data.values())

        if self.api_data:
            summary.api_plugin = self.api_data.get("plugin")
            summary.api_headers_scanned = self.api_data.get("total_files", 0)

        summary.errors = dict(self.errors)
        return summary

    def graph_payload(self) -> Dict[str, List[Dict[str, Any]]]:
        nodes: List[Dict[str, Any]] = []
        edges: List[Dict[str, Any]] = []
        inbound_counter: Counter[str] = Counter()

        for source, info in self.depmap_data.items():
            for target in info.get("Dependencies") or []:
                inbound_counter[target] += 1
                edges.append({"source": source, "target": target})

        plugin_names = set(self.depmap_data.keys()) | set(self.todo_data.keys()) | set(self.tag_data.keys())
        for plugin in sorted(plugin_names):
            outbound = len(self.depmap_data.get(plugin, {}).get("Dependencies") or [])
            todo_count = sum(len(entries or []) for entries in self.todo_data.get(plugin, {}).values())
            tag_count = sum(len(entries or []) for entries in self.tag_data.get(plugin, {}).values())
            nodes.append(
                {
                    "id": plugin,
                    "label": plugin,
                    "meta": {
                        "outbound": outbound,
                        "inbound": inbound_counter.get(plugin, 0),
                        "todo": todo_count,
                        "tag": tag_count,
                    },
                }
            )

        return {"nodes": nodes, "edges": edges}

    def plugin_names(self) -> List[str]:
        plugin_names = set(self.depmap_data.keys()) | set(self.todo_data.keys()) | set(self.tag_data.keys())
        return sorted(plugin_names)
