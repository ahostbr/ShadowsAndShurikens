from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List

from .command_catalog import COMMAND_TEMPLATES


@dataclass
class CommandEntry:
    label: str
    kind: str  # "nav" or "copy"
    target: str | None = None
    payload: str | None = None
    priority: int = 1


def build_command_entries(report_store, pins) -> List[CommandEntry]:
    entries: List[CommandEntry] = []

    base_nav = [
        ("Go: Home", "/"),
        ("Go: Runs", "/runs"),
        ("Go: Plugins", "/plugins"),
        ("Go: Graphs", "/graphs"),
        ("Go: Search", "/search"),
        ("Go: Hotspots", "/hotspots"),
    ]

    for label, route in base_nav:
        entries.append(CommandEntry(label=label, kind="nav", target=route, priority=0))

    plugin_names = getattr(report_store, "plugin_names", lambda: [])()
    pinned = [p for p in plugin_names if p in pins]
    others = [p for p in plugin_names if p not in pins]

    for plugin in pinned + others:
        entries.append(
            CommandEntry(
                label=f"Plugin: {plugin}",
                kind="nav",
                target=f"/plugin/{plugin}",
                priority=0 if plugin in pins else 1,
            )
        )

    template_label = {
        "depmap": "Copy: Depmap command",
        "todo": "Copy: TODO backlog command",
        "tags": "Copy: Tag usage command",
        "api_surface": "Copy: API surface command (example)",
    }
    for template in COMMAND_TEMPLATES:
        entries.append(
            CommandEntry(
                label=template_label.get(template.key, f"Copy: {template.title}"),
                kind="copy",
                payload=template.command,
                priority=1,
            )
        )

    return entries


def to_rows(entries: List[CommandEntry]) -> List[Dict[str, str]]:
    return [
        {
            "label": entry.label,
            "kind": entry.kind,
            "target": entry.target or "",
            "payload": entry.payload or "",
            "priority": entry.priority,
        }
        for entry in entries
    ]
