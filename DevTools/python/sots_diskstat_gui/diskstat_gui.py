from __future__ import annotations

import csv
import gzip
import json
import os
import subprocess
import sys
import threading
import time
import traceback
from datetime import datetime
from pathlib import Path
from typing import Optional

from . import __version__
from .duplicates import (
    DuplicateScanCancelled,
    DuplicateScanOptions,
    DuplicateScanStats,
    scan_duplicates,
)
from .model import FileEntry, Node, ScanStats
from .scanner import ScanCancelled, ScanOptions, scan_path
from .workers import ScanWorker
from .treemap import TreemapRect, build_treemap

try:
    from PySide6 import QtCore, QtGui, QtWidgets
except Exception as exc:  # noqa: BLE001
    QT_AVAILABLE = False
    QT_IMPORT_ERROR = exc
else:
    QT_AVAILABLE = True
    QT_IMPORT_ERROR = None

try:
    from send2trash import send2trash
except Exception:  # noqa: BLE001
    send2trash = None


def human_size(num: int) -> str:
    step = 1024.0
    units = ["B", "KB", "MB", "GB", "TB", "PB"]
    size = float(num)
    for unit in units:
        if size < step:
            return f"{size:.1f} {unit}"
        size /= step
    return f"{size:.1f} PB"


