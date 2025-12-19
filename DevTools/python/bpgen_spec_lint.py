"""Lint a BPGen GraphSpec JSON file (SPINE_M)."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
from typing import Any, Dict, Iterable, List


def _load_json(path: pathlib.Path) -> Dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    except Exception as exc:  # pragma: no cover - defensive
        raise RuntimeError(f"Failed to read JSON from {path}: {exc}") from exc


def _get_field(obj: Dict[str, Any], *names: str) -> Any:
    for name in names:
        if name in obj:
            return obj[name]
    lower_map = {k.lower(): k for k in obj}
    for name in names:
        key = lower_map.get(name.lower())
        if key:
            return obj[key]
    return None


def _iter_list(value: Any) -> Iterable[Dict[str, Any]]:
    if isinstance(value, list):
        for entry in value:
            if isinstance(entry, dict):
                yield entry


def lint_spec(spec: Dict[str, Any]) -> tuple[list[str], list[str]]:
    errors: List[str] = []
    warnings: List[str] = []

    spec_version = _get_field(spec, "spec_version", "SpecVersion")
    spec_schema = _get_field(spec, "spec_schema", "SpecSchema")
    if spec_version is None:
        warnings.append("Missing spec_version at root.")
    if not spec_schema:
        warnings.append("Missing spec_schema at root.")

    nodes = _get_field(spec, "Nodes", "nodes") or []
    links = _get_field(spec, "Links", "links") or []

    for idx, node in enumerate(_iter_list(nodes), start=1):
        node_id = _get_field(node, "Id", "id")
        if not node_id:
            errors.append(f"Node {idx}: missing Id.")

        node_type = _get_field(node, "NodeType", "node_type")
        spawner_key = _get_field(node, "SpawnerKey", "spawner_key")
        if not node_type and not spawner_key:
            errors.append(f"Node {idx} ({node_id or 'unknown'}): missing NodeType/SpawnerKey.")

        stable_id = _get_field(node, "NodeId", "node_id")
        if not stable_id:
            warnings.append(f"Node {idx} ({node_id or 'unknown'}): missing NodeId (stable id).")

        prefer_spawner = _get_field(node, "PreferSpawnerKey", "prefer_spawner_key")
        if spawner_key and prefer_spawner is False:
            warnings.append(f"Node {idx} ({node_id or 'unknown'}): PreferSpawnerKey=false while SpawnerKey is set.")

    for idx, link in enumerate(_iter_list(links), start=1):
        from_node = _get_field(link, "FromNodeId", "from_node_id")
        to_node = _get_field(link, "ToNodeId", "to_node_id")
        from_pin = _get_field(link, "FromPinName", "from_pin", "from_pin_name")
        to_pin = _get_field(link, "ToPinName", "to_pin", "to_pin_name")
        if not from_node or not to_node or not from_pin or not to_pin:
            errors.append(f"Link {idx}: missing from/to node or pin fields.")

        allow_heuristic = _get_field(link, "AllowHeuristicPinMatch", "allow_heuristic_pin_match")
        if allow_heuristic:
            warnings.append(f"Link {idx}: allow_heuristic_pin_match enabled (risky).")

    return errors, warnings


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Lint a BPGen GraphSpec JSON file")
    parser.add_argument("--spec", required=True, type=pathlib.Path, help="Path to GraphSpec JSON file")
    return parser.parse_args(argv)


def main(argv: List[str]) -> int:
    args = parse_args(argv)
    spec = _load_json(args.spec)
    errors, warnings = lint_spec(spec)

    if errors:
        print("[ERROR] Issues found:")
        for err in errors:
            print(f"  - {err}")
    if warnings:
        print("[WARN] Warnings:")
        for warn in warnings:
            print(f"  - {warn}")

    if not errors and not warnings:
        print("No issues found.")
        return 0

    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
