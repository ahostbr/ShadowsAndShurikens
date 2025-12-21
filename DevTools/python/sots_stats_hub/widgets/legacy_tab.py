from __future__ import annotations

from pathlib import Path
from typing import Optional

from PySide6 import QtWidgets


class LegacyTab(QtWidgets.QWidget):
    def __init__(self, project_root: Path, control_center_path: Optional[str] = None, parent=None) -> None:
        super().__init__(parent)
        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(QtWidgets.QLabel("Legacy tools (reference only). These will retire after SPINE_4 parity."))

        self._add_entry(layout, "sots_hub (NiceGUI)", project_root / "DevTools" / "python" / "sots_hub")
        if control_center_path:
            self._add_entry(layout, "DevToolsControlCenter / SOTS_CONTROL", Path(control_center_path))
        self._add_entry(layout, "sots_pipeline_hub (legacy CLI)", project_root / "DevTools" / "python" / "sots_pipeline_hub.py")
        layout.addStretch(1)

    def _add_entry(self, layout: QtWidgets.QVBoxLayout, label: str, target: Path) -> None:
        box = QtWidgets.QGroupBox(label)
        v = QtWidgets.QVBoxLayout(box)
        v.addWidget(QtWidgets.QLabel(str(target)))
        btn_row = QtWidgets.QHBoxLayout()
        open_btn = QtWidgets.QPushButton("Open Folder" if target.suffix == "" else "Open File")
        open_btn.clicked.connect(lambda: self._open_target(target))
        btn_row.addWidget(open_btn)
        btn_row.addStretch(1)
        v.addLayout(btn_row)
        layout.addWidget(box)

    def _open_target(self, target: Path) -> None:
        try:
            import subprocess

            if target.is_file():
                subprocess.Popen(["explorer", str(target)])
            else:
                subprocess.Popen(["explorer", str(target)])
        except Exception:
            pass