if QT_AVAILABLE:

    class DuplicateWorker(QtCore.QThread):
        progress = QtCore.Signal(str, int, int)  # path, files_scanned, candidates
        finished_ok = QtCore.Signal(object, object)  # groups, stats
        finished_cancelled = QtCore.Signal()
        finished_error = QtCore.Signal(str)

        def __init__(self, root: str, options: DuplicateScanOptions, parent: Optional[QtCore.QObject] = None) -> None:
            super().__init__(parent)
            self.root = root
            self.options = options
            self._cancel_event = threading.Event()

        def cancel(self) -> None:
            self._cancel_event.set()

        def run(self) -> None:  # noqa: D401
            try:
                groups, stats = scan_duplicates(
                    self.root,
                    options=self.options,
                    progress_cb=self.progress.emit,
                    cancel_cb=self._cancel_event.is_set,
                )
            except DuplicateScanCancelled:
                self.finished_cancelled.emit()
                return
            except Exception:
                self.finished_error.emit(traceback.format_exc())
                return

            self.finished_ok.emit(groups, stats)


    class DiskStatWindow(QtWidgets.QMainWindow):
        def __init__(self) -> None:
            super().__init__()
            self.setWindowTitle("SOTS DiskStat GUI")
            self.resize(1200, 800)

            self.root_node: Optional[Node] = None
            self.stats: Optional[ScanStats] = None
            self.scan_worker: Optional[ScanWorker] = None
            self.dup_worker: Optional[DuplicateWorker] = None
            self.dup_groups: list[dict] = []
            self.dup_stats: Optional[DuplicateScanStats] = None
            self.dup_last_root: Optional[str] = None
            self.dup_last_options: Optional[DuplicateScanOptions] = None
            self._path_to_treeitem: dict[str, QtWidgets.QTreeWidgetItem] = {}
            self._treemap_node: Optional[Node] = None
            self._treemap_render_timer = QtCore.QTimer(self)
            self._treemap_render_timer.setSingleShot(True)
            self._treemap_render_timer.timeout.connect(self._rerender_treemap)
            self.settings = QtCore.QSettings("SOTS", "DiskStatGUI")
            self._theme = self._load_theme_choice()
            self._quiet_mode = self._load_bool("ui/quiet_mode", True)
            self._exclusions_list = self._load_exclusions_list()
            self._exclusions_enabled = self._load_bool("scan/exclusions_enabled", True)
            self._exclusions_preset_last = self.settings.value("scan/exclusions_preset_last", "default")
            self._last_progress_log = 0.0
            self._suppress_history = False
            self._nav_history: list[str] = []
            self._nav_index = -1

            self._build_ui()
            self._apply_exclusion_ui_state()
            self._wire_signals()
            self._apply_theme(self._theme)
            self._update_buttons()

        def _build_ui(self) -> None:
            central = QtWidgets.QWidget()
            self.setCentralWidget(central)
            layout = QtWidgets.QVBoxLayout(central)
            v_splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
            layout.addWidget(v_splitter)
            top_container = QtWidgets.QWidget()
            top_layout = QtWidgets.QVBoxLayout(top_container)

            menubar = self.menuBar()
            view_menu = menubar.addMenu("View")
            self.dark_theme_action = QtGui.QAction("Dark Theme", self)
            self.dark_theme_action.setCheckable(True)
            self.dark_theme_action.setChecked(self._theme == "dark")
            self.dark_theme_action.triggered.connect(self._toggle_theme)
            view_menu.addAction(self.dark_theme_action)

            # Top controls
            top_row = QtWidgets.QHBoxLayout()
            self.path_edit = QtWidgets.QLineEdit(str(Path.cwd()))
            self.browse_btn = QtWidgets.QPushButton("Browse")
            self.back_btn = QtWidgets.QPushButton("Back")
            self.forward_btn = QtWidgets.QPushButton("Forward")
            self.up_btn = QtWidgets.QPushButton("Up")
            self.scan_btn = QtWidgets.QPushButton("Scan")
            self.cancel_btn = QtWidgets.QPushButton("Cancel")
            self.cancel_btn.setEnabled(False)
            self.load_btn = QtWidgets.QPushButton("Load Snapshot")
            self.save_btn = QtWidgets.QPushButton("Save Snapshot")

            top_row.addWidget(QtWidgets.QLabel("Root:"))
            top_row.addWidget(self.path_edit, stretch=1)
            top_row.addWidget(self.browse_btn)
            top_row.addWidget(self.back_btn)
            top_row.addWidget(self.forward_btn)
            top_row.addWidget(self.up_btn)
            top_row.addWidget(self.scan_btn)
            top_row.addWidget(self.cancel_btn)
            top_row.addWidget(self.load_btn)
            top_row.addWidget(self.save_btn)
            top_layout.addLayout(top_row)

            # Options row
            opts_row = QtWidgets.QHBoxLayout()
            self.skip_symlinks_cb = QtWidgets.QCheckBox("Skip symlinks")
            self.skip_symlinks_cb.setChecked(True)
            self.depth_spin = QtWidgets.QSpinBox()
            self.depth_spin.setRange(0, 64)
            self.depth_spin.setValue(0)
            self.quiet_mode_cb = QtWidgets.QCheckBox("Quiet mode")
            self.quiet_mode_cb.setChecked(self._quiet_mode)
            opts_row.addWidget(self.skip_symlinks_cb)
            opts_row.addWidget(QtWidgets.QLabel("Max depth (0=unlimited):"))
            opts_row.addWidget(self.depth_spin)
            opts_row.addWidget(self.quiet_mode_cb)
            opts_row.addStretch(1)
            top_layout.addLayout(opts_row)

            # Main splitter: tree + tabs
            splitter = QtWidgets.QSplitter()
            splitter.setOrientation(QtCore.Qt.Horizontal)

            self.tree = QtWidgets.QTreeWidget()
            self.tree.setHeaderLabels(["Name", "Size", "Items"])
            self.tree.header().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
            self.tree.header().setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
            self.tree.header().setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
            splitter.addWidget(self.tree)

            self.tab = QtWidgets.QTabWidget()

            # Treemap tab
            self.treemap_view = QtWidgets.QGraphicsView()
            self.treemap_view.setRenderHint(QtGui.QPainter.Antialiasing)
            self.treemap_scene = QtWidgets.QGraphicsScene()
            self.treemap_view.setScene(self.treemap_scene)
            treemap_container = QtWidgets.QVBoxLayout()
            treemap_widget = QtWidgets.QWidget()
            treemap_widget.setLayout(treemap_container)
            breadcrumb_widget = QtWidgets.QWidget()
            self.breadcrumb_bar = QtWidgets.QHBoxLayout(breadcrumb_widget)
            self.breadcrumb_bar.setContentsMargins(0, 0, 0, 0)
            treemap_container.addWidget(breadcrumb_widget)
            treemap_container.addWidget(self.treemap_view)
            self.tab.addTab(treemap_widget, "Treemap")

            # Largest tab
            self.largest_table = QtWidgets.QTableWidget(0, 4)
            self.largest_table.setHorizontalHeaderLabels(["Type", "Name", "Size", "Path"])
            self.largest_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
            self.largest_table.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
            self.largest_table.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
            self.largest_table.horizontalHeader().setSectionResizeMode(3, QtWidgets.QHeaderView.Stretch)
            self.largest_table.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
            self.largest_table.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
            largest_container = QtWidgets.QVBoxLayout()
            largest_widget = QtWidgets.QWidget()
            largest_widget.setLayout(largest_container)
            largest_container.addWidget(self.largest_table)
            self.tab.addTab(largest_widget, "Largest")

            # File types tab
            self.filetypes_table = QtWidgets.QTableWidget(0, 4)
            self.filetypes_table.setHorizontalHeaderLabels(["Extension", "Count", "Total Size", "Percent of Node"])
            self.filetypes_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
            self.filetypes_table.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
            self.filetypes_table.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
            self.filetypes_table.horizontalHeader().setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeToContents)
            self.filetypes_table.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
            self.filetypes_table.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
            filetypes_container = QtWidgets.QVBoxLayout()
            filetypes_widget = QtWidgets.QWidget()
            filetypes_widget.setLayout(filetypes_container)
            filetypes_container.addWidget(self.filetypes_table)
            self.tab.addTab(filetypes_widget, "File Types")

            # Duplicates tab
            dup_widget = QtWidgets.QWidget()
            dup_layout = QtWidgets.QVBoxLayout(dup_widget)

            dup_target_row = QtWidgets.QHBoxLayout()
            self.dup_target_current = QtWidgets.QRadioButton("Scan current node")
            self.dup_target_root = QtWidgets.QRadioButton("Scan full root")
            self.dup_target_current.setChecked(True)
            dup_target_row.addWidget(self.dup_target_current)
            dup_target_row.addWidget(self.dup_target_root)
            dup_target_row.addStretch(1)
            dup_layout.addLayout(dup_target_row)

            dup_opts_row = QtWidgets.QHBoxLayout()
            dup_opts_row.addWidget(QtWidgets.QLabel("Min size (MB):"))
            self.dup_min_size = QtWidgets.QSpinBox()
            self.dup_min_size.setRange(1, 1024 * 10)
            self.dup_min_size.setValue(1)
            dup_opts_row.addWidget(self.dup_min_size)
            dup_opts_row.addWidget(QtWidgets.QLabel("Hash mode:"))
            self.dup_hash_mode = QtWidgets.QComboBox()
            self.dup_hash_mode.addItem("Quick+Full", userData="quick_then_full")
            self.dup_hash_mode.addItem("Full Only", userData="full_only")
            dup_opts_row.addWidget(self.dup_hash_mode)
            dup_opts_row.addStretch(1)
            dup_layout.addLayout(dup_opts_row)

            dup_buttons_row = QtWidgets.QHBoxLayout()
            self.dup_scan_btn = QtWidgets.QPushButton("Scan Duplicates")
            self.dup_cancel_btn = QtWidgets.QPushButton("Cancel")
            self.dup_cancel_btn.setEnabled(False)
            self.dup_export_csv_btn = QtWidgets.QPushButton("Export CSV…")
            self.dup_export_json_btn = QtWidgets.QPushButton("Export JSON…")
            dup_buttons_row.addWidget(self.dup_scan_btn)
            dup_buttons_row.addWidget(self.dup_cancel_btn)
            dup_buttons_row.addWidget(self.dup_export_csv_btn)
            dup_buttons_row.addWidget(self.dup_export_json_btn)
            dup_buttons_row.addStretch(1)
            dup_layout.addLayout(dup_buttons_row)

            self.dup_table = QtWidgets.QTableWidget(0, 4)
            self.dup_table.setHorizontalHeaderLabels(["Group", "Size", "Count", "Path"])
            self.dup_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
            self.dup_table.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
            self.dup_table.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
            self.dup_table.horizontalHeader().setSectionResizeMode(3, QtWidgets.QHeaderView.Stretch)
            self.dup_table.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
            self.dup_table.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
            self.dup_table.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
            dup_layout.addWidget(self.dup_table, stretch=1)

            self.tab.addTab(dup_widget, "Duplicates")

            splitter.addWidget(self.tab)
            splitter.setStretchFactor(1, 3)
            top_layout.addWidget(splitter, stretch=1)

            # Bottom actions + status
            actions_row = QtWidgets.QHBoxLayout()
            self.open_btn = QtWidgets.QPushButton("Open in Explorer")
            self.copy_btn = QtWidgets.QPushButton("Copy Path")
            self.enable_delete_cb = QtWidgets.QCheckBox("Enable destructive actions")
            self.delete_btn = QtWidgets.QPushButton("Delete (Recycle Bin)")
            self.delete_btn.setEnabled(False)
            self.status_label = QtWidgets.QLabel("Idle")
            actions_row.addWidget(self.open_btn)
            actions_row.addWidget(self.copy_btn)
            actions_row.addWidget(self.enable_delete_cb)
            actions_row.addWidget(self.delete_btn)
            actions_row.addStretch(1)
            actions_row.addWidget(self.status_label)
            top_layout.addLayout(actions_row)

            # Log panel
            self.log_view = QtWidgets.QPlainTextEdit()
            self.log_view.setReadOnly(True)
            self.log_view.setMaximumBlockCount(2000)
            exclusions_widget = QtWidgets.QWidget()
            exclusions_layout = QtWidgets.QVBoxLayout(exclusions_widget)
            exclusions_layout.setContentsMargins(4, 4, 4, 4)
            exclusions_row = QtWidgets.QHBoxLayout()
            self.exclusions_enable_cb = QtWidgets.QCheckBox("Enable exclusions")
            self.exclusions_enable_cb.setChecked(self._exclusions_enabled)
            self.exclusions_preset_windows_btn = QtWidgets.QPushButton("Preset: Windows")
            self.exclusions_preset_unreal_btn = QtWidgets.QPushButton("Preset: Unreal/SOTS")
            self.exclusions_reset_btn = QtWidgets.QPushButton("Reset")
            exclusions_row.addWidget(QtWidgets.QLabel("Exclusions:"))
            exclusions_row.addWidget(self.exclusions_enable_cb)
            exclusions_row.addWidget(self.exclusions_preset_windows_btn)
            exclusions_row.addWidget(self.exclusions_preset_unreal_btn)
            exclusions_row.addWidget(self.exclusions_reset_btn)
            exclusions_row.addStretch(1)
            exclusions_layout.addLayout(exclusions_row)
            self.exclusions_edit = QtWidgets.QPlainTextEdit()
            self.exclusions_edit.setPlaceholderText("One pattern per line (case-insensitive substring match)")
            self.exclusions_edit.setPlainText("\n".join(self._exclusions_list))
            exclusions_layout.addWidget(self.exclusions_edit)

            v_splitter.addWidget(top_container)
            v_splitter.addWidget(exclusions_widget)
            v_splitter.addWidget(self.log_view)
            v_splitter.setStretchFactor(0, 5)
            v_splitter.setStretchFactor(1, 2)
            v_splitter.setStretchFactor(2, 1)

        def _wire_signals(self) -> None:
            self.browse_btn.clicked.connect(self.on_browse)
            self.up_btn.clicked.connect(self.on_up)
            self.back_btn.clicked.connect(self.on_back)
            self.forward_btn.clicked.connect(self.on_forward)
            self.scan_btn.clicked.connect(self.on_scan)
            self.cancel_btn.clicked.connect(self.on_cancel)
            self.load_btn.clicked.connect(self.on_load_snapshot)
            self.save_btn.clicked.connect(self.on_save_snapshot)

            self.tree.itemSelectionChanged.connect(self.on_tree_selection)
            self.open_btn.clicked.connect(self.on_open)
            self.copy_btn.clicked.connect(self.on_copy)
            self.delete_btn.clicked.connect(self.on_delete)
            self.enable_delete_cb.stateChanged.connect(self._update_buttons)
            self.dup_scan_btn.clicked.connect(self.on_dup_scan)
            self.dup_cancel_btn.clicked.connect(self.on_dup_cancel)
            self.dup_export_csv_btn.clicked.connect(self.on_dup_export_csv)
            self.dup_export_json_btn.clicked.connect(self.on_dup_export_json)
            self.dup_table.doubleClicked.connect(self.on_dup_open)
            self.dup_table.customContextMenuRequested.connect(self.on_dup_context_menu)
            self.quiet_mode_cb.toggled.connect(self._on_quiet_mode_changed)
            self.exclusions_enable_cb.toggled.connect(self._on_exclusions_enabled_changed)
            self.exclusions_preset_windows_btn.clicked.connect(lambda: self._apply_exclusions_preset("windows"))
            self.exclusions_preset_unreal_btn.clicked.connect(lambda: self._apply_exclusions_preset("unreal"))
            self.exclusions_reset_btn.clicked.connect(lambda: self._apply_exclusions_preset("default"))
            self.exclusions_edit.textChanged.connect(self._on_exclusions_changed)

        def _load_theme_choice(self) -> str:
            try:
                theme = self.settings.value("ui/theme", "dark")
            except Exception:
                theme = "dark"
            return "dark" if theme not in {"dark", "light"} else theme

        def _load_bool(self, key: str, default: bool) -> bool:
            try:
                val = self.settings.value(key, default)
            except Exception:
                return default
            if isinstance(val, str):
                return val.lower() in {"1", "true", "yes", "on"}
            try:
                return bool(val)
            except Exception:
                return default

        def _windows_exclusions(self) -> list[str]:
            return ["$recycle.bin", "system volume information", "windows\\winsxs", "temp", "windows\\temp"]

        def _unreal_exclusions(self) -> list[str]:
            return ["deriveddatacache", "intermediate", "binaries", "saved", ".vs", ".git", "bep_exports", "node_modules"]

        def _default_exclusions(self) -> list[str]:
            seen: set[str] = set()
            combined: list[str] = []
            for pat in self._windows_exclusions() + self._unreal_exclusions():
                low = pat.lower()
                if low in seen:
                    continue
                seen.add(low)
                combined.append(pat)
            return combined

        def _load_exclusions_list(self) -> list[str]:
            try:
                raw = self.settings.value("scan/exclusions_list", [])
            except Exception:
                raw = []
            if isinstance(raw, str):
                raw_list = [line.strip() for line in raw.splitlines() if line.strip()]
            else:
                raw_list = [str(x).strip() for x in (raw or []) if str(x).strip()]
            return raw_list or self._default_exclusions()

        def _apply_theme(self, theme: str) -> None:
            is_dark = theme == "dark"
            palette = QtGui.QPalette()
            if is_dark:
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
            else:
                palette = self.style().standardPalette()
            QtWidgets.QApplication.instance().setPalette(palette)

            if is_dark:
                self.setStyleSheet(
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
            else:
                self.setStyleSheet("")
            bg = QtGui.QColor(25, 25, 25) if is_dark else QtGui.QColor(255, 255, 255)
            self.treemap_view.setBackgroundBrush(QtGui.QBrush(bg))

        def _toggle_theme(self, checked: bool) -> None:
            self._theme = "dark" if checked else "light"
            self.settings.setValue("ui/theme", self._theme)
            self._apply_theme(self._theme)
            self.dark_theme_action.setChecked(self._theme == "dark")

        def append_log(self, msg: str) -> None:
            timestamp = datetime.now().strftime("%H:%M:%S")
            self.log_view.appendPlainText(f"[{timestamp}] {msg}")
            self.log_view.verticalScrollBar().setValue(self.log_view.verticalScrollBar().maximum())

        def _on_scan_progress(self, msg: str) -> None:
            now = time.time()
            is_progress = msg.startswith(("[DIR]", "[PROGRESS]"))
            important = msg.startswith(("[WARN]", "[ERROR]", "[SCAN]"))
            interval = 0.15 if not self.quiet_mode_cb.isChecked() else 0.75
            if is_progress and not important:
                if now - self._last_progress_log < interval:
                    return
                self._last_progress_log = now
            self.append_log(msg)
            if msg.startswith("[PROGRESS]"):
                self.status_label.setText(msg.replace("[PROGRESS] ", ""))

        def _parse_exclusions_text(self) -> list[str]:
            return [line.strip() for line in self.exclusions_edit.toPlainText().splitlines() if line.strip()]

        def _current_exclusions(self) -> list[str]:
            if not self.exclusions_enable_cb.isChecked():
                return []
            return [pat.lower() for pat in self._parse_exclusions_text()]

        def _apply_exclusion_ui_state(self) -> None:
            enabled = self.exclusions_enable_cb.isChecked()
            self.exclusions_edit.setReadOnly(not enabled)

        def _save_exclusions_settings(self) -> None:
            self._exclusions_list = self._parse_exclusions_text()
            self.settings.setValue("scan/exclusions_enabled", self.exclusions_enable_cb.isChecked())
            self.settings.setValue("scan/exclusions_list", self._exclusions_list)
            self.settings.setValue("scan/exclusions_preset_last", self._exclusions_preset_last or "")

        def _apply_exclusions_preset(self, preset: str) -> None:
            if preset == "windows":
                patterns = self._windows_exclusions()
            elif preset == "unreal":
                patterns = self._unreal_exclusions()
            else:
                patterns = self._default_exclusions()
            self.exclusions_enable_cb.setChecked(True)
            self.exclusions_edit.blockSignals(True)
            self.exclusions_edit.setPlainText("\n".join(patterns))
            self.exclusions_edit.blockSignals(False)
            self._exclusions_preset_last = preset
            self._save_exclusions_settings()
            self._apply_exclusion_ui_state()

        def _on_exclusions_enabled_changed(self, checked: bool) -> None:  # noqa: ARG002
            self._apply_exclusion_ui_state()
            self._save_exclusions_settings()

        def _on_exclusions_changed(self) -> None:
            self._save_exclusions_settings()

        def _on_quiet_mode_changed(self, checked: bool) -> None:  # noqa: ARG002
            self._quiet_mode = checked
            self.settings.setValue("ui/quiet_mode", self._quiet_mode)

        def on_browse(self) -> None:
            dir_path = QtWidgets.QFileDialog.getExistingDirectory(self, "Select root", self.path_edit.text())
            if dir_path:
                self.path_edit.setText(dir_path)

        def on_up(self) -> None:
            current = Path(self.path_edit.text()).resolve()
            parent = current.parent if current != current.parent else current
            self.path_edit.setText(str(parent))

        def on_scan(self) -> None:
            if self.scan_worker and self.scan_worker.isRunning():
                return

            root = self.path_edit.text().strip()
            if not root:
                QtWidgets.QMessageBox.warning(self, "Scan", "Please provide a root path.")
                return

            opts = ScanOptions(
                skip_symlinks=self.skip_symlinks_cb.isChecked(),
                max_depth=self.depth_spin.value(),
                exclusions=self._current_exclusions(),
                quiet_mode=self.quiet_mode_cb.isChecked(),
            )
            self._save_exclusions_settings()
            self._last_progress_log = 0.0
            self.append_log(f"[UI] Starting scan for {root}")
            self.status_label.setText("Scanning...")
            self.scan_worker = ScanWorker(root, opts)
            self.scan_worker.progress.connect(self._on_scan_progress)
            self.scan_worker.finished.connect(self._on_scan_finished)
            self.scan_worker.cancelled.connect(self._on_scan_cancelled)
            self.scan_worker.failed.connect(self._on_scan_failed)
            self.scan_btn.setEnabled(False)
            self.cancel_btn.setEnabled(True)
            self.scan_worker.start()

        def on_cancel(self) -> None:
            if self.scan_worker and self.scan_worker.isRunning():
                self.append_log("[UI] Cancelling scan...")
                self.scan_worker.cancel()

        def _on_scan_finished(self, node: Node, stats: ScanStats) -> None:
            self.append_log("[UI] Scan finished.")
            self.status_label.setText("Scan complete")
            self.scan_btn.setEnabled(True)
            self.cancel_btn.setEnabled(False)
            self.root_node = node
            self.stats = stats
            node.rebuild_parents(None)
            self._reset_navigation_history(node.path)
            self._populate_tree(push_history=False)
            self._update_details(node)

        def _on_scan_cancelled(self) -> None:
            self.append_log("[UI] Scan cancelled.")
            self.status_label.setText("Cancelled")
            self.scan_btn.setEnabled(True)
            self.cancel_btn.setEnabled(False)

        def _on_scan_failed(self, error: str) -> None:
            self.append_log("[ERROR] Scan failed.")
            self.append_log(error)
            self.status_label.setText("Failed")
            QtWidgets.QMessageBox.critical(self, "Scan failed", error)
            self.scan_btn.setEnabled(True)
            self.cancel_btn.setEnabled(False)

        def _populate_tree(self, push_history: bool = True) -> None:
            self.tree.clear()
            self._path_to_treeitem.clear()
            if not self.root_node:
                return

            def add_item(node: Node, parent_item: Optional[QtWidgets.QTreeWidgetItem]) -> None:
                items_count = len(node.children)
                item = QtWidgets.QTreeWidgetItem([node.name, human_size(node.size), str(items_count)])
                item.setData(0, QtCore.Qt.UserRole, node)
                self._path_to_treeitem[str(Path(node.path).resolve())] = item
                if parent_item is None:
                    self.tree.addTopLevelItem(item)
                else:
                    parent_item.addChild(item)
                for child in node.children:
                    add_item(child, item)

            add_item(self.root_node, None)
            self.tree.expandToDepth(1)
            self.tree.resizeColumnToContents(0)
            if self.tree.topLevelItemCount() > 0:
                self._select_tree_path(self.tree.topLevelItem(0).data(0, QtCore.Qt.UserRole).path, push_history=push_history)

        def on_tree_selection(self) -> None:
            node = self._selected_node()
            if node:
                if not self._suppress_history:
                    self._record_history(node.path)
                self._update_details(node)
            self._suppress_history = False
            self._update_buttons()
            self._update_history_buttons()

        def _selected_node(self) -> Optional[Node]:
            items = self.tree.selectedItems()
            if not items:
                return None
            node = items[0].data(0, QtCore.Qt.UserRole)
            return node

        def _select_tree_path(self, path: str, push_history: bool = True) -> None:
            if not path:
                return
            key = str(Path(path).resolve())
            item = self._path_to_treeitem.get(key)
            if item:
                self._suppress_history = not push_history
                self.tree.setCurrentItem(item)
                self.tree.scrollToItem(item)
            else:
                self._suppress_history = False

        def _record_history(self, path: str) -> None:
            if not path:
                return
            resolved = str(Path(path).resolve())
            if self._nav_history and self._nav_history[self._nav_index] == resolved:
                return
            if self._nav_index < len(self._nav_history) - 1:
                self._nav_history = self._nav_history[: self._nav_index + 1]
            self._nav_history.append(resolved)
            self._nav_index = len(self._nav_history) - 1
            self._update_history_buttons()

        def _reset_navigation_history(self, path: str) -> None:
            resolved = str(Path(path).resolve())
            self._nav_history = [resolved]
            self._nav_index = 0
            self._update_history_buttons()

        def _update_history_buttons(self) -> None:
            self.back_btn.setEnabled(self._nav_index > 0)
            self.forward_btn.setEnabled(self._nav_index < len(self._nav_history) - 1)

        def on_back(self) -> None:
            if self._nav_index <= 0:
                return
            self._nav_index -= 1
            target = self._nav_history[self._nav_index]
            self._select_tree_path(target, push_history=False)
            self._update_history_buttons()

        def on_forward(self) -> None:
            if self._nav_index >= len(self._nav_history) - 1:
                return
            self._nav_index += 1
            target = self._nav_history[self._nav_index]
            self._select_tree_path(target, push_history=False)
            self._update_history_buttons()

        def _zoom_up(self) -> None:
            if self._treemap_node and self._treemap_node.parent:
                self._select_tree_path(self._treemap_node.parent.path)

        def _update_details(self, node: Node) -> None:
            self.status_label.setText(f"{node.name} | {human_size(node.size)}")
            self._populate_largest(node)
            self._populate_filetypes(node)
            self._populate_treemap(node)
            self._refresh_breadcrumbs(node)
            self._update_buttons()

        def resizeEvent(self, event: QtGui.QResizeEvent) -> None:  # type: ignore[override]
            super().resizeEvent(event)
            self._treemap_render_timer.start(50)

        def _rerender_treemap(self) -> None:
            if self._treemap_node:
                self._populate_treemap(self._treemap_node)

        def _populate_largest(self, node: Node) -> None:
            rows: list[tuple[str, str, int, str]] = []
            for child in node.children:
                rows.append(("Dir", child.name, child.size, child.path))
            for f in node.top_files:
                rows.append(("File", f.name, f.size, f.path))
            rows.sort(key=lambda r: r[2], reverse=True)

            self.largest_table.setRowCount(len(rows))
            for i, row in enumerate(rows):
                type_item = QtWidgets.QTableWidgetItem(row[0])
                name_item = QtWidgets.QTableWidgetItem(row[1])
                size_item = QtWidgets.QTableWidgetItem(human_size(row[2]))
                path_item = QtWidgets.QTableWidgetItem(row[3])
                for item in (type_item, name_item, size_item, path_item):
                    item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                self.largest_table.setItem(i, 0, type_item)
                self.largest_table.setItem(i, 1, name_item)
                self.largest_table.setItem(i, 2, size_item)
                self.largest_table.setItem(i, 3, path_item)

        def _populate_filetypes(self, node: Node) -> None:
            totals: dict[str, tuple[int, int]] = {}
            for f in node.top_files:
                ext = f.extension or "<none>"
                count, size = totals.get(ext, (0, 0))
                totals[ext] = (count + 1, size + f.size)

            rows = []
            for ext, (count, size) in totals.items():
                pct = (size / node.size * 100.0) if node.size else 0.0
                rows.append((ext, count, size, pct))
            rows.sort(key=lambda r: r[2], reverse=True)

            self.filetypes_table.setRowCount(len(rows))
            for i, (ext, count, size, pct) in enumerate(rows):
                ext_item = QtWidgets.QTableWidgetItem(ext)
                count_item = QtWidgets.QTableWidgetItem(str(count))
                size_item = QtWidgets.QTableWidgetItem(human_size(size))
                pct_item = QtWidgets.QTableWidgetItem(f"{pct:.1f}%")
                for item in (ext_item, count_item, size_item, pct_item):
                    item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                self.filetypes_table.setItem(i, 0, ext_item)
                self.filetypes_table.setItem(i, 1, count_item)
                self.filetypes_table.setItem(i, 2, size_item)
                self.filetypes_table.setItem(i, 3, pct_item)

        def _populate_treemap(self, node: Node) -> None:
            self.treemap_scene.clear()
            self._treemap_node = node
            items = [(child.name, child.size, child.path) for child in node.children if child.size > 0]
            if not items and node.top_files:
                items = [(f.name, f.size, f.path) for f in node.top_files]
            view_size = self.treemap_view.viewport().size()
            rects: list[TreemapRect] = build_treemap(
                items,
                max(view_size.width(), 10),
                max(view_size.height(), 10),
                padding=2.0,
                gutter=4.0,
            )
            if not rects:
                return

            for rect in rects:
                color = QtGui.QColor.fromHsv(hash(rect.path) % 360, 180, 220, 180)
                item = _TreemapRectItem(rect, color, self)
                self.treemap_scene.addItem(item)
                self.treemap_scene.addItem(item.text_item)
            self.treemap_scene.setSceneRect(self.treemap_scene.itemsBoundingRect())
            self.treemap_view.fitInView(self.treemap_scene.sceneRect(), QtCore.Qt.KeepAspectRatio)

        def _refresh_breadcrumbs(self, node: Optional[Node]) -> None:
            while self.breadcrumb_bar.count():
                child = self.breadcrumb_bar.takeAt(0)
                widget = child.widget()
                if widget is not None:
                    widget.deleteLater()
            if not node:
                self.breadcrumb_bar.addWidget(QtWidgets.QLabel("No selection"))
                self.breadcrumb_bar.addStretch(1)
                return
            chain: list[Node] = []
            cur = node
            while cur:
                chain.append(cur)
                cur = cur.parent
            chain.reverse()
            for idx, crumb in enumerate(chain):
                btn = QtWidgets.QToolButton()
                btn.setText(crumb.name or crumb.path)
                btn.setAutoRaise(True)
                btn.clicked.connect(lambda _checked=False, p=crumb.path: self._select_tree_path(p, push_history=True))
                self.breadcrumb_bar.addWidget(btn)
                if idx < len(chain) - 1:
                    self.breadcrumb_bar.addWidget(QtWidgets.QLabel(">"))
            self.breadcrumb_bar.addStretch(1)

        def _dup_target_path(self) -> Optional[str]:
            if self.dup_target_current.isChecked():
                node = self._selected_node()
                if node:
                    return node.path
                if self.root_node:
                    return self.root_node.path
            return self.path_edit.text().strip()

        def on_dup_scan(self) -> None:
            if self.dup_worker and self.dup_worker.isRunning():
                return

            target = self._dup_target_path()
            if not target:
                QtWidgets.QMessageBox.warning(self, "Duplicates", "Select a folder to scan.")
                return

            opts = DuplicateScanOptions(
                skip_symlinks=self.skip_symlinks_cb.isChecked(),
                min_size_bytes=self.dup_min_size.value() * 1024 * 1024,
                hash_mode=self.dup_hash_mode.currentData(),
            )
            self.dup_last_root = target
            self.dup_last_options = opts
            self.dup_groups = []
            self.dup_stats = None
            self.dup_table.setRowCount(0)
            self.append_log(f"[UI] Starting duplicate scan: {target}")
            self.status_label.setText("Duplicate scan running...")

            self.dup_worker = DuplicateWorker(target, opts)
            self.dup_worker.progress.connect(self._on_dup_progress)
            self.dup_worker.finished_ok.connect(self._on_dup_finished)
            self.dup_worker.finished_cancelled.connect(self._on_dup_cancelled)
            self.dup_worker.finished_error.connect(self._on_dup_failed)
            self.dup_worker.start()
            self._update_buttons()

        def _on_dup_progress(self, path: str, files: int, candidates: int) -> None:
            self.status_label.setText(f"Duplicates: {files} files, {candidates} size buckets")
            if files % 400 == 0:
                self.append_log(f"[DUP] files={files} buckets={candidates} path={path}")

        def _on_dup_finished(self, groups: list[dict], stats: DuplicateScanStats) -> None:
            self.dup_groups = groups
            self.dup_stats = stats
            self._populate_dup_table()
            self.append_log(
                f"[UI] Duplicates done: groups={stats.dup_groups} files={stats.dup_files} "
                f"wasted={human_size(stats.wasted_bytes)}"
            )
            self.status_label.setText("Duplicate scan complete")
            self.dup_worker = None
            self._update_buttons()

        def _populate_dup_table(self) -> None:
            total_rows = sum(len(g["paths"]) for g in self.dup_groups)
            self.dup_table.setRowCount(total_rows)
            row_idx = 0
            for gi, group in enumerate(self.dup_groups, start=1):
                size_h = human_size(group["size"])
                count_str = str(len(group["paths"]))
                for path in group["paths"]:
                    group_item = QtWidgets.QTableWidgetItem(str(gi))
                    size_item = QtWidgets.QTableWidgetItem(size_h)
                    count_item = QtWidgets.QTableWidgetItem(count_str)
                    path_item = QtWidgets.QTableWidgetItem(path)
                    path_item.setData(QtCore.Qt.UserRole, path)
                    for item in (group_item, size_item, count_item, path_item):
                        item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                    self.dup_table.setItem(row_idx, 0, group_item)
                    self.dup_table.setItem(row_idx, 1, size_item)
                    self.dup_table.setItem(row_idx, 2, count_item)
                    self.dup_table.setItem(row_idx, 3, path_item)
                    row_idx += 1

        def on_dup_cancel(self) -> None:
            if self.dup_worker and self.dup_worker.isRunning():
                self.append_log("[UI] Cancelling duplicate scan...")
                self.dup_worker.cancel()

        def _on_dup_cancelled(self) -> None:
            self.append_log("[UI] Duplicate scan cancelled.")
            self.status_label.setText("Duplicate scan cancelled")
            self.dup_worker = None
            self._update_buttons()

        def _on_dup_failed(self, error: str) -> None:
            self.append_log("[ERROR] Duplicate scan failed.")
            self.append_log(error)
            QtWidgets.QMessageBox.critical(self, "Duplicates failed", error)
            self.status_label.setText("Duplicate scan failed")
            self.dup_worker = None
            self._update_buttons()

        def _dup_row_path(self, row: int) -> Optional[str]:
            item = self.dup_table.item(row, 3)
            if not item:
                return None
            return item.data(QtCore.Qt.UserRole) or item.text()

        def on_dup_open(self, index: QtCore.QModelIndex) -> None:
            if not index.isValid():
                return
            path = self._dup_row_path(index.row())
            if path:
                self._open_in_explorer(path)

        def on_dup_context_menu(self, pos: QtCore.QPoint) -> None:
            index = self.dup_table.indexAt(pos)
            if not index.isValid():
                return
            path = self._dup_row_path(index.row())
            if not path:
                return
            menu = QtWidgets.QMenu(self)
            open_action = menu.addAction("Open in Explorer")
            copy_action = menu.addAction("Copy Path")
            action = menu.exec_(self.dup_table.viewport().mapToGlobal(pos))
            if action == open_action:
                self._open_in_explorer(path)
            elif action == copy_action:
                QtWidgets.QApplication.clipboard().setText(path)
                self.append_log(f"[UI] Copied path: {path}")

        def on_dup_export_csv(self) -> None:
            if not self.dup_groups:
                QtWidgets.QMessageBox.information(self, "Export CSV", "Run a duplicate scan first.")
                return
            file_path, _ = QtWidgets.QFileDialog.getSaveFileName(
                self,
                "Export duplicates CSV",
                "duplicates.csv",
                "CSV files (*.csv);;All files (*.*)",
            )
            if not file_path:
                return
            try:
                with open(file_path, "w", newline="", encoding="utf-8") as fh:
                    writer = csv.writer(fh)
                    writer.writerow(["group_id", "size_bytes", "size_human", "hash", "path"])
                    for gi, group in enumerate(self.dup_groups, start=1):
                        size_h = human_size(group["size"])
                        for path in group["paths"]:
                            writer.writerow([gi, group["size"], size_h, group["hash"], path])
                self.append_log(f"[UI] Exported duplicates CSV: {file_path}")
            except Exception as exc:  # noqa: BLE001
                QtWidgets.QMessageBox.critical(self, "Export failed", str(exc))
                self.append_log(f"[ERROR] Failed to export CSV: {exc}")

        def on_dup_export_json(self) -> None:
            if not self.dup_groups:
                QtWidgets.QMessageBox.information(self, "Export JSON", "Run a duplicate scan first.")
                return
            file_path, _ = QtWidgets.QFileDialog.getSaveFileName(
                self,
                "Export duplicates JSON",
                "duplicates.json",
                "JSON files (*.json);;All files (*.*)",
            )
            if not file_path:
                return
            payload = {
                "meta": {
                    "tool": "sots_diskstat_gui",
                    "version": __version__,
                    "created_utc": datetime.utcnow().isoformat() + "Z",
                    "root_scanned": self.dup_last_root,
                    "options": self.dup_last_options.to_dict() if self.dup_last_options else {},
                    "stats": self.dup_stats.to_dict() if self.dup_stats else {},
                },
                "groups": self.dup_groups,
            }
            try:
                with open(file_path, "w", encoding="utf-8") as fh:
                    json.dump(payload, fh, indent=2)
                self.append_log(f"[UI] Exported duplicates JSON: {file_path}")
            except Exception as exc:  # noqa: BLE001
                QtWidgets.QMessageBox.critical(self, "Export failed", str(exc))
                self.append_log(f"[ERROR] Failed to export JSON: {exc}")

        def on_open(self) -> None:
            node = self._selected_node()
            if not node:
                return
            self._open_in_explorer(node.path)

        def _open_in_explorer(self, path: str) -> None:
            if not Path(path).exists():
                QtWidgets.QMessageBox.warning(self, "Open", f"Path does not exist:\n{path}")
                return
            try:
                subprocess.Popen(["explorer", path])
                self.append_log(f"[UI] Opened in Explorer: {path}")
            except Exception as exc:  # noqa: BLE001
                self.append_log(f"[ERROR] Failed to open Explorer: {exc}")

        def on_copy(self) -> None:
            node = self._selected_node()
            if not node:
                return
            QtWidgets.QApplication.clipboard().setText(node.path)
            self.append_log(f"[UI] Copied path: {node.path}")

        def on_delete(self) -> None:
            if send2trash is None:
                QtWidgets.QMessageBox.information(self, "Delete", "send2trash is not installed.")
                return
            if not self.enable_delete_cb.isChecked():
                return
            node = self._selected_node()
            if not node:
                return
            reply = QtWidgets.QMessageBox.question(
                self,
                "Confirm delete",
                f"Send to Recycle Bin?\n\n{node.path}",
            )
            if reply != QtWidgets.QMessageBox.Yes:
                return
            try:
                send2trash(node.path)
                self.append_log(f"[UI] Sent to Recycle Bin: {node.path}")
            except Exception as exc:  # noqa: BLE001
                self.append_log(f"[ERROR] Failed to delete: {exc}")
                QtWidgets.QMessageBox.critical(self, "Delete failed", str(exc))

        def on_load_snapshot(self) -> None:
            file_path, _ = QtWidgets.QFileDialog.getOpenFileName(
                self,
                "Load snapshot",
                "",
                "DiskStat snapshot (*.json.gz);;All files (*.*)",
            )
            if not file_path:
                return
            try:
                with gzip.open(file_path, "rt", encoding="utf-8") as fh:
                    payload = json.load(fh)
                meta = payload.get("meta", {})
                node_payload = payload.get("root") or payload.get("node")
                if not node_payload:
                    raise ValueError("Snapshot missing root node data.")
                stats_payload = meta.get("stats") or payload.get("stats")
                node = Node.from_dict(node_payload)
                stats = ScanStats.from_dict(stats_payload or {})
                node.rebuild_parents(None)
                self.root_node = node
                self.stats = stats
                self.path_edit.setText(meta.get("root_path", node.path))
                self._reset_navigation_history(node.path)
                self._populate_tree(push_history=False)
                self._update_details(node)
                self.append_log(f"[UI] Loaded snapshot: {file_path}")
            except Exception as exc:  # noqa: BLE001
                QtWidgets.QMessageBox.critical(self, "Load failed", str(exc))
                self.append_log(f"[ERROR] Failed to load snapshot: {exc}")

        def on_save_snapshot(self) -> None:
            if not self.root_node or not self.stats:
                QtWidgets.QMessageBox.information(self, "Save snapshot", "Run a scan first.")
                return

            file_path, _ = QtWidgets.QFileDialog.getSaveFileName(
                self,
                "Save snapshot",
                "diskstat_snapshot.json.gz",
                "DiskStat snapshot (*.json.gz);;All files (*.*)",
            )
            if not file_path:
                return
            data = {
                "meta": {
                    "tool": "sots_diskstat_gui",
                    "version": __version__,
                    "created_utc": datetime.utcnow().isoformat() + "Z",
                    "root_path": self.path_edit.text(),
                    "stats": self.stats.to_dict(),
                },
                "root": self.root_node.to_dict(),
            }
            try:
                with gzip.open(file_path, "wt", encoding="utf-8") as fh:
                    json.dump(data, fh, indent=2)
                self.append_log(f"[UI] Saved snapshot: {file_path}")
            except Exception as exc:  # noqa: BLE001
                QtWidgets.QMessageBox.critical(self, "Save failed", str(exc))
                self.append_log(f"[ERROR] Failed to save snapshot: {exc}")

        def _update_buttons(self) -> None:
            is_running = self.scan_worker is not None and self.scan_worker.isRunning()
            self.scan_btn.setEnabled(not is_running)
            self.cancel_btn.setEnabled(is_running)
            node = self._selected_node()
            path_exists = node is not None and Path(node.path).exists()
            self.open_btn.setEnabled(node is not None)
            self.copy_btn.setEnabled(node is not None)
            destructive_ready = (
                send2trash is not None and self.enable_delete_cb.isChecked() and node is not None and path_exists
            )
            self.delete_btn.setEnabled(destructive_ready)
            if send2trash is None:
                self.delete_btn.setToolTip("Install send2trash to enable recycle-bin deletes.")
            else:
                self.delete_btn.setToolTip("")
            dup_running = self.dup_worker is not None and self.dup_worker.isRunning()
            self.dup_scan_btn.setEnabled(not dup_running)
            self.dup_cancel_btn.setEnabled(dup_running)
            has_dups = bool(self.dup_groups)
            self.dup_export_csv_btn.setEnabled(has_dups and not dup_running)
            self.dup_export_json_btn.setEnabled(has_dups and not dup_running)
            self._update_history_buttons()


    class _TreemapRectItem(QtWidgets.QGraphicsRectItem):
        def __init__(self, rect: TreemapRect, color: QtGui.QColor, window: DiskStatWindow) -> None:
            super().__init__(rect.x, rect.y, rect.w, rect.h)
            self.setAcceptHoverEvents(True)
            self.setBrush(QtGui.QBrush(color))
            self.base_pen = QtGui.QPen(QtGui.QColor(0, 0, 0, 80), 0.8)
            self.hover_pen = QtGui.QPen(QtGui.QColor(255, 255, 255, 180), 1.2)
            self.setPen(self.base_pen)
            self.rect_data = rect
            self.window = window
            label_text = f"{rect.name}  {human_size(rect.size)}"
            fm = QtGui.QFontMetrics(QtGui.QFont())
            elided = fm.elidedText(label_text, QtCore.Qt.ElideRight, int(rect.w - 6))
            text_color = QtGui.QColor(240, 240, 240) if self._is_dark(color) else QtGui.QColor(20, 20, 20)
            label = QtWidgets.QGraphicsSimpleTextItem(elided)
            label.setBrush(QtGui.QBrush(text_color))
            label.setPos(rect.x + 4, rect.y + 4)
            if rect.w < 40 or rect.h < 24:
                label.setVisible(False)
            label.setToolTip(f"{rect.path}\n{human_size(rect.size)}")
            self.text_item = label

        def mouseDoubleClickEvent(self, event: QtWidgets.QGraphicsSceneMouseEvent) -> None:
            self.window._select_tree_path(self.rect_data.path)
            super().mouseDoubleClickEvent(event)

        def mousePressEvent(self, event: QtWidgets.QGraphicsSceneMouseEvent) -> None:
            if event.button() == QtCore.Qt.RightButton:
                self.window._zoom_up()
            else:
                self.window._select_tree_path(self.rect_data.path)
            super().mousePressEvent(event)

        def hoverEnterEvent(self, event: QtWidgets.QGraphicsSceneHoverEvent) -> None:
            self.setPen(self.hover_pen)
            super().hoverEnterEvent(event)

        def hoverLeaveEvent(self, event: QtWidgets.QGraphicsSceneHoverEvent) -> None:
            self.setPen(self.base_pen)
            super().hoverLeaveEvent(event)

        @staticmethod
        def _is_dark(color: QtGui.QColor) -> bool:
            # Simple luminance check
            return (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()) < 140


def main() -> int:
    if not QT_AVAILABLE:
        print("[ERROR] PySide6 is required. Install with: pip install PySide6")
        if QT_IMPORT_ERROR:
            print(f"Import error: {QT_IMPORT_ERROR}")
        return 1

    app = QtWidgets.QApplication(sys.argv)
    window = DiskStatWindow()
    window.show()
    return app.exec()
