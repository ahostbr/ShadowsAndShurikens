from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, List, Optional


@dataclass
class ToolSpec:
    id: str
    label: str
    category: str  # Core | Reports | Watch | Zips | External | Legacy
    kind: str  # python_script | external_exe | function
    entrypoint: str | Callable[[], int]
    default_args: List[str] = field(default_factory=list)
    cwd_policy: str = "project_root"  # project_root | devtools_root | script_dir
    outputs_hint: List[str] = field(default_factory=list)
    safety_level: str = "read_only"  # read_only | write_files | server | external
    requires_confirm: bool = False
    confirm_text: str = ""
    palette_keywords: List[str] = field(default_factory=list)


def build_tools(project_root: Path, video_control_path: str = "") -> List[ToolSpec]:
    devtools_root = project_root / "DevTools" / "python"
    reports_root = project_root / "DevTools" / "reports"
    tools: List[ToolSpec] = []
    def script(
        spec_id: str,
        label: str,
        script_name: str,
        category: str,
        safety: str = "read_only",
        args: Optional[List[str]] = None,
        outputs: Optional[List[str]] = None,
        requires_confirm: bool = False,
        confirm_text: str = "",
    ) -> None:
        tools.append(
            ToolSpec(
                id=spec_id,
                label=label,
                category=category,
                kind="python_script",
                entrypoint=str(devtools_root / script_name),
                default_args=args or [],
                cwd_policy="devtools_root",
                outputs_hint=outputs or [],
                safety_level=safety,
                requires_confirm=requires_confirm,
                confirm_text=confirm_text,
                palette_keywords=[label.lower(), spec_id],
            )
        )

    # Core/Actions
    script(
        "sots_tools",
        "Run sots_tools",
        "sots_tools.py",
        "Core",
        safety="read_only",
        requires_confirm=False,
    )
    tools.append(
        ToolSpec(
            id="bridge_server",
            label="Start Bridge Server",
            category="Core",
            kind="function",
            entrypoint="start_bridge",
            safety_level="server",
            requires_confirm=True,
            confirm_text="Start the bridge server? It runs until stopped.",
            palette_keywords=["bridge", "server", "start"],
        )
    )
    tools.append(
        ToolSpec(
            id="bridge_stop",
            label="Stop Bridge Server",
            category="Core",
            kind="function",
            entrypoint="stop_bridge",
            safety_level="server",
            palette_keywords=["bridge", "server", "stop"],
        )
    )
    script(
        "capture_ffmpeg",
        "Launch Capture (ffmpeg)",
        "sots_capture_ffmpeg.py",
        "Core",
        safety="write_files",
        requires_confirm=True,
        confirm_text="Launch capture? This may write video files.",
    )

    # Reports
    script("depmap", "Generate DepMap", "sots_plugin_depmap.py", "Reports", safety="read_only", outputs=[str(reports_root / "sots_plugin_depmap.json")])
    script("todo", "Generate TODO Backlog", "sots_todo_backlog.py", "Reports", safety="read_only", outputs=[str(reports_root / "sots_todo_backlog.json")])
    script("tags", "Generate Tag Usage", "sots_tag_usage_report.py", "Reports", safety="read_only", outputs=[str(reports_root / "sots_tag_usage_report.json")])
    script("api", "Generate API Surface", "sots_api_surface.py", "Reports", safety="read_only", outputs=[str(reports_root / "sots_api_surface.json")])

    # Watch
    script("watch_scan", "Docs Watch: Scan", "watch_new_docs.py", "Watch", safety="read_only", args=["scan"])
    script(
        "watch_ack",
        "Docs Watch: ACK",
        "watch_new_docs.py",
        "Watch",
        safety="write_files",
        args=["ack"],
        requires_confirm=True,
        confirm_text="Reset docs watch baseline?",
    )
    script(
        "watch_zip",
        "Docs Watch: Zip",
        "watch_new_docs.py",
        "Watch",
        safety="write_files",
        args=["interactive"],
        requires_confirm=True,
        confirm_text="Create a zip of new/modified docs?",
    )

    # Zips
    script(
        "zip_plugins",
        "Zip Plugin Sources",
        "zip_sots_suite_plugin_sources.py",
        "Zips",
        safety="write_files",
        requires_confirm=True,
        confirm_text="Zip plugin sources now?",
    )
    script(
        "zip_docs",
        "Zip All Docs",
        "zip_all_docs.py",
        "Zips",
        safety="write_files",
        requires_confirm=True,
        confirm_text="Zip all docs now?",
    )

    # External launcher
    if video_control_path:
        tools.append(
            ToolSpec(
                id="video_control",
                label="Open Video Control",
                category="External",
                kind="external_exe",
                entrypoint=video_control_path,
                cwd_policy="project_root",
                safety_level="external",
                palette_keywords=["video", "control"],
            )
        )
    return tools
