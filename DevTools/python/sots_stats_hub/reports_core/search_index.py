from __future__ import annotations

from collections import defaultdict
from typing import List, Tuple

from .models import ApiRow, SearchRow, TagRow, TodoRow


def build_search_index(todos: List[TodoRow], tags: List[TagRow], apis: List[ApiRow]) -> List[SearchRow]:
    out: List[SearchRow] = []
    for t in todos:
        out.append(SearchRow(type="TODO", plugin=t.plugin, file=t.file, preview=t.text))
    for t in tags:
        out.append(SearchRow(type="TAG", plugin=t.plugin, file=t.file, preview=t.text))
    for a in apis:
        label = f"{a.kind}: {a.symbol}"
        out.append(SearchRow(type="API", plugin=a.plugin, file=a.file, preview=label))
    out.sort(key=lambda r: (r.type, r.plugin.lower(), r.file.lower()))
    return out


def build_hotspots(todos: List[TodoRow], tags: List[TagRow]) -> Tuple[List[Tuple[str, int]], List[Tuple[str, int]]]:
    plugin_scores = defaultdict(int)
    file_scores = defaultdict(int)
    for t in todos:
        plugin_scores[t.plugin] += 2
        file_scores[t.file] += 2
    for t in tags:
        plugin_scores[t.plugin] += 1
        file_scores[t.file] += 1
    hot_plugins = sorted(plugin_scores.items(), key=lambda x: (-x[1], x[0].lower()))[:50]
    hot_files = sorted(file_scores.items(), key=lambda x: (-x[1], x[0].lower()))[:50]
    return hot_plugins, hot_files
