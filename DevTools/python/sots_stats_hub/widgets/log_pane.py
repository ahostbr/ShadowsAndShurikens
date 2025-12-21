from __future__ import annotations

import datetime
from pathlib import Path
from typing import Optional

from PySide6 import QtWidgets


class LogPane(QtWidgets.QWidget):
    def __init__(self, log_dir: Path, parent: Optional[QtWidgets.QWidget] = None) -> None:
        super().__init__(parent)
        self.log_dir = log_dir
        self.log_dir.mkdir(parents=True, exist_ok=True)
        self.log_path = self.log_dir / f"log_{datetime.datetime.now().strftime('%Y%m%d')}.txt"

        layout = QtWidgets.QVBoxLayout(self)
        filter_row = QtWidgets.QHBoxLayout()
        filter_row.addWidget(QtWidgets.QLabel("Log filter:"))
        self.filter_combo = QtWidgets.QComboBox()
        self.filter_combo.addItems(["All", "Errors", "Jobs"])
        filter_row.addWidget(self.filter_combo)
        filter_row.addStretch(1)
        layout.addLayout(filter_row)
        self.view = QtWidgets.QPlainTextEdit()
        self.view.setReadOnly(True)
        self.view.setMaximumBlockCount(5000)
        layout.addWidget(self.view)
        self.buffer: list[str] = []
        self.filter_combo.currentIndexChanged.connect(self._apply_filter)

    def append(self, msg: str) -> None:
        timestamp = datetime.datetime.now().strftime("%H:%M:%S")
        line = f"[{timestamp}] {msg}"
        self.buffer.append(line)
        self._apply_filter()
        try:
            self.log_path.parent.mkdir(parents=True, exist_ok=True)
            with self.log_path.open("a", encoding="utf-8") as fh:
                fh.write(line + "\n")
        except Exception:
            pass

    def _apply_filter(self) -> None:
        sel = self.filter_combo.currentText()
        self.view.blockSignals(True)
        self.view.clear()
        for line in self.buffer[-5000:]:
            if sel == "Errors" and "[ERR" not in line and "[WARN" not in line:
                continue
            if sel == "Jobs" and "[JOB" not in line:
                continue
            self.view.appendPlainText(line)
        self.view.verticalScrollBar().setValue(self.view.verticalScrollBar().maximum())
        self.view.blockSignals(False)
