from __future__ import annotations

from pathlib import Path


BASE_DIR = Path(__file__).resolve().parent
DEVTOOLS_DIR = BASE_DIR.parent
PROJECT_ROOT = DEVTOOLS_DIR.parent
REPORTS_DIR = PROJECT_ROOT / "DevTools" / "reports"
LOG_PATH = BASE_DIR / "sots_hub.log"
ASSETS_DIR = BASE_DIR / "assets"

# Known report file names + friendly labels
REPORT_SPECS = {
    "depmap": {
        "title": "Plugin Dependency Map",
        "filename": "sots_plugin_depmap.json",
    },
    "todo": {
        "title": "TODO / FIXME Backlog",
        "filename": "sots_todo_backlog.json",
    },
    "tags": {
        "title": "Gameplay Tag Usage",
        "filename": "sots_tag_usage_report.json",
    },
    "api_surface": {
        "title": "API Surface Snapshot",
        "filename": "sots_api_surface.json",
    },
}


def resolve_report_path(filename: str) -> Path:
    return REPORTS_DIR / filename
