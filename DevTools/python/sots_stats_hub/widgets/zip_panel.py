from __future__ import annotations

import sys
from pathlib import Path
from typing import Callable, Optional

from PySide6 import QtWidgets

from ..runner import ProcessManager


class ZipPanel(QtWidgets.QWidget):
    def __init__(
        self,
        pm: ProcessManager,
        tools_root: Path,
        log: Callable[[str], None],
        enqueue_job=None,
        run_tool=None,
        tool_lookup=None,
        parent: Optional[QtWidgets.QWidget] = None,
    ) -> None:
        super().__init__(parent)
        self.pm = pm
        self.tools_root = tools_root
        self.log = log
        self.enqueue_job = enqueue_job
        self.run_tool = run_tool
        self.tool_lookup = tool_lookup or {}
        self._build_ui()

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        row = QtWidgets.QHBoxLayout()
        self.zip_plugins_btn = QtWidgets.QPushButton("Zip Plugin Sources")
        self.zip_docs_btn = QtWidgets.QPushButton("Zip All Docs")
        self.open_last_btn = QtWidgets.QPushButton("Open Folder")
        self.copy_last_btn = QtWidgets.QPushButton("Copy Path")
        row.addWidget(self.zip_plugins_btn)
        row.addWidget(self.zip_docs_btn)
        row.addWidget(self.open_last_btn)
        row.addWidget(self.copy_last_btn)
        row.addStretch(1)
        layout.addLayout(row)
        self.last_path_lbl = QtWidgets.QLabel("(no zips yet)")
        layout.addWidget(self.last_path_lbl)
        layout.addStretch(1)

        self.zip_plugins_btn.clicked.connect(self._on_zip_plugins)
        self.zip_docs_btn.clicked.connect(self._on_zip_docs)
        self.open_last_btn.clicked.connect(self._on_open_last)
        self.copy_last_btn.clicked.connect(self._on_copy_last)

        self._last_path: Optional[Path] = None

    def _run_script(self, script_name: str) -> None:
        tool_id_map = {
            "zip_sots_suite_plugin_sources.py": "zip_plugins",
            "zip_all_docs.py": "zip_docs",
        }
        spec = self.tool_lookup.get(tool_id_map.get(script_name, ""))
        if self.run_tool and spec:
            self.run_tool(spec)
            return
        script = self.tools_root / script_name
        if not script.exists():
            self.log(f"[ERROR] Script missing: {script}")
            return
        args = [sys.executable, str(script)]
        self._last_path = None
        if self.enqueue_job:
            self.enqueue_job(f"Run {script_name}", lambda log_fn: self._job_run(args, log_fn))
        else:
            self.pm.run_threaded(args, cwd=self.tools_root)

    def _on_zip_plugins(self) -> None:
        self._run_script("zip_sots_suite_plugin_sources.py")

    def _on_zip_docs(self) -> None:
        self._run_script("zip_all_docs.py")

    def set_last_path(self, path: Path) -> None:
        self._last_path = path
        self.last_path_lbl.setText(str(path))

    def _on_open_last(self) -> None:
        if not self._last_path:
            return
        try:
            import subprocess

            subprocess.Popen(["explorer", str(self._last_path.parent)])
        except Exception:
            self.log("[WARN] Could not open folder.")

    def _on_copy_last(self) -> None:
        if not self._last_path:
            return
        QtWidgets.QApplication.clipboard().setText(str(self._last_path))
        self.log(f"[INFO] Copied path: {self._last_path}")

    def _job_run(self, args: list[str], log_fn: Callable[[str], None]) -> int:
        import subprocess

        log_fn(f"[INFO] running: {' '.join(args)}")
        proc = subprocess.Popen(args, cwd=str(self.tools_root), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        for line in proc.stdout or []:
            log_fn(line.rstrip())
        proc.wait()
        log_fn(f"[INFO] exit code {proc.returncode}")
        return int(proc.returncode)
