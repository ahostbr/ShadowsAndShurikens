from __future__ import annotations

import sys
from pathlib import Path
from typing import List, Tuple


def report_commands(project_root: Path) -> List[Tuple[str, List[str]]]:
    tools_root = project_root / "DevTools" / "python"
    cmds = [
        ("DepMap", [sys.executable, str(tools_root / "sots_plugin_depmap.py"), "--root", str(project_root)]),
        ("TODO Backlog", [sys.executable, str(tools_root / "sots_todo_backlog.py"), "--root", str(project_root)]),
        ("Tag Usage", [sys.executable, str(tools_root / "sots_tag_usage_report.py"), "--root", str(project_root)]),
        ("API Surface", [sys.executable, str(tools_root / "sots_api_surface.py"), "--root", str(project_root)]),
    ]
    return cmds


def generate_all_commands(project_root: Path) -> List[List[str]]:
    return [cmd for _, cmd in report_commands(project_root)]
