from __future__ import annotations

import sys
from pathlib import Path
from typing import Callable, Optional

from PySide6 import QtWidgets

from ..runner import ProcessManager


class ActionsPanel(QtWidgets.QWidget):
    def __init__(
        self,
        pm: ProcessManager,
        tools_root: Path,
        video_control_path: str,
        log: Callable[[str], None],
        enqueue_job=None,
        tool_specs=None,
        run_tool=None,
        parent: Optional[QtWidgets.QWidget] = None,
    ) -> None:
        super().__init__(parent)
        self.pm = pm
        self.tools_root = tools_root
        self.video_control_path = video_control_path
        self.log = log
        self.enqueue_job = enqueue_job
        self.tool_specs = tool_specs or []
        self.run_tool = run_tool
        self._layout = QtWidgets.QVBoxLayout(self)
        self._build_ui()

    def _build_ui(self) -> None:
        layout = self._layout
        while layout.count():
            item = layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
        if self.tool_specs and self.run_tool:
            core_tools = [t for t in self.tool_specs if t.category in {"Core", "External"}]
            row = QtWidgets.QHBoxLayout()
            for spec in core_tools:
                btn = QtWidgets.QPushButton(spec.label)
                btn.clicked.connect(lambda _=False, s=spec: self.run_tool(s))
                row.addWidget(btn)
            row.addStretch(1)
            layout.addLayout(row)
        else:
            row1 = QtWidgets.QHBoxLayout()
            self.run_tools_btn = QtWidgets.QPushButton("Run sots_tools…")
            self.bridge_start_btn = QtWidgets.QPushButton("Start Bridge Server")
            self.bridge_stop_btn = QtWidgets.QPushButton("Stop Bridge Server")
            self.capture_btn = QtWidgets.QPushButton("Launch Capture (ffmpeg)")
            self.video_btn = QtWidgets.QPushButton("Open Video Control")
            row1.addWidget(self.run_tools_btn)
            row1.addWidget(self.bridge_start_btn)
            row1.addWidget(self.bridge_stop_btn)
            row1.addWidget(self.capture_btn)
            row1.addWidget(self.video_btn)
            row1.addStretch(1)
            layout.addLayout(row1)

            self.run_tools_btn.clicked.connect(self._on_run_tools)
            self.bridge_start_btn.clicked.connect(self._on_start_bridge)
            self.bridge_stop_btn.clicked.connect(self._on_stop_bridge)
            self.capture_btn.clicked.connect(self._on_capture)
            self.video_btn.clicked.connect(self._on_video)

        layout.addStretch(1)

    def update_tools(self, specs) -> None:
        self.tool_specs = specs or []
        self._build_ui()

    def _on_run_tools(self) -> None:
        script = self.tools_root / "sots_tools.py"
        if not script.exists():
            self.log(f"[ERROR] sots_tools.py missing at {script}")
            return
        try:
            import subprocess

            subprocess.Popen(["explorer", str(script.parent)])
            self.log(f"[INFO] opened sots_tools folder in Explorer: {script.parent}")
        except Exception as exc:  # noqa: BLE001
            self.log(f"[ERROR] Failed to open sots_tools folder: {exc}")

    def _on_start_bridge(self) -> None:
        script = self.tools_root / "sots_bridge_server.py"
        self.pm.start_bridge(script)

    def _on_stop_bridge(self) -> None:
        self.pm.stop_bridge()

    def _on_capture(self) -> None:
        script = self.tools_root / "sots_capture_ffmpeg.py"
        if not script.exists():
            self.log(f"[ERROR] Capture script missing: {script}")
            return
        try:
            import subprocess

            # Match legacy tab behavior: just open the folder in Explorer
            subprocess.Popen(["explorer", str(script.parent)])
            self.log(f"[INFO] opened capture folder in Explorer (not running): {script.parent}")
        except Exception as exc:  # noqa: BLE001
            self.log(f"[ERROR] Failed to open capture script: {exc}")

    def _on_video(self) -> None:
        if self.video_control_path and Path(self.video_control_path).exists():
            args = [self.video_control_path]
            try:
                import subprocess

                subprocess.Popen(args, cwd=str(Path(self.video_control_path).parent))
            except Exception as exc:  # noqa: BLE001
                self.log(f"[ERROR] Failed to launch Video Control: {exc}")
        else:
            self.log("[WARN] Video control path not set or missing.")

    def _job_run(self, args: list[str], log_fn: Callable[[str], None]) -> int:
        import subprocess

        log_fn(f"[INFO] running: {' '.join(args)}")
        proc = subprocess.Popen(args, cwd=str(self.tools_root), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        for line in proc.stdout or []:
            log_fn(line.rstrip())
        proc.wait()
        log_fn(f"[INFO] exit code {proc.returncode}")
        return int(proc.returncode)
