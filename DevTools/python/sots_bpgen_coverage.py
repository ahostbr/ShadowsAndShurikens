"""
Coverage reporting for BPGen snippet packs.

- Based on a central snippet_index.json (manual/semi-manual for now).
- Pure filesystem/JSON; no Unreal dependencies.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, List


def get_default_index_path() -> Path:
    return Path(__file__).resolve().parent.parent / "bpgen_snippets" / "index" / "snippet_index.json"


def load_snippet_index(index_path: str | Path | None = None) -> Dict[str, Any]:
    """
    Load the snippet index JSON. Returns a dict with at least 'version' and 'snippets'.
    """
    path = Path(index_path) if index_path else get_default_index_path()
    path = path.resolve()
    print(f"[INFO] Loading snippet index: {path}")

    if not path.exists():
        print(f"[ERROR] snippet_index.json not found at {path}")
        return {"version": "0.0.0", "snippets": []}

    try:
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception as exc:
        print(f"[ERROR] Failed to read snippet index '{path}': {exc}")
        return {"version": "0.0.0", "snippets": []}

    if not isinstance(data, dict) or "snippets" not in data:
        print(f"[ERROR] Invalid snippet index format at {path}")
        return {"version": "0.0.0", "snippets": []}

    print(f"[INFO] Loaded snippet index with {len(data.get('snippets', []))} snippet(s).")
    return data


def build_node_coverage(index_data: Dict[str, Any]) -> Dict[str, Dict[str, Any]]:
    """
    Build mapping: node_class -> { count, snippets }
    """
    coverage: Dict[str, Dict[str, Any]] = {}
    snippets = index_data.get("snippets", [])
    if not isinstance(snippets, list):
        return coverage

    for entry in snippets:
        if not isinstance(entry, dict):
            continue
        snippet_name = entry.get("snippet_name")
        node_classes = entry.get("node_classes", [])
        if not snippet_name or not isinstance(node_classes, list):
            continue
        for node_cls in node_classes:
            cov = coverage.setdefault(node_cls, {"count": 0, "snippets": []})
            cov["count"] += 1
            cov["snippets"].append(snippet_name)

    return coverage


def print_coverage_report(index_data: Dict[str, Any]) -> None:
    """
    Print human-readable coverage report to stdout.
    """
    coverage = build_node_coverage(index_data)
    print("Node coverage report:")
    if not coverage:
        print("- No node coverage entries found (index may be empty).")
        return

    for node_cls in sorted(coverage.keys()):
        info = coverage[node_cls]
        snippets = ", ".join(info.get("snippets", []))
        print(f"- {node_cls}: {info.get('count', 0)} snippet(s) ({snippets})")

    print("")
    print("Note: This report reflects only snippets listed in snippet_index.json. Missing nodes simply mean no snippets have been registered yet.")
    print("Future enhancements: auto-update index from packs, and integrate UE node exports to track missing coverage.")


if __name__ == "__main__":
    data = load_snippet_index()
    print_coverage_report(data)
