from __future__ import annotations

from typing import Callable, List, Tuple

from PySide6 import QtCore, QtWidgets


class CommandPalette(QtWidgets.QDialog):
    def __init__(self, commands: List[Tuple[str, Callable[[], None]]], recent: List[str] | None = None, parent=None) -> None:
        super().__init__(parent)
        self.setWindowTitle("Command Palette")
        self.setModal(True)
        self.resize(500, 400)
        self.commands = commands
        self.recent = recent or []
        self.filtered: List[Tuple[str, Callable[[], None]]] = list(commands)
        layout = QtWidgets.QVBoxLayout(self)
        self.input = QtWidgets.QLineEdit()
        self.input.setPlaceholderText("Type a command...")
        layout.addWidget(self.input)
        self.list = QtWidgets.QListWidget()
        layout.addWidget(self.list)
        btn_row = QtWidgets.QHBoxLayout()
        self.close_btn = QtWidgets.QPushButton("Close")
        btn_row.addStretch(1)
        btn_row.addWidget(self.close_btn)
        layout.addLayout(btn_row)
        self.input.textChanged.connect(self._filter)
        self.list.itemActivated.connect(self._run_current)
        self.close_btn.clicked.connect(self.close)
        self._filter()

    def _filter(self) -> None:
        term = self.input.text().lower().strip()
        if term:
            self.filtered = [c for c in self.commands if term in c[0].lower()]
        else:
            recent_cmds = [c for c in self.commands if c[0] in self.recent]
            self.filtered = recent_cmds + [c for c in self.commands if c[0] not in self.recent]
        self.list.clear()
        for label, _ in self.filtered:
            self.list.addItem(label)
        if self.list.count() > 0:
            self.list.setCurrentRow(0)

    def _run_current(self) -> None:
        row = self.list.currentRow()
        if row < 0 or row >= len(self.filtered):
            return
        _, func = self.filtered[row]
        try:
            func()
        except Exception:
            pass
        self.accept()
