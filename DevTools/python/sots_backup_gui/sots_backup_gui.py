from __future__ import annotations

import json
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

from PySide6 import QtGui
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QGuiApplication
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QFormLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QPlainTextEdit,
    QSizePolicy,
    QSplitter,
    QTableWidget,
    QTableWidgetItem,
    QTextEdit,
    QVBoxLayout,
    QWidget,
    QFileDialog,
)

from notes_store import NotesStore
from restic_client import ActionLogger, ResticClient, make_log_path, scan_for_restic_exe_on_c_drive
from workers import ResticWorker


DEFAULT_LOG_DIR = Path("C:/UE5/SOTS_Backup/logs")


def _load_config(config_path: Path) -> dict:
    if not config_path.is_file():
        raise FileNotFoundError(f"GUI config not found: {config_path}")
    return json.loads(config_path.read_text(encoding="utf-8"))


def _save_config(config_path: Path, config: dict) -> None:
    config_path.write_text(json.dumps(config, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def _shorten_text(value: str, max_len: int) -> str:
    if len(value) <= max_len:
        return value
    if max_len <= 3:
        return value[:max_len]
    return value[: max_len - 3] + "..."


def _shorten_paths(paths: list[str], max_len: int = 60) -> str:
    joined = "; ".join(paths)
    return _shorten_text(joined, max_len)


def apply_dark_theme(widget: QWidget) -> None:
    # Ported from DevTools/python/sots_stats_hub/stats_hub_gui.py (apply_dark_theme).
    palette = QtGui.QPalette()
    base = QtGui.QColor(30, 30, 30)
    alt = QtGui.QColor(45, 45, 45)
    text = QtGui.QColor(235, 235, 235)
    mid = QtGui.QColor(70, 70, 70)
    highlight = QtGui.QColor(60, 110, 180)
    palette.setColor(QtGui.QPalette.Window, base)
    palette.setColor(QtGui.QPalette.Base, alt)
    palette.setColor(QtGui.QPalette.AlternateBase, base)
    palette.setColor(QtGui.QPalette.Text, text)
    palette.setColor(QtGui.QPalette.WindowText, text)
    palette.setColor(QtGui.QPalette.Button, mid)
    palette.setColor(QtGui.QPalette.ButtonText, text)
    palette.setColor(QtGui.QPalette.Highlight, highlight)
    palette.setColor(QtGui.QPalette.HighlightedText, QtGui.QColor(255, 255, 255))
    QApplication.instance().setPalette(palette)
    widget.setStyleSheet(
        """
        QTreeWidget, QTableWidget, QPlainTextEdit {
            background-color: #1f1f1f;
            color: #e5e5e5;
            selection-background-color: #3a5a8c;
        }
        QLineEdit, QSpinBox, QComboBox {
            background-color: #2a2a2a;
            color: #e5e5e5;
            border: 1px solid #444;
        }
        QPushButton, QToolButton {
            background-color: #3a3a3a;
            color: #e5e5e5;
            border: 1px solid #555;
            padding: 4px 8px;
        }
        QPushButton:disabled, QToolButton:disabled {
            background-color: #2a2a2a;
            color: #777;
            border: 1px solid #444;
        }
        QPushButton:hover, QToolButton:hover {
            background-color: #4a4a4a;
        }
        QPushButton:pressed, QToolButton:pressed {
            background-color: #2f4f72;
            border-color: #3a5a8c;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            background: #1f1f1f;
            border: 1px solid #444;
            margin: 0px;
        }
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: #3a3a3a;
            border: 1px solid #555;
            min-height: 20px;
            min-width: 20px;
        }
        QScrollBar::handle:hover {
            background: #4a4a4a;
        }
        QScrollBar::add-line, QScrollBar::sub-line {
            background: #2a2a2a;
            border: 1px solid #555;
            subcontrol-origin: margin;
            height: 12px;
            width: 12px;
        }
        QScrollBar::add-page, QScrollBar::sub-page {
            background: #1a1a1a;
        }
        QTabWidget::pane {
            border: 1px solid #444;
        }
        QMenuBar, QMenu {
            background-color: #2a2a2a;
            color: #e5e5e5;
        }
        QMenu::item:selected {
            background-color: #3a5a8c;
        }
        QCheckBox, QRadioButton {
            color: #e5e5e5;
        }
        QTabBar::tab {
            background: #2a2a2a;
            color: #e5e5e5;
            border: 1px solid #444;
            padding: 6px 10px;
            margin-right: 1px;
        }
        QTabBar::tab:selected {
            background: #3a3a3a;
            border-color: #555;
        }
        QTabBar::tab:!selected {
            background: #2a2a2a;
            color: #c0c0c0;
        }
        QTabBar::tab:disabled {
            background: #262626;
            color: #777;
        }
        QHeaderView::section {
            background-color: #2a2a2a;
            color: #e5e5e5;
            border: 1px solid #444;
            padding: 4px;
        }
        """
    )


class SotsBackupGui(QMainWindow):
    def __init__(self, config: dict, startup_logger: ActionLogger) -> None:
        super().__init__()
        self.setWindowTitle("SOTS Restic Snapshot Manager")

        self.config = config
        self.log_dir = Path(config["log_dir"]).expanduser()
        self.notes_db = Path(config["notes_db"]).expanduser()
        self.restic_repo = Path(config["restic_repo"]).expanduser()
        self.passfile = Path(config["passfile"]).expanduser()
        self.restic_exe = str(config.get("restic_exe", "restic"))

        self.restic = ResticClient(
            restic_repo=self.restic_repo,
            passfile=self.passfile,
            log_dir=self.log_dir,
            restic_exe=self.restic_exe,
        )
        self.notes = NotesStore(self.notes_db)

        self._snapshots: list[dict[str, Any]] = []
        self._active_worker: ResticWorker | None = None
        self._refresh_after_worker = False

        self._build_ui()
        self._connect_signals()

        startup_logger.line(f"[startup] config={self._config_path()}")
        startup_logger.line(f"[startup] restic_repo={self.restic_repo}")
        startup_logger.line(f"[startup] notes_db={self.notes_db}")
        startup_logger.line(f"[startup] log_dir={self.log_dir}")
        startup_logger.close()

        QTimer.singleShot(200, self._startup)

    def _startup(self) -> None:
        if self._maybe_autodetect_restic():
            return
        self.refresh_snapshots()

    def _maybe_autodetect_restic(self) -> bool:
        # One-time bootstrapping: if restic isn't resolvable, scan C:\ once and persist.
        if bool(self.config.get("restic_autodetect_done", False)):
            return False

        # If the user already configured an explicit path, don't auto-scan.
        configured = str(self.config.get("restic_exe", "restic")).strip()
        if configured and configured.lower() not in {"restic"}:
            self.config["restic_autodetect_done"] = True
            try:
                _save_config(self._config_path(), self.config)
            except Exception:
                pass
            return False

        log_path = make_log_path(self.log_dir, "autodetect_restic")
        logger = ActionLogger(log_path, ui_callback=self._append_log_line)

        try:
            if self.restic.try_resolve_restic_exe(logger):
                self.config["restic_autodetect_done"] = True
                try:
                    _save_config(self._config_path(), self.config)
                except Exception:
                    pass
                return False
        finally:
            logger.close()

        def action(logger: ActionLogger) -> str | None:
            return scan_for_restic_exe_on_c_drive(logger)

        def on_result(found: str | None) -> None:
            self.config["restic_autodetect_done"] = True
            if found:
                self.config["restic_exe"] = found
                try:
                    _save_config(self._config_path(), self.config)
                except Exception as exc:
                    self._show_error("autodetect_restic", f"Failed to write config: {exc}")
                    return
                QMessageBox.information(
                    self,
                    "SOTS Backup GUI",
                    "Found restic.exe and saved it to gui_config.json.\n\n"
                    "Please restart the GUI.\n\n"
                    f"restic_exe={found}",
                )
                QTimer.singleShot(0, QApplication.instance().quit)
                return

            try:
                _save_config(self._config_path(), self.config)
            except Exception:
                pass

            self._show_error(
                "autodetect_restic",
                "restic.exe was not found after scanning C:\\ once.\n\n"
                "Install restic or set 'restic_exe' in DevTools/python/sots_backup_gui/gui_config.json to a full path.",
            )

        self._run_worker(action_name="autodetect_restic", action_fn=action, on_result=on_result)
        return True

    def _config_path(self) -> Path:
        return Path(__file__).resolve().parent / "gui_config.json"

    def _build_ui(self) -> None:
        central = QWidget()
        root_layout = QVBoxLayout(central)

        self.table = QTableWidget(0, 6)
        self.table.setHorizontalHeaderLabels(["Time", "ShortId", "Tags", "Host", "Paths", "Note"])
        self.table.setSelectionBehavior(QTableWidget.SelectRows)
        self.table.setSelectionMode(QTableWidget.SingleSelection)
        self.table.setEditTriggers(QTableWidget.NoEditTriggers)
        self.table.setAlternatingRowColors(True)
        self.table.verticalHeader().setVisible(False)

        details = QWidget()
        details_layout = QVBoxLayout(details)

        id_layout = QHBoxLayout()
        self.snapshot_id = QLineEdit()
        self.snapshot_id.setReadOnly(True)
        self.copy_id_btn = QPushButton("Copy")
        id_layout.addWidget(self.snapshot_id, 1)
        id_layout.addWidget(self.copy_id_btn)

        form = QFormLayout()
        form.addRow(QLabel("Snapshot ID"), id_layout)
        self.time_label = QLabel("-")
        form.addRow("Time", self.time_label)
        self.tags_label = QLabel("-")
        form.addRow("Tags", self.tags_label)
        self.tag_note_label = QLabel("-")
        form.addRow("Tag Note", self.tag_note_label)
        self.host_label = QLabel("-")
        form.addRow("Host/User", self.host_label)
        self.paths_view = QPlainTextEdit()
        self.paths_view.setReadOnly(True)
        self.paths_view.setMaximumBlockCount(200)
        form.addRow("Paths", self.paths_view)

        self.note_title = QLineEdit()
        form.addRow("Note Title", self.note_title)
        self.note_body = QTextEdit()
        self.note_body.setPlaceholderText(
            "Write a note for this snapshot (stored locally). Tag Note shows restic note_ tag."
        )
        form.addRow("Note", self.note_body)

        self.save_note_btn = QPushButton("Save Note")

        actions_layout = QHBoxLayout()
        self.refresh_btn = QPushButton("Refresh")
        self.restore_btn = QPushButton("Restore...")
        self.forget_btn = QPushButton("Forget Snapshot...")
        self.prune_btn = QPushButton("Prune...")
        actions_layout.addWidget(self.refresh_btn)
        actions_layout.addWidget(self.restore_btn)
        actions_layout.addWidget(self.forget_btn)
        actions_layout.addWidget(self.prune_btn)

        details_layout.addLayout(form)
        details_layout.addWidget(self.save_note_btn)
        details_layout.addLayout(actions_layout)
        details_layout.addStretch(1)

        top_split = QSplitter(Qt.Horizontal)
        top_split.addWidget(self.table)
        top_split.addWidget(details)
        top_split.setStretchFactor(0, 3)
        top_split.setStretchFactor(1, 2)

        self.log_panel = QPlainTextEdit()
        self.log_panel.setReadOnly(True)
        self.log_panel.setMaximumBlockCount(2000)
        self.log_panel.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.log_panel.setPlaceholderText("Action logs will appear here...")

        main_split = QSplitter(Qt.Vertical)
        main_split.addWidget(top_split)
        main_split.addWidget(self.log_panel)
        main_split.setStretchFactor(0, 3)
        main_split.setStretchFactor(1, 1)

        root_layout.addWidget(main_split)
        self.setCentralWidget(central)

    def _connect_signals(self) -> None:
        self.refresh_btn.clicked.connect(self.refresh_snapshots)
        self.restore_btn.clicked.connect(self.restore_snapshot)
        self.forget_btn.clicked.connect(self.forget_snapshot)
        self.prune_btn.clicked.connect(self.prune_repo)
        self.save_note_btn.clicked.connect(self.save_note)
        self.copy_id_btn.clicked.connect(self.copy_snapshot_id)
        self.table.itemSelectionChanged.connect(self._on_selection_changed)

    def _set_busy(self, busy: bool) -> None:
        for btn in [self.refresh_btn, self.restore_btn, self.forget_btn, self.prune_btn, self.save_note_btn]:
            btn.setEnabled(not busy)

    def _append_log_line(self, text: str) -> None:
        self.log_panel.appendPlainText(text)
        self.log_panel.ensureCursorVisible()

    def _run_worker(self, *, action_name: str, action_fn, on_result=None) -> None:
        if self._active_worker:
            self._append_log_line("[warn] Action already running.")
            return

        worker = ResticWorker(action_name=action_name, log_dir=self.log_dir, action_fn=action_fn)
        worker.log_line.connect(self._append_log_line)
        if on_result:
            worker.result.connect(on_result)
        worker.error.connect(lambda msg: self._show_error(action_name, msg))
        worker.finished.connect(self._on_worker_finished)
        self._active_worker = worker
        self._set_busy(True)
        worker.start()

    def _on_worker_finished(self) -> None:
        self._active_worker = None
        self._set_busy(False)
        if self._refresh_after_worker:
            self._refresh_after_worker = False
            self.refresh_snapshots()

    def _show_error(self, action: str, message: str) -> None:
        QMessageBox.critical(self, "SOTS Backup GUI", f"{action} failed:\n{message}")

    def refresh_snapshots(self) -> None:
        def action(logger: ActionLogger) -> list[dict[str, Any]]:
            self.restic.check_requirements(logger)
            data = self.notes.load(logger)
            notes_map = data.get("notes", {})
            snapshots = self.restic.list_snapshots(logger)
            for snap in snapshots:
                note_entry = notes_map.get(snap["id"], {})
                snap["note"] = str(note_entry.get("note", ""))
                snap["note_title"] = str(note_entry.get("title", ""))
                snap["note_updated_at"] = str(note_entry.get("updated_at", ""))
                tag_note = str(snap.get("tag_note", ""))
                tag_note_title = str(snap.get("tag_note_title", ""))
                snap["note_display"] = snap["note"] or tag_note or tag_note_title
            logger.line(f"[snapshots] count={len(snapshots)}")
            return snapshots

        def on_result(snapshots: list[dict[str, Any]]) -> None:
            prev_id = self.snapshot_id.text().strip()
            self._snapshots = snapshots
            selected = self._populate_table(prev_id if prev_id else None)
            if snapshots and not selected:
                self.table.selectRow(0)
            elif not snapshots:
                self._clear_details()

        self._run_worker(action_name="list_snapshots", action_fn=action, on_result=on_result)

    def _populate_table(self, selected_id: str | None = None) -> bool:
        selected = False
        self.table.setRowCount(len(self._snapshots))
        for row, snap in enumerate(self._snapshots):
            note_preview = _shorten_text(snap.get("note_display", ""), 60)
            tags = ", ".join(snap.get("tags", []))
            host = snap.get("hostname", "")
            if snap.get("username"):
                host = f"{host}/{snap.get('username')}"
            values = [
                snap.get("time", ""),
                snap.get("short_id", ""),
                tags,
                host,
                _shorten_paths(snap.get("paths", [])),
                note_preview,
            ]
            for col, value in enumerate(values):
                item = QTableWidgetItem(str(value))
                if col in (0, 1, 2, 3, 4, 5):
                    item.setFlags(item.flags() ^ Qt.ItemIsEditable)
                self.table.setItem(row, col, item)
            if selected_id and snap.get("id") == selected_id:
                self.table.selectRow(row)
                selected = True

        self.table.resizeColumnsToContents()
        self.table.resizeRowsToContents()
        return selected

    def _on_selection_changed(self) -> None:
        snap = self._selected_snapshot()
        if not snap:
            self._clear_details()
            return
        self.snapshot_id.setText(snap.get("id", ""))
        self.time_label.setText(snap.get("time", "-"))
        self.tags_label.setText(", ".join(snap.get("tags", [])) or "-")
        tag_note = snap.get("tag_note", "")
        tag_note_title = snap.get("tag_note_title", "")
        if tag_note_title and tag_note:
            self.tag_note_label.setText(f"{tag_note_title} | {tag_note}")
        else:
            self.tag_note_label.setText(tag_note_title or tag_note or "-")
        host = snap.get("hostname", "")
        user = snap.get("username", "")
        host_user = "/".join([p for p in [host, user] if p])
        self.host_label.setText(host_user or "-")
        self.paths_view.setPlainText("\n".join(snap.get("paths", [])))
        display_title = snap.get("note_title") or snap.get("tag_note_title", "")
        display_note = snap.get("note") or snap.get("tag_note", "")
        self.note_title.setText(display_title)
        self.note_body.setPlainText(display_note)

    def _clear_details(self) -> None:
        self.snapshot_id.setText("")
        self.time_label.setText("-")
        self.tags_label.setText("-")
        self.tag_note_label.setText("-")
        self.host_label.setText("-")
        self.paths_view.clear()
        self.note_title.clear()
        self.note_body.clear()

    def _selected_snapshot(self) -> dict[str, Any] | None:
        row = self.table.currentRow()
        if row < 0 or row >= len(self._snapshots):
            return None
        return self._snapshots[row]

    def copy_snapshot_id(self) -> None:
        snap = self._selected_snapshot()
        if not snap:
            return
        QGuiApplication.clipboard().setText(snap.get("id", ""))
        self._append_log_line("[ui] Snapshot ID copied to clipboard.")

    def save_note(self) -> None:
        snap = self._selected_snapshot()
        if not snap:
            return

        title = self.note_title.text()
        note = self.note_body.toPlainText()
        snapshot_id = snap.get("id", "")
        existing_tags = list(snap.get("tags", []))

        def action(logger: ActionLogger) -> dict[str, str]:
            logger.line(f"[action] save_note id={snapshot_id}")
            self.notes.set_note(snapshot_id, title=title, note=note, logger=logger)
            self.restic.update_note_tags(snapshot_id, title, note, existing_tags, logger)
            return {"id": snapshot_id, "title": title, "note": note}

        def on_result(payload: dict[str, str]) -> None:
            snap["note_title"] = payload.get("title", "").strip()
            snap["note"] = payload.get("note", "").strip()
            snap["note_display"] = snap["note"] or snap.get("tag_note", "") or snap.get("tag_note_title", "")
            self._populate_table(snap.get("id"))
            self._refresh_after_worker = True

        self._run_worker(action_name="save_note", action_fn=action, on_result=on_result)

    def restore_snapshot(self) -> None:
        snap = self._selected_snapshot()
        if not snap:
            return
        target = QFileDialog.getExistingDirectory(self, "Select Restore Target")
        if not target:
            return

        def action(logger: ActionLogger) -> str:
            logger.line(f"[restore] id={snap.get('id')} target={target}")
            self.restic.restore_snapshot(snap["id"], Path(target), logger=logger)
            return target

        def on_result(target_path: str) -> None:
            QMessageBox.information(self, "Restore Complete", f"Restored to:\n{target_path}")

        self._run_worker(action_name="restore", action_fn=action, on_result=on_result)

    def forget_snapshot(self) -> None:
        snap = self._selected_snapshot()
        if not snap:
            return

        confirm = QMessageBox(self)
        confirm.setWindowTitle("Forget Snapshot")
        confirm.setText("Forget this snapshot from the repo?")
        confirm.setInformativeText("This does NOT prune by default. Prune will be slower but reclaims space.")
        confirm.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        confirm.setDefaultButton(QMessageBox.No)
        prune_box = QCheckBox("Also prune after forget (slow)")
        confirm.setCheckBox(prune_box)
        result = confirm.exec()
        if result != QMessageBox.Yes:
            return

        do_prune = bool(prune_box.isChecked())

        def action(logger: ActionLogger) -> str:
            logger.line(f"[forget] id={snap.get('id')} prune={do_prune}")
            self.restic.forget_snapshot(snap["id"], prune=do_prune, logger=logger)
            self.notes.delete_note(snap["id"], logger=logger)
            return snap["id"]

        def on_result(_snapshot_id: str) -> None:
            self._refresh_after_worker = True

        self._run_worker(action_name="forget", action_fn=action, on_result=on_result)

    def prune_repo(self) -> None:
        confirm = QMessageBox(self)
        confirm.setWindowTitle("Prune Repo")
        confirm.setText("Run restic prune? This can take a while.")
        confirm.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        confirm.setDefaultButton(QMessageBox.No)
        if confirm.exec() != QMessageBox.Yes:
            return

        def action(logger: ActionLogger) -> str:
            logger.line("[prune] Starting prune")
            self.restic.prune_repo(logger)
            return "ok"

        def on_result(_result: str) -> None:
            QMessageBox.information(self, "Prune Complete", "Prune completed.")

        self._run_worker(action_name="prune", action_fn=action, on_result=on_result)


def main() -> int:
    print("[startup] SOTS Backup GUI launching")
    app = QApplication(sys.argv)

    config_path = Path(__file__).resolve().parent / "gui_config.json"
    log_dir = DEFAULT_LOG_DIR
    config: dict | None = None
    try:
        config = _load_config(config_path)
        log_dir = Path(config.get("log_dir", str(DEFAULT_LOG_DIR))).expanduser()
    except Exception as exc:
        logger = ActionLogger(make_log_path(log_dir, "startup"), ui_callback=None)
        logger.line(f"[startup] Failed to load config: {exc}")
        logger.close()
        QMessageBox.critical(
            None,
            "SOTS Backup GUI",
            f"Failed to load GUI config:\n{config_path}\n\n{exc}",
        )
        return 1

    logger = ActionLogger(make_log_path(log_dir, "startup"), ui_callback=None)
    try:
        logger.line("[startup] GUI config loaded.")
        window = SotsBackupGui(config, startup_logger=logger)
        apply_dark_theme(window)
        window.resize(1200, 800)
        window.show()
        return app.exec()
    except Exception as exc:
        logger.line(f"[startup] fatal: {type(exc).__name__}: {exc}")
        QMessageBox.critical(None, "SOTS Backup GUI", f"Startup failed:\n{exc}")
        return 1
    finally:
        logger.close()


if __name__ == "__main__":
    raise SystemExit(main())
