from __future__ import annotations

from pathlib import Path
from typing import Dict


def detect_project_root(start: Path | None = None) -> Path:
    here = (start or Path(__file__)).resolve()
    for p in [here] + list(here.parents):
        if (p / "Plugins").exists():
            return p
    return here.parents[1]


def reports_root(project_root: Path) -> Path:
    return project_root / "DevTools" / "reports"


def report_paths(project_root: Path) -> Dict[str, Path]:
    rep = reports_root(project_root)
    return {
        "depmap": rep / "sots_plugin_depmap.json",
        "todo": rep / "sots_todo_backlog.json",
        "tags": rep / "sots_tag_usage_report.json",
        "api": rep / "sots_api_surface.json",
    }


def pins_path(project_root: Path) -> Path:
    return reports_root(project_root) / "sots_stats_hub_pins.json"


def logs_dir(project_root: Path) -> Path:
    return project_root / "DevTools" / "python" / "_reports" / "sots_stats_hub" / "Logs"
