from __future__ import annotations

import threading
import subprocess
from pathlib import Path
from typing import Callable, List, Optional

from PySide6 import QtCore, QtWidgets

import watch_new_docs as wnd


class DocsWatchPanel(QtWidgets.QWidget):
    pollChanged = QtCore.Signal(int)
    def __init__(
        self,
        project_root: Path,
        poll_seconds: float,
        log: Callable[[str], None],
        enqueue_job: Callable[[str, Callable[[Callable[[str], None],], int]], None] | None = None,  # type: ignore
        guard_writes: Optional[Callable[[str], bool]] = None,
        parent: Optional[QtWidgets.QWidget] = None,
    ) -> None:
        super().__init__(parent)
        self.project_root = project_root
        self.log = log
        self.poll_seconds = poll_seconds
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.scan_once)
        self.enqueue_job = enqueue_job
        self.guard_writes = guard_writes
        self._build_ui()
        self.timer.start(int(self.poll_seconds * 1000))

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        controls = QtWidgets.QHBoxLayout()
        controls.addWidget(QtWidgets.QLabel("Poll (s):"))
        self.poll_combo = QtWidgets.QComboBox()
        for v in [2, 5, 10, 30]:
            self.poll_combo.addItem(str(v), userData=v)
            if int(self.poll_seconds) == v:
                self.poll_combo.setCurrentText(str(v))
        controls.addWidget(self.poll_combo)
        self.watch_btn = QtWidgets.QPushButton("Stop Watching")
        self.scan_btn = QtWidgets.QPushButton("Scan Now")
        self.combo_zip_btn = QtWidgets.QPushButton("Zip New/Modified (combo)")
        self.zip_btn = QtWidgets.QPushButton("Zip New/Modified Docs")
        self.ack_btn = QtWidgets.QPushButton("Mark Handled (ACK)")
        self.open_reports_btn = QtWidgets.QPushButton("Open Reports Folder")
        controls.addWidget(self.watch_btn)
        controls.addWidget(self.scan_btn)
        controls.addWidget(self.combo_zip_btn)
        controls.addWidget(self.zip_btn)
        controls.addWidget(self.ack_btn)
        controls.addWidget(self.open_reports_btn)
        controls.addStretch(1)
        layout.addLayout(controls)

        lists_layout = QtWidgets.QHBoxLayout()
        self.new_list = QtWidgets.QListWidget()
        self.mod_list = QtWidgets.QListWidget()
        self.new_list.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        self.mod_list.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        lists_layout.addWidget(self._wrap_group("New docs", self.new_list))
        lists_layout.addWidget(self._wrap_group("Modified docs", self.mod_list))
        layout.addLayout(lists_layout)
        layout.addStretch(1)

        self.poll_combo.currentIndexChanged.connect(self._on_poll_change)
        self.watch_btn.clicked.connect(self._on_toggle_watch)
        self.scan_btn.clicked.connect(self.scan_once)
        self.combo_zip_btn.clicked.connect(self._on_combo_zip)
        self.zip_btn.clicked.connect(self._on_zip)
        self.ack_btn.clicked.connect(self._on_ack)
        self.open_reports_btn.clicked.connect(self._on_open_reports)
        # initial populate
        self.scan_once()

    def _wrap_group(self, title: str, widget: QtWidgets.QWidget) -> QtWidgets.QGroupBox:
        box = QtWidgets.QGroupBox(title)
        v = QtWidgets.QVBoxLayout(box)
        v.addWidget(widget)
        return box

    def _state(self) -> dict:
        exts = wnd.DEFAULT_EXTS
        return wnd._init_state_if_missing(self.project_root, exts, verbose=False)

    def _diff(self) -> tuple[list[wnd.DocFile], list[wnd.DocFile]]:
        st = self._state()
        exts = wnd.DEFAULT_EXTS
        new_files, modified_files, _ = wnd._diff_since_baseline(self.project_root, exts, st)
        return list(new_files), list(modified_files)

    def scan_once(self) -> None:
        def work() -> None:
            try:
                new_files, mod_files = self._diff()
                QtCore.QMetaObject.invokeMethod(
                    self,
                    "_update_lists",
                    QtCore.Qt.QueuedConnection,
                    QtCore.Q_ARG("QVariantList", [df.rel for df in new_files]),
                    QtCore.Q_ARG("QVariantList", [df.rel for df in mod_files]),
                )
            except Exception as exc:  # noqa: BLE001
                QtCore.QMetaObject.invokeMethod(
                    self,
                    "_log_error",
                    QtCore.Qt.QueuedConnection,
                    QtCore.Q_ARG(str, f"[DOCS] scan failed: {exc}"),
                )

        threading.Thread(target=work, daemon=True).start()

    @QtCore.Slot(list, list)
    def _update_lists(self, new_items: List[str], mod_items: List[str]) -> None:
        self.new_list.clear()
        self.mod_list.clear()
        self.new_list.addItems(new_items or ["(none)"])
        self.mod_list.addItems(mod_items or ["(none)"])
        self.log(f"[DOCS] new={len(new_items)} modified={len(mod_items)}")

    @QtCore.Slot(str)
    def _log_error(self, msg: str) -> None:
        self.log(msg)

    def _on_zip(self) -> None:
        if self.guard_writes and not self.guard_writes("Docs Zip"):
            return
        if self.enqueue_job:
            self.enqueue_job("Docs Zip", self._job_zip)
        else:
            threading.Thread(target=lambda: self._job_zip(self.log), daemon=True).start()

    def _on_combo_zip(self) -> None:
        if self.guard_writes and not self.guard_writes("Docs Combo Zip"):
            return
        if self.enqueue_job:
            self.enqueue_job("Docs Combo Zip", self._job_combo_zip)
        else:
            threading.Thread(target=lambda: self._job_combo_zip(self.log), daemon=True).start()

    def _job_zip(self, log_fn: Callable[[str], None]) -> int:
        st = self._state()
        new_files, mod_files, _ = wnd._diff_since_baseline(self.project_root, wnd.DEFAULT_EXTS, st)
        report, listfile, rels = wnd._write_report_and_listfiles(
            self.project_root, st, new_files, mod_files, include_modified=True
        )
        stamp = wnd._local_now_stamp()
        out_zip = wnd._report_dir(self.project_root) / f"SOTS_new_docs_{stamp}.zip"
        written, missing = wnd._zip_files(self.project_root, [df.rel for df in new_files + mod_files], out_zip)
        wnd._ack_now(self.project_root, wnd.DEFAULT_EXTS)
        log_fn(f"[DOCS] zipped={written} missing={len(missing)} -> {out_zip}")
        log_fn(f"[DOCS] report={report} list={listfile}")
        QtCore.QMetaObject.invokeMethod(self, "scan_once", QtCore.Qt.QueuedConnection)
        return 0

    def _job_combo_zip(self, log_fn: Callable[[str], None]) -> int:
        st = self._state()
        new_files, mod_files, _ = wnd._diff_since_baseline(self.project_root, wnd.DEFAULT_EXTS, st)
        if not new_files and not mod_files:
            log_fn("[DOCS] no changes; skipping zip")
            return 0
        res = self._job_zip(log_fn)
        wnd._ack_now(self.project_root, wnd.DEFAULT_EXTS)
        log_fn("[DOCS] baseline acknowledged after combo zip")
        return res

    def _on_ack(self) -> None:
        if self.guard_writes and not self.guard_writes("Docs ACK"):
            return
        wnd._ack_now(self.project_root, wnd.DEFAULT_EXTS)
        self.log("[DOCS] baseline reset (handled).")
        self.scan_once()

    def _on_open_reports(self) -> None:
        reports_dir = wnd._report_dir(self.project_root)
        reports_dir.mkdir(parents=True, exist_ok=True)
        try:
            subprocess.Popen(["explorer", str(reports_dir)])
        except Exception:
            self.log("[WARN] Could not open reports folder.")

    def _on_poll_change(self) -> None:
        val = int(self.poll_combo.currentData())
        self.poll_seconds = val
        if self.timer.isActive():
            self.timer.setInterval(val * 1000)
        self.log(f"[DOCS] poll interval set to {val}s")
        self.pollChanged.emit(val)

    def _on_toggle_watch(self) -> None:
        if self.timer.isActive():
            self.timer.stop()
            self.watch_btn.setText("Start Watching")
            self.log("[DOCS] watching paused")
        else:
            self.timer.start(int(self.poll_seconds * 1000))
            self.watch_btn.setText("Stop Watching")
            self.log(f"[DOCS] watching resumed every {int(self.poll_seconds)}s")
