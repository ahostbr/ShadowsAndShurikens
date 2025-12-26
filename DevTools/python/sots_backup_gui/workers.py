from __future__ import annotations

from typing import Any, Callable

from PySide6.QtCore import QThread, Signal

from restic_client import ActionLogger, make_log_path


class ResticWorker(QThread):
    log_line = Signal(str)
    result = Signal(object)
    error = Signal(str)

    def __init__(
        self,
        *,
        action_name: str,
        log_dir,
        action_fn: Callable[[ActionLogger], Any],
    ) -> None:
        super().__init__()
        self.action_name = action_name
        self.log_dir = log_dir
        self.action_fn = action_fn

    def run(self) -> None:
        logger = ActionLogger(make_log_path(self.log_dir, self.action_name), ui_callback=self.log_line.emit)
        try:
            logger.line(f"[action] {self.action_name}")
            result = self.action_fn(logger)
            self.result.emit(result)
        except Exception as exc:
            logger.line(f"[error] {type(exc).__name__}: {exc}")
            self.error.emit(str(exc))
        finally:
            logger.close()
