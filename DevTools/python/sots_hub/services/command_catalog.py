from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import List

from ..hub_config import PROJECT_ROOT


@dataclass(frozen=True)
class CommandTemplate:
    key: str
    title: str
    description: str
    command: str
    outputs: List[str]
    note: str | None = None


def _quote(path: Path) -> str:
    return f'"{path}"'


def _python_script(script_name: str) -> Path:
    return PROJECT_ROOT / "DevTools" / "python" / script_name


def _report_path(filename: str) -> Path:
    return PROJECT_ROOT / "DevTools" / "reports" / filename



def _build_command(script: str, output: str, root_override: str | None = None) -> str:
    script_path = _quote(_python_script(script))
    root = root_override or str(PROJECT_ROOT)
    output_path = _quote(_report_path(output))
    return f'python {script_path} --root "{root}" --output-json {output_path}'


COMMAND_TEMPLATES: List[CommandTemplate] = [
    CommandTemplate(
        key="depmap",
        title="Plugin Dependency Map",
        description="Scans Plugins/* for dependencies and emits a depmap (.json + log).",
        command=_build_command("sots_plugin_depmap.py", "sots_plugin_depmap.json"),
        outputs=[str(_report_path("sots_plugin_depmap.json"))],
    ),
    CommandTemplate(
        key="todo",
        title="TODO / FIXME Backlog",
        description="Collects TODO/FIXME comments across the tree.",
        command=_build_command("sots_todo_backlog.py", "sots_todo_backlog.json"),
        outputs=[str(_report_path("sots_todo_backlog.json"))],
    ),
    CommandTemplate(
        key="tags",
        title="Gameplay Tag Usage",
        description="Searches for gameplay tags (SAS.*) inside Plugins and Config.",
        command=_build_command("sots_tag_usage_report.py", "sots_tag_usage_report.json"),
        outputs=[str(_report_path("sots_tag_usage_report.json"))],
    ),
    CommandTemplate(
        key="api_surface",
        title="API Surface Snapshot",
        description="Extracts public UCLASS/USTRUCT/UFUNCTION/UPROPERTY data for a plugin.",
        command=_build_command(
            "sots_api_surface.py",
            "sots_api_surface.json",
            root_override=str(PROJECT_ROOT / "Plugins" / "SOTS_UI" / "Source" / "SOTS_UI"),
        ),
        outputs=[str(_report_path("sots_api_surface.json"))],
        note="Example root; edit to point at the plugin you care about.",
    ),
]
