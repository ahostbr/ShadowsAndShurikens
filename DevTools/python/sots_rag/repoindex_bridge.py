from __future__ import annotations

import json
import re
from collections import defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


class RepoIndexBridge:
    SYMBOL_PATTERN = re.compile(r"[A-Za-z0-9_]+")

    def __init__(self, project_root: Path) -> None:
        self.project_root = project_root
        self.repo_index_dir = project_root / "Reports" / "RepoIndex"
        self.symbols_by_name: Dict[str, List[Dict[str, object]]] = {}
        self.symbols_by_file: Dict[str, List[Dict[str, object]]] = defaultdict(list)
        self.tags_by_file: Dict[str, List[Dict[str, object]]] = defaultdict(list)
        self.module_to_plugin: Dict[str, str] = {}
        self.loaded = False

    def has_repo_index(self) -> bool:
        return (self.repo_index_dir / "symbol_map.json").exists()

    def _load_json(self, filename: str) -> Optional[object]:
        path = self.repo_index_dir / filename
        if not path.exists():
            return None
        try:
            with path.open("r", encoding="utf-8") as fh:
                return json.load(fh)
        except Exception:
            return None

    def load(self) -> None:
        if not self.repo_index_dir.is_dir():
            return
        symbol_map = self._load_json("symbol_map.json") or []
        tag_usage = self._load_json("tag_usage.json") or {}
        module_graph = self._load_json("module_graph.json") or {}

        for entry in symbol_map:
            name = entry.get("name")
            if not name:
                continue
            self.symbols_by_name.setdefault(name, []).append(entry)
            header = entry.get("header")
            source = entry.get("source")
            if header:
                self.symbols_by_file[header].append(entry)
            if source:
                self.symbols_by_file[source].append(entry)

        tags = tag_usage.get("tags", {})
        for tag, occurrences in tags.items():
            for occ in occurrences:
                file_path = occ.get("file")
                if file_path:
                    self.tags_by_file[file_path].append({"tag": tag, **occ})

        modules = module_graph.get("modules", [])
        for module in modules:
            name = module.get("name")
            plugin = module.get("plugin")
            if name and plugin:
                self.module_to_plugin[name] = plugin

        self.loaded = True

    @staticmethod
    def tokenize_query(query: str) -> Sequence[str]:
        return [tok for tok in RepoIndexBridge.SYMBOL_PATTERN.findall(query)]

    def infer_plugin_module(self, rel_path: str) -> Tuple[str, str]:
        parts = Path(rel_path).parts
        plugin = ""
        module = ""
        if "Plugins" in parts:
            idx = parts.index("Plugins")
            if idx + 1 < len(parts):
                plugin = parts[idx + 1]
        if "Source" in parts:
            idx = parts.index("Source")
            if idx + 1 < len(parts):
                module = parts[idx + 1]
        else:
            for mod_name in self.module_to_plugin:
                if mod_name in rel_path.replace("\\", "/"):
                    module = mod_name
                    plugin = self.module_to_plugin.get(mod_name, plugin)
                    break
        return plugin, module

    def symbols_in_range(self, rel_path: str, start: int, end: int) -> List[str]:
        hits: List[str] = []
        for entry in self.symbols_by_file.get(rel_path, []):
            line = entry.get("line")
            if isinstance(line, int) and start <= line <= end:
                name = entry.get("name")
                if name:
                    hits.append(name)
        return sorted(set(hits))

    def tags_in_range(self, rel_path: str, start: int, end: int) -> List[str]:
        hits: List[str] = []
        for entry in self.tags_by_file.get(rel_path, []):
            line = entry.get("line")
            if isinstance(line, int) and start <= line <= end:
                tag = entry.get("tag")
                if tag:
                    hits.append(tag)
        return sorted(set(hits))

    def exact_symbol_matches(self, query: str) -> List[Dict[str, object]]:
        if not self.loaded or not self.symbols_by_name:
            return []
        query_lower = query.lower()
        matches: List[Dict[str, object]] = []
        for name, entries in self.symbols_by_name.items():
            if name.lower() in query_lower:
                for entry in entries:
                    record = {
                        "type": "symbol",
                        "term": name,
                        "path": entry.get("header") or entry.get("source") or "",
                        "line": entry.get("line"),
                        "module": entry.get("module"),
                        "plugin": entry.get("plugin"),
                    }
                    matches.append(record)
        return matches

    def exact_tag_matches(self, query: str) -> List[Dict[str, object]]:
        if not self.loaded or not self.tags_by_file:
            return []
        query_lower = query.lower()
        matches: List[Dict[str, object]] = []
        seen: set[str] = set()
        for file_path, occurrences in self.tags_by_file.items():
            for occ in occurrences:
                tag = occ.get("tag", "")
                if not tag or tag in seen:
                    continue
                if tag.lower() in query_lower:
                    matches.append(
                        {
                            "type": "tag",
                            "term": tag,
                            "path": file_path,
                            "line": occ.get("line"),
                            "plugin": occ.get("plugin"),
                            "module": occ.get("module"),
                        }
                    )
                    seen.add(tag)
        return matches

    def exact_matches(self, query: str) -> List[Dict[str, object]]:
        return self.exact_symbol_matches(query) + self.exact_tag_matches(query)
