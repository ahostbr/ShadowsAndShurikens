from __future__ import annotations

import threading
from pathlib import Path
from typing import Optional

from PySide6 import QtCore

from .model import Node, ScanStats
from .scanner import ScanCancelled, ScanOptions, scan_path


class ScanWorker(QtCore.QThread):
    progress = QtCore.Signal(str)
    finished = QtCore.Signal(object, object)  # Node, ScanStats
    cancelled = QtCore.Signal()
    failed = QtCore.Signal(str)

    def __init__(self, root: str, options: ScanOptions, parent: Optional[QtCore.QObject] = None) -> None:
        super().__init__(parent)
        self.root = root
        self.options = options
        self._cancel_event = threading.Event()

    def cancel(self) -> None:
        self._cancel_event.set()

    def run(self) -> None:  # noqa: D401
        try:
            node, stats = scan_path(
                self.root,
                options=self.options,
                progress_cb=self.progress.emit,
                cancel_event=self._cancel_event,
            )
        except ScanCancelled:
            self.cancelled.emit()
            return
        except Exception as exc:  # noqa: BLE001
            self.failed.emit(str(exc))
            return

        node.rebuild_parents(None)
        self.finished.emit(node, stats)
