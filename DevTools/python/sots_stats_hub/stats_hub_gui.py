from __future__ import annotations

import os
import sys
import time
import threading
import datetime
import json
import urllib.request
import urllib.error
from pathlib import Path
from typing import Optional, List, Callable

from PySide6 import QtCore, QtGui, QtWidgets

from . import __version__
from .runner import ProcessManager, run_command
from .settings import Settings
from .jobs import JobManager
from .reports_core.report_store import ReportStore
from .reports_core.paths import detect_project_root as core_detect_root, reports_root
from .reports_core.command_catalog import report_commands, generate_all_commands
from .reports_core.models import PluginRow, SearchRow
from .widgets.actions_panel import ActionsPanel
from .widgets.docs_watch_panel import DocsWatchPanel
from .widgets.log_pane import LogPane
from .widgets.zip_panel import ZipPanel
from .widgets.command_palette import CommandPalette
from .widgets.legacy_tab import LegacyTab
from .tool_registry import build_tools, ToolSpec

# Reuse disk scanning primitives without altering the original GUI.
from sots_diskstat_gui.scanner import ScanOptions
from sots_diskstat_gui.model import Node
from sots_diskstat_gui.treemap import TreemapRect, build_treemap
from sots_diskstat_gui.workers import ScanWorker


def _inject_devtools() -> Path:
    cur = Path(__file__).resolve()
    for parent in [cur] + list(cur.parents):
        if (parent / "Plugins").exists() and (parent / "DevTools" / "python").exists():
            dev = parent / "DevTools" / "python"
            if str(dev) not in sys.path:
                sys.path.insert(0, str(dev))
            return dev
    return Path(__file__).resolve().parents[1]


_DEVTOOLS_ROOT = _inject_devtools()


def detect_project_root() -> Path:
    return core_detect_root()


def human_size(num: int) -> str:
    step = 1024.0
    units = ["B", "KB", "MB", "GB", "TB", "PB"]
    size = float(num)
    for unit in units:
        if size < step:
            return f"{size:.1f} {unit}"
        size /= step
    return f"{size:.1f} PB"


def apply_dark_theme(widget: QtWidgets.QWidget) -> None:
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
    QtWidgets.QApplication.instance().setPalette(palette)
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


class DiskTab(QtWidgets.QWidget):
    def __init__(self, log_fn, settings: Settings, project_root: Path, parent: Optional[QtWidgets.QWidget] = None, toggle_log_area=None) -> None:
        super().__init__(parent)
        self.log = log_fn
        self.settings = settings
        self.project_root = project_root
        self._toggle_log_area = toggle_log_area
        self.root_node: Optional[Node] = None
        self._path_map: dict[str, QtWidgets.QTreeWidgetItem] = {}
        self._treemap_node: Optional[Node] = None
        self._treemap_render_timer = QtCore.QTimer(self)
        self._treemap_render_timer.setSingleShot(True)
        self._treemap_render_timer.timeout.connect(self._rerender_treemap)
        self._worker: Optional[ScanWorker] = None
        self._last_stats: Optional[dict] = None
        self._last_progress_log = 0.0
        self._exclusions_list = self._load_exclusions_list()
        self._exclusions_enabled = self._load_bool("scan/exclusions_enabled", True)
        self._exclusions_preset_last = self.settings.get("scan/exclusions_preset_last", "default")
        self._splitter_state = self.settings.get("disk/splitter_state", [])
        self._splitter_top_state = self.settings.get("disk/splitter_top_state", [])
        self._build_ui()
        self._wire()

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        top_row = QtWidgets.QHBoxLayout()
        self.path_edit = QtWidgets.QLineEdit(self.settings.get("last_disk_root") or str(detect_project_root()))
        self.browse_btn = QtWidgets.QPushButton("Browse")
        self.scan_btn = QtWidgets.QPushButton("Scan")
        self.cancel_btn = QtWidgets.QPushButton("Cancel")
        self.cancel_btn.setEnabled(False)
        self.auto_scan_cb = QtWidgets.QCheckBox("Auto-scan on launch")
        self.auto_scan_cb.setChecked(bool(self.settings.get("disk/auto_scan", True)))
        top_row.addWidget(QtWidgets.QLabel("Root:"))
        top_row.addWidget(self.path_edit, stretch=1)
        top_row.addWidget(self.browse_btn)
        top_row.addWidget(self.scan_btn)
        top_row.addWidget(self.cancel_btn)
        top_row.addWidget(self.auto_scan_cb)
        layout.addLayout(top_row)

        presets_row = QtWidgets.QHBoxLayout()
        self.preset_project = QtWidgets.QPushButton("Scan Project Root")
        self.preset_devtools = QtWidgets.QPushButton("Scan DevTools")
        self.preset_plugins = QtWidgets.QPushButton("Scan Plugins")
        self.toggle_excl_btn = QtWidgets.QPushButton("Toggle Exclusions")
        self.toggle_logs_btn = QtWidgets.QPushButton("Toggle Logs")
        presets_row.addWidget(self.preset_project)
        presets_row.addWidget(self.preset_devtools)
        presets_row.addWidget(self.preset_plugins)
        presets_row.addWidget(self.toggle_excl_btn)
        presets_row.addWidget(self.toggle_logs_btn)
        presets_row.addStretch(1)
        layout.addLayout(presets_row)

        opts_row = QtWidgets.QHBoxLayout()
        self.skip_symlinks_cb = QtWidgets.QCheckBox("Skip symlinks")
        self.skip_symlinks_cb.setChecked(True)
        self.depth_spin = QtWidgets.QSpinBox()
        self.depth_spin.setRange(0, 64)
        self.depth_spin.setValue(0)
        opts_row.addWidget(self.skip_symlinks_cb)
        opts_row.addWidget(QtWidgets.QLabel("Max depth:"))
        opts_row.addWidget(self.depth_spin)
        opts_row.addStretch(1)
        layout.addLayout(opts_row)

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
        self.exclusions_edit.setStyleSheet(
            "QPlainTextEdit { background-color: #1f1f1f; color: #e5e5e5; border: 1px solid #444; }"
        )
        exclusions_layout.addWidget(self.exclusions_edit)

        splitter_h = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        splitter_h.setHandleWidth(8)

        self.tree = QtWidgets.QTreeWidget()
        self.tree.setHeaderLabels(["Name", "Size", "Items"])
        self.tree.header().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.tree.header().setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        self.tree.header().setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
        splitter_h.addWidget(self.tree)

        self.treemap_view = QtWidgets.QGraphicsView()
        self.treemap_view.setRenderHint(QtGui.QPainter.Antialiasing)
        self.treemap_scene = QtWidgets.QGraphicsScene()
        self.treemap_view.setScene(self.treemap_scene)
        self.treemap_view.setBackgroundBrush(QtGui.QBrush(QtGui.QColor(25, 25, 25)))
        treemap_container = QtWidgets.QVBoxLayout()
        treemap_widget = QtWidgets.QWidget()
        treemap_widget.setLayout(treemap_container)
        self.breadcrumb_bar = QtWidgets.QHBoxLayout()
        self.breadcrumb_bar.setContentsMargins(0, 0, 0, 0)
        treemap_container.addLayout(self.breadcrumb_bar)
        treemap_container.addWidget(self.treemap_view)
        splitter_h.addWidget(treemap_widget)
        splitter_h.setStretchFactor(1, 2)


        status_row = QtWidgets.QHBoxLayout()
        self.status_lbl = QtWidgets.QLabel("Idle")
        self.progress = QtWidgets.QProgressBar()
        self.progress.setRange(0, 0)
        self.progress.setVisible(False)
        status_row.addWidget(self.status_lbl)
        status_row.addWidget(self.progress)

        top_splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        top_splitter.setHandleWidth(8)
        top_splitter.addWidget(splitter_h)
        excl_container = QtWidgets.QWidget()
        excl_layout = QtWidgets.QVBoxLayout(excl_container)
        excl_layout.setContentsMargins(0, 0, 0, 0)
        self.exclusions_group = QtWidgets.QGroupBox("Exclusions")
        self.exclusions_group.setCheckable(True)
        self.exclusions_group.setChecked(True)
        excl_group_layout = QtWidgets.QVBoxLayout(self.exclusions_group)
        excl_group_layout.addWidget(exclusions_widget)
        excl_layout.addWidget(self.exclusions_group)
        excl_layout.addLayout(status_row)
        top_splitter.addWidget(excl_container)
        top_splitter.setStretchFactor(0, 3)
        top_splitter.setStretchFactor(1, 1)

        v_splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        v_splitter.setHandleWidth(10)
        # Only top content lives here; global log remains below main tabs
        v_splitter.addWidget(top_splitter)
        v_splitter.addWidget(QtWidgets.QWidget())
        v_splitter.setStretchFactor(0, 1)
        v_splitter.setStretchFactor(1, 0)
        self.splitter_main = v_splitter
        self.splitter_top = top_splitter

        if isinstance(self._splitter_state, list) and self._splitter_state:
            try:
                v_splitter.setSizes([int(x) for x in self._splitter_state])
            except Exception:
                pass
        if isinstance(self._splitter_top_state, list) and self._splitter_top_state:
            try:
                top_splitter.setSizes([int(x) for x in self._splitter_top_state])
            except Exception:
                pass

        layout.addWidget(v_splitter, stretch=1)

        # Track current visibility for toggles
        self._logs_hidden = False
        self._excl_hidden = False

    def _wire(self) -> None:
        self.browse_btn.clicked.connect(self._on_browse)
        self.scan_btn.clicked.connect(self._on_scan)
        self.cancel_btn.clicked.connect(self._on_cancel)
        self.tree.itemSelectionChanged.connect(self._on_selection)
        self.treemap_scene.selectionChanged.connect(lambda: None)
        self.auto_scan_cb.toggled.connect(self._persist_auto_scan)
        self.preset_project.clicked.connect(lambda: self._quick_scan(self.project_root))
        self.preset_devtools.clicked.connect(lambda: self._quick_scan(self.project_root / "DevTools"))
        self.preset_plugins.clicked.connect(lambda: self._quick_scan(self.project_root / "Plugins"))
        self.toggle_excl_btn.clicked.connect(self._toggle_exclusions_panel)
        self.toggle_logs_btn.clicked.connect(self._toggle_logs_panel)
        self.exclusions_enable_cb.toggled.connect(self._on_exclusions_enabled_changed)
        self.exclusions_preset_windows_btn.clicked.connect(lambda: self._apply_exclusions_preset("windows"))
        self.exclusions_preset_unreal_btn.clicked.connect(lambda: self._apply_exclusions_preset("unreal"))
        self.exclusions_reset_btn.clicked.connect(lambda: self._apply_exclusions_preset("default"))
        self.exclusions_edit.textChanged.connect(self._on_exclusions_changed)
        QtCore.QTimer.singleShot(200, self._maybe_auto_scan)

    def _on_browse(self) -> None:
        dir_path = QtWidgets.QFileDialog.getExistingDirectory(self, "Select root", self.path_edit.text())
        if dir_path:
            self.path_edit.setText(dir_path)

    def _on_scan(self) -> None:
        root = self.path_edit.text().strip()
        if not root:
            return
        self.settings.set("last_disk_root", root)
        opts = ScanOptions(skip_symlinks=self.skip_symlinks_cb.isChecked(), max_depth=self.depth_spin.value())
        opts.exclusions = self._current_exclusions()
        if self._worker and self._worker.isRunning():
            return
        self.scan_btn.setEnabled(False)
        self.cancel_btn.setEnabled(True)
        self.progress.setVisible(True)
        self.status_lbl.setText("Scanning…")
        self.log(f"[DISK] scanning {root}")

        self._worker = ScanWorker(root, opts, self)
        self._worker.progress.connect(self._on_progress)
        self._worker.finished.connect(self._on_scan_finished)
        self._worker.failed.connect(self._on_scan_failed)
        self._worker.cancelled.connect(self._on_scan_cancelled)
        self._worker.start()

    def _quick_scan(self, root: Path) -> None:
        self.path_edit.setText(str(root))
        self._on_scan()

    @QtCore.Slot(str)
    def _on_scan_failed(self, err: str) -> None:
        self.scan_btn.setEnabled(True)
        self.cancel_btn.setEnabled(False)
        self.progress.setVisible(False)
        QtWidgets.QMessageBox.critical(self, "Scan failed", err)
        self._log(f"[DISK] scan failed: {err}")

    @QtCore.Slot(object, object)
    def _on_scan_finished(self, node: Node, stats) -> None:
        self._worker = None
        self.root_node = node
        self._populate_tree()
        self._update_details(node)
        self.scan_btn.setEnabled(True)
        self.cancel_btn.setEnabled(False)
        self.progress.setVisible(False)
        stamp = time.strftime("%H:%M:%S")
        self._last_stats = {"dirs": stats.total_dirs, "files": stats.total_files, "errors": stats.total_errors, "ts": stamp}
        self.status_lbl.setText(f"Last scan {stamp} | dirs {stats.total_dirs} files {stats.total_files} errors {stats.total_errors}")
        self._log(f"[DISK] done dirs={stats.total_dirs} files={stats.total_files} errors={stats.total_errors}")

    @QtCore.Slot(str)
    def _on_progress(self, msg: str) -> None:
        self.status_lbl.setText(msg.replace("[PROGRESS] ", "").strip() or "Scanning…")
        now = time.time()
        if now - self._last_progress_log > 0.5:
            self._log(msg)
            self._last_progress_log = now

    @QtCore.Slot()
    def _on_scan_cancelled(self) -> None:
        self._worker = None
        self.scan_btn.setEnabled(True)
        self.cancel_btn.setEnabled(False)
        self.progress.setVisible(False)
        self.status_lbl.setText("Scan cancelled")
        self._log("[DISK] scan cancelled")

    def _toggle_exclusions_panel(self) -> None:
        self._excl_hidden = not getattr(self, "_excl_hidden", False)
        self.exclusions_group.setVisible(not self._excl_hidden)

    def _toggle_logs_panel(self) -> None:
        self._logs_hidden = not getattr(self, "_logs_hidden", False)
        if callable(self._toggle_log_area):
            self._toggle_log_area(not self._logs_hidden)

    def _populate_tree(self) -> None:
        self.tree.clear()
        self._path_map.clear()
        if not self.root_node:
            return

        def add_item(node: Node, parent: Optional[QtWidgets.QTreeWidgetItem]) -> None:
            item = QtWidgets.QTreeWidgetItem([node.name, human_size(node.size), str(len(node.children))])
            item.setData(0, QtCore.Qt.UserRole, node)
            self._path_map[str(Path(node.path).resolve())] = item
            if parent is None:
                self.tree.addTopLevelItem(item)
            else:
                parent.addChild(item)
            for child in node.children:
                add_item(child, item)

        add_item(self.root_node, None)
        if self.tree.topLevelItemCount():
            self.tree.setCurrentItem(self.tree.topLevelItem(0))

    def _selected_node(self) -> Optional[Node]:
        items = self.tree.selectedItems()
        if not items:
            return None
        return items[0].data(0, QtCore.Qt.UserRole)

    def _on_selection(self) -> None:
        node = self._selected_node()
        if node:
            self._update_details(node)
        elif self.root_node:
            self.status_lbl.setText("Idle")

    def _update_details(self, node: Node) -> None:
        self._populate_treemap(node)
        self._refresh_breadcrumbs(node)

    def _on_cancel(self) -> None:
        if self._worker and self._worker.isRunning():
            self._worker.cancel()
        self.cancel_btn.setEnabled(False)
        self.status_lbl.setText("Cancelling…")

    def _populate_treemap(self, node: Node) -> None:
        self.treemap_scene.clear()
        self._treemap_node = node
        items = [(child.name, child.size, child.path) for child in node.children if child.size > 0]
        if not items and node.top_files:
            items = [(f.name, f.size, f.path) for f in node.top_files]
        view_size = self.treemap_view.viewport().size()
        rects: list[TreemapRect] = build_treemap(items, max(view_size.width(), 10), max(view_size.height(), 10), padding=2.0, gutter=4.0)
        for rect in rects:
            color = QtGui.QColor.fromHsv(hash(rect.path) % 360, 180, 220, 180)
            item = _TreemapRectItem(rect, color, self)
            self.treemap_scene.addItem(item)
            self.treemap_scene.addItem(item.text_item)
        self.treemap_scene.setSceneRect(self.treemap_scene.itemsBoundingRect())
        self.treemap_view.fitInView(self.treemap_scene.sceneRect(), QtCore.Qt.KeepAspectRatio)

    def _rerender_treemap(self) -> None:
        if self._treemap_node:
            self._populate_treemap(self._treemap_node)

    def _refresh_breadcrumbs(self, node: Optional[Node]) -> None:
        while self.breadcrumb_bar.count():
            child = self.breadcrumb_bar.takeAt(0)
            if child.widget():
                child.widget().deleteLater()
        if not node:
            return
        chain = []
        cur = node
        while cur:
            chain.append(cur)
            cur = cur.parent
        chain.reverse()
        for idx, crumb in enumerate(chain):
            btn = QtWidgets.QToolButton()
            btn.setText(crumb.name or crumb.path)
            btn.setAutoRaise(True)
            btn.clicked.connect(lambda _=False, p=crumb.path: self._select_tree_path(p))
            self.breadcrumb_bar.addWidget(btn)
            if idx < len(chain) - 1:
                self.breadcrumb_bar.addWidget(QtWidgets.QLabel(">"))
        self.breadcrumb_bar.addStretch(1)

    def _select_tree_path(self, path: str) -> None:
        key = str(Path(path).resolve())
        item = self._path_map.get(key)
        if item:
            self.tree.setCurrentItem(item)
            self.tree.scrollToItem(item)

    def _zoom_up(self) -> None:
        if self._treemap_node and self._treemap_node.parent:
            self._select_tree_path(self._treemap_node.parent.path)

    def _persist_auto_scan(self, checked: bool) -> None:
        self.settings.set("disk/auto_scan", bool(checked))

    def _parse_exclusions_text(self) -> list[str]:
        return [line.strip() for line in self.exclusions_edit.toPlainText().splitlines() if line.strip()]

    def _current_exclusions(self) -> list[str]:
        if not self.exclusions_enable_cb.isChecked():
            return []
        return [pat.lower() for pat in self._parse_exclusions_text()]

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
            raw = self.settings.get("scan/exclusions_list", [])
        except Exception:
            raw = []
        if isinstance(raw, str):
            raw_list = [line.strip() for line in raw.splitlines() if line.strip()]
        else:
            raw_list = [str(x).strip() for x in (raw or []) if str(x).strip()]
        return raw_list or self._default_exclusions()

    def _load_bool(self, key: str, default: bool) -> bool:
        try:
            val = self.settings.get(key, default)
        except Exception:
            return default
        if isinstance(val, str):
            return val.lower() in {"1", "true", "yes", "on"}
        try:
            return bool(val)
        except Exception:
            return default

    def _apply_exclusion_ui_state(self) -> None:
        enabled = self.exclusions_enable_cb.isChecked()
        self.exclusions_edit.setReadOnly(not enabled)

    def _save_exclusions_settings(self) -> None:
        self._exclusions_list = self._parse_exclusions_text()
        self.settings.set("scan/exclusions_enabled", self.exclusions_enable_cb.isChecked())
        self.settings.set("scan/exclusions_list", self._exclusions_list)
        self.settings.set("scan/exclusions_preset_last", self._exclusions_preset_last or "")

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

    def _maybe_auto_scan(self) -> None:
        if not bool(self.settings.get("disk/auto_scan", True)):
            return
        root = self.settings.get("last_disk_root") or str(self.project_root / "DevTools")
        self.path_edit.setText(str(root))
        if self.depth_spin.value() == 0:
            try:
                depth = int(self.settings.get("disk/auto_scan_depth", 6))
            except Exception:
                depth = 6
            self.depth_spin.setValue(depth)
        self._on_scan()

    def resizeEvent(self, event: QtGui.QResizeEvent) -> None:  # type: ignore[override]
        super().resizeEvent(event)
        self._treemap_render_timer.start(50)


class _TreemapRectItem(QtWidgets.QGraphicsRectItem):
    def __init__(self, rect: TreemapRect, color: QtGui.QColor, window: DiskTab) -> None:
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
        return (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()) < 140


    def _log(self, msg: str) -> None:
        ts = time.strftime("%H:%M:%S")
        line = f"[{ts}] {msg}"
        self.log(line)
        self._apply_local_log_filter(line)

    def _apply_local_log_filter(self, line: str | None = None) -> None:
        return

    def save_splitters(self) -> None:
        if hasattr(self, "splitter_main"):
            try:
                self.settings.set("disk/splitter_state", [int(x) for x in self.splitter_main.sizes()])
            except Exception:
                pass
        if hasattr(self, "splitter_top"):
            try:
                self.settings.set("disk/splitter_top_state", [int(x) for x in self.splitter_top.sizes()])
            except Exception:
                pass


class ReportsTab(QtWidgets.QWidget):
    def __init__(
        self,
        project_root: Path,
        store: ReportStore,
        pm: ProcessManager,
        log_fn,
        on_refresh,
        on_run_record,
        enqueue_job,
        settings: Settings,
        run_tool=None,
        tool_lookup=None,
        parent=None,
    ) -> None:
        super().__init__(parent)
        self.project_root = project_root
        self.store = store
        self.pm = pm
        self.log = log_fn
        self.on_refresh = on_refresh
        self.on_run_record = on_run_record
        self.enqueue_job = enqueue_job
        self.settings = settings
        self.run_tool = run_tool
        self.tool_lookup = tool_lookup or {}
        self._build_ui()
        self._sync_status()

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        btn_row = QtWidgets.QHBoxLayout()
        self.dep_btn = QtWidgets.QPushButton("Generate DepMap")
        self.todo_btn = QtWidgets.QPushButton("Generate TODO Backlog")
        self.tag_btn = QtWidgets.QPushButton("Generate Tag Usage")
        self.api_btn = QtWidgets.QPushButton("Generate API Surface")
        self.all_btn = QtWidgets.QPushButton("Generate ALL")
        self.smart_btn = QtWidgets.QPushButton("Generate Smart")
        self.smart_toggle = QtWidgets.QCheckBox("Smart enabled")
        self.smart_toggle.setChecked(bool(self.settings.get("reports/smart_enabled", True)))
        self.smart_spin = QtWidgets.QSpinBox()
        self.smart_spin.setRange(1, 180)
        self.smart_spin.setValue(int(self.settings.get("reports/stale_minutes", 30)))
        for b in [self.dep_btn, self.todo_btn, self.tag_btn, self.api_btn, self.all_btn, self.smart_btn]:
            btn_row.addWidget(b)
        btn_row.addStretch(1)
        layout.addLayout(btn_row)
        smart_row = QtWidgets.QHBoxLayout()
        smart_row.addWidget(self.smart_toggle)
        smart_row.addWidget(QtWidgets.QLabel("Stale minutes:"))
        smart_row.addWidget(self.smart_spin)
        smart_row.addStretch(1)
        layout.addLayout(smart_row)

        self.status_layout = QtWidgets.QVBoxLayout()
        layout.addLayout(self.status_layout)
        layout.addStretch(1)

        self.dep_btn.clicked.connect(lambda: self._run_single("DepMap"))
        self.todo_btn.clicked.connect(lambda: self._run_single("TODO Backlog"))
        self.tag_btn.clicked.connect(lambda: self._run_single("Tag Usage"))
        self.api_btn.clicked.connect(lambda: self._run_single("API Surface"))
        self.all_btn.clicked.connect(self._run_all)
        self.smart_btn.clicked.connect(self.run_smart)
        self.smart_toggle.toggled.connect(self._persist_smart)
        self.smart_spin.valueChanged.connect(self._persist_smart)

    def _clear_status(self) -> None:
        while self.status_layout.count():
            item = self.status_layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

    def _status_card(self, name: str, path: Path, err: str | None) -> QtWidgets.QGroupBox:
        box = QtWidgets.QGroupBox(name)
        v = QtWidgets.QVBoxLayout(box)
        v.addWidget(QtWidgets.QLabel(f"Path: {path}"))
        mtime = "-"
        if path.exists():
            try:
                mtime = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(path.stat().st_mtime))
            except Exception:
                pass
        v.addWidget(QtWidgets.QLabel(f"Last Modified: {mtime}"))
        status = "OK" if err is None else err
        v.addWidget(QtWidgets.QLabel(f"Status: {status}"))
        btn_row = QtWidgets.QHBoxLayout()
        open_btn = QtWidgets.QPushButton("Open file")
        folder_btn = QtWidgets.QPushButton("Open folder")
        copy_btn = QtWidgets.QPushButton("Copy path")
        btn_row.addWidget(open_btn)
        btn_row.addWidget(folder_btn)
        btn_row.addWidget(copy_btn)
        btn_row.addStretch(1)
        v.addLayout(btn_row)
        open_btn.clicked.connect(lambda: self._open_path(path))
        folder_btn.clicked.connect(lambda: self._open_path(path.parent))
        copy_btn.clicked.connect(lambda: QtWidgets.QApplication.clipboard().setText(str(path)))
        return box

    def _open_path(self, path: Path) -> None:
        try:
            import subprocess

            subprocess.Popen(["explorer", str(path)])
        except Exception:
            self.log(f"[WARN] Could not open: {path}")

    def _sync_status(self) -> None:
        self._clear_status()
        for name, path in self.store.paths.items():
            display = {
                "depmap": "DepMap JSON",
                "todo": "TODO Backlog",
                "tags": "Tag Usage",
                "api": "API Surface",
            }.get(name, name)
            err = self.store.errors.get(name)
            card = self._status_card(display, path, err)
            self.status_layout.addWidget(card)

    def _run_single(self, label: str) -> None:
        tool_id = {
            "DepMap": "depmap",
            "TODO Backlog": "todo",
            "Tag Usage": "tags",
            "API Surface": "api",
        }.get(label)
        if tool_id and self.run_tool and tool_id in self.tool_lookup:
            self.run_tool(self.tool_lookup[tool_id])
            return
        for n, args in report_commands(self.project_root):
            if n == label:
                self._start_process(n, args)
                break

    def _run_all(self) -> None:
        self.run_all()

    def run_all(self) -> None:
        specs = [self.tool_lookup.get(tid) for tid in ["depmap", "todo", "tags", "api"]]
        if self.run_tool and all(specs):
            for spec in specs:
                self.run_tool(spec)
            return
        cmds = generate_all_commands(self.project_root)
        for cmd in cmds:
            self._start_process(" ".join(cmd), cmd)

    def _start_process(self, name: str, args: list[str]) -> None:
        def finish(log_fn) -> int:
            code = run_command(args, log_fn, cwd=self.project_root)
            self.on_run_record(args[1] if len(args) > 1 else name, code)
            QtCore.QMetaObject.invokeMethod(self, "refresh_view", QtCore.Qt.QueuedConnection)
            self.on_refresh()
            return code

        self.enqueue_job(name, func=finish)

    def refresh_view(self) -> None:
        self._sync_status()

    def _persist_smart(self) -> None:
        self.settings.set("reports/smart_enabled", self.smart_toggle.isChecked())
        self.settings.set("reports/stale_minutes", self.smart_spin.value())

    def run_smart(self) -> None:
        if not self.smart_toggle.isChecked():
            self.run_all()
            return
        stale_minutes = int(self.smart_spin.value())
        now = time.time()
        cmd_map = {lbl: args for lbl, args in report_commands(self.project_root)}
        tool_map = {
            "depmap": ("DepMap", self.tool_lookup.get("depmap")),
            "todo": ("TODO Backlog", self.tool_lookup.get("todo")),
            "tags": ("Tag Usage", self.tool_lookup.get("tags")),
            "api": ("API Surface", self.tool_lookup.get("api")),
        }
        for name, path in self.store.paths.items():
            try:
                mtime = path.stat().st_mtime
            except Exception:
                mtime = 0.0
            stale = (now - mtime) > stale_minutes * 60 or not path.exists()
            if stale:
                key, spec = tool_map.get(name, ("", None))
                if spec and self.run_tool:
                    self.run_tool(spec)
                    continue
                args = cmd_map.get(key)
                if args:
                    self._start_process(key, args)


class PluginsTab(QtWidgets.QWidget):
    def __init__(self, store: ReportStore, log_fn, on_select_plugin, parent=None) -> None:
        super().__init__(parent)
        self.store = store
        self.log = log_fn
        self.on_select_plugin = on_select_plugin
        self._build_ui()
        self.refresh()

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        filter_row = QtWidgets.QHBoxLayout()
        self.filter_edit = QtWidgets.QLineEdit()
        self.filter_edit.setPlaceholderText("Filter plugins...")
        self.sots_only_cb = QtWidgets.QCheckBox("SOTS_* only")
        filter_row.addWidget(self.filter_edit)
        filter_row.addWidget(self.sots_only_cb)
        layout.addLayout(filter_row)

        splitter = QtWidgets.QSplitter()
        splitter.setOrientation(QtCore.Qt.Horizontal)

        self.table = QtWidgets.QTableWidget(0, 6)
        self.table.setHorizontalHeaderLabels(["Pin", "Plugin", "Inbound", "Outbound", "TODO", "Tags"])
        self.table.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        splitter.addWidget(self.table)

        self.detail_tabs = QtWidgets.QTabWidget()
        self.inbound_list = QtWidgets.QListWidget()
        self.outbound_list = QtWidgets.QListWidget()
        self.todo_table = QtWidgets.QTableWidget(0, 3)
        self.todo_table.setHorizontalHeaderLabels(["File", "Line", "Text"])
        self.todo_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.todo_table.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        self.tags_table = QtWidgets.QTableWidget(0, 3)
        self.tags_table.setHorizontalHeaderLabels(["File", "Line", "Text"])
        self.tags_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.tags_table.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        self.api_table = QtWidgets.QTableWidget(0, 3)
        self.api_table.setHorizontalHeaderLabels(["Kind", "Symbol", "File"])
        self.api_table.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        self.detail_tabs.addTab(self._wrap_widget(self.inbound_list, "Inbound"), "Inbound")
        self.detail_tabs.addTab(self._wrap_widget(self.outbound_list, "Outbound"), "Outbound")
        self.detail_tabs.addTab(self.todo_table, "TODO")
        self.detail_tabs.addTab(self.tags_table, "Tags")
        self.detail_tabs.addTab(self.api_table, "API")
        splitter.addWidget(self.detail_tabs)
        splitter.setStretchFactor(1, 2)

        layout.addWidget(splitter)

        self.filter_edit.textChanged.connect(self.refresh)
        self.sots_only_cb.toggled.connect(self.refresh)
        self.table.cellChanged.connect(self._on_cell_changed)
        self.table.itemSelectionChanged.connect(self._on_select)

    def _wrap_widget(self, widget: QtWidgets.QWidget, title: str) -> QtWidgets.QWidget:
        box = QtWidgets.QWidget()
        v = QtWidgets.QVBoxLayout(box)
        v.addWidget(QtWidgets.QLabel(title))
        v.addWidget(widget)
        return box

    def _filtered_rows(self) -> List[PluginRow]:
        rows = self.store.plugin_rows
        filt = self.filter_edit.text().lower().strip()
        sots_only = self.sots_only_cb.isChecked()
        out = []
        for r in rows:
            if sots_only and not r.name.lower().startswith("sots_"):
                continue
            if filt and filt not in r.name.lower():
                continue
            out.append(r)
        return out

    def refresh(self) -> None:
        rows = self._filtered_rows()
        self.table.blockSignals(True)
        self.table.setRowCount(len(rows))
        for i, r in enumerate(rows):
            pin_item = QtWidgets.QTableWidgetItem()
            pin_item.setCheckState(QtCore.Qt.Checked if r.pinned else QtCore.Qt.Unchecked)
            self.table.setItem(i, 0, pin_item)
            self.table.setItem(i, 1, QtWidgets.QTableWidgetItem(r.name))
            self.table.setItem(i, 2, QtWidgets.QTableWidgetItem(str(r.inbound)))
            self.table.setItem(i, 3, QtWidgets.QTableWidgetItem(str(r.outbound)))
            self.table.setItem(i, 4, QtWidgets.QTableWidgetItem(str(r.todo)))
            self.table.setItem(i, 5, QtWidgets.QTableWidgetItem(str(r.tags)))
        self.table.blockSignals(False)

    def _on_cell_changed(self, row: int, col: int) -> None:
        if col != 0:
            return
        name_item = self.table.item(row, 1)
        pin_item = self.table.item(row, 0)
        if not name_item or not pin_item:
            return
        name = name_item.text()
        pinned = pin_item.checkState() == QtCore.Qt.Checked
        if pinned and name not in self.store.pins:
            self.store.pins.append(name)
            self.store.save_pins()
        if not pinned and name in self.store.pins:
            self.store.pins.remove(name)
            self.store.save_pins()
        self.store.refresh(force=True)
        self.refresh()

    def _on_select(self) -> None:
        items = self.table.selectedItems()
        if not items:
            return
        name = items[1].text()
        detail = self.store.plugin_detail(name)
        self._fill_list(self.inbound_list, detail.get("inbound", []))
        self._fill_list(self.outbound_list, detail.get("outbound", []))
        self._fill_table(self.todo_table, detail.get("todo", {}))
        self._fill_table(self.tags_table, detail.get("tags", {}))
        self._fill_api(self.api_table, detail.get("api", {}))
        self.on_select_plugin(name)

    def _fill_list(self, widget: QtWidgets.QListWidget, items: List[str]) -> None:
        widget.clear()
        widget.addItems(items or [])

    def _fill_table(self, table: QtWidgets.QTableWidget, data: dict) -> None:
        rows = []
        for file, matches in (data or {}).items():
            for m in matches:
                rows.append((file, str(m.get("line", "")), str(m.get("text", ""))))
        table.setRowCount(len(rows))
        for i, (f, line, text) in enumerate(rows):
            table.setItem(i, 0, QtWidgets.QTableWidgetItem(f))
            table.setItem(i, 1, QtWidgets.QTableWidgetItem(line))
            table.setItem(i, 2, QtWidgets.QTableWidgetItem(text))

    def _fill_api(self, table: QtWidgets.QTableWidget, data: dict) -> None:
        rows = []
        for file, entry in (data or {}).items():
            for t in entry.get("types", []):
                rows.append((t.get("kind", ""), t.get("name", ""), file))
            for f in entry.get("functions", []):
                rows.append(("Function", f.get("name", ""), file))
            for p in entry.get("properties", []):
                rows.append(("Property", p.get("declaration", ""), file))
        table.setRowCount(len(rows))
        for i, (k, sym, file) in enumerate(rows):
            table.setItem(i, 0, QtWidgets.QTableWidgetItem(k))
            table.setItem(i, 1, QtWidgets.QTableWidgetItem(sym))
            table.setItem(i, 2, QtWidgets.QTableWidgetItem(file))

    def select_plugin(self, name: str) -> None:
        for row in range(self.table.rowCount()):
            item = self.table.item(row, 1)
            if item and item.text() == name:
                self.table.setCurrentCell(row, 1)
                self._on_select()
                break


class SearchTab(QtWidgets.QWidget):
    def __init__(self, store: ReportStore, project_root: Path, log_fn, parent=None) -> None:
        super().__init__(parent)
        self.store = store
        self.project_root = project_root
        self.log = log_fn
        self._build_ui()
        self.refresh()

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        self.search_edit = QtWidgets.QLineEdit()
        self.search_edit.setPlaceholderText("Search TODO/Tags/API...")
        layout.addWidget(self.search_edit)
        self.table = QtWidgets.QTableWidget(0, 4)
        self.table.setHorizontalHeaderLabels(["Type", "Plugin", "File", "Preview"])
        self.table.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        self.table.horizontalHeader().setSectionResizeMode(3, QtWidgets.QHeaderView.Stretch)
        layout.addWidget(self.table)
        self.search_edit.textChanged.connect(self.refresh)
        self.table.doubleClicked.connect(self._open_row)

    def refresh(self) -> None:
        term = self.search_edit.text().lower().strip()
        rows: List[SearchRow] = []
        for r in self.store.search_rows:
            if term and term not in r.preview.lower() and term not in r.plugin.lower() and term not in r.file.lower():
                continue
            rows.append(r)
        self.table.setRowCount(len(rows))
        for i, r in enumerate(rows):
            self.table.setItem(i, 0, QtWidgets.QTableWidgetItem(r.type))
            self.table.setItem(i, 1, QtWidgets.QTableWidgetItem(r.plugin))
            self.table.setItem(i, 2, QtWidgets.QTableWidgetItem(r.file))
            self.table.setItem(i, 3, QtWidgets.QTableWidgetItem(r.preview))

    def _open_row(self) -> None:
        items = self.table.selectedItems()
        if not items:
            return
        file_rel = items[2].text()
        abs_path = (self.project_root / file_rel).resolve()
        try:
            import subprocess

            subprocess.Popen(["explorer", "/select,", str(abs_path)])
            self.log(f"[SEARCH] opened {abs_path}")
        except Exception:
            self.log(f"[WARN] Could not open {abs_path}")


class HotspotsTab(QtWidgets.QWidget):
    def __init__(self, store: ReportStore, parent=None) -> None:
        super().__init__(parent)
        self.store = store
        self._build_ui()
        self.refresh()

    def _build_ui(self) -> None:
        layout = QtWidgets.QHBoxLayout(self)
        self.plugins_table = QtWidgets.QTableWidget(0, 2)
        self.plugins_table.setHorizontalHeaderLabels(["Plugin", "Score"])
        self.plugins_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.files_table = QtWidgets.QTableWidget(0, 2)
        self.files_table.setHorizontalHeaderLabels(["File", "Score"])
        self.files_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        layout.addWidget(self.plugins_table)
        layout.addWidget(self.files_table)

    def refresh(self) -> None:
        self.plugins_table.setRowCount(len(self.store.hot_plugins))
        for i, (name, score) in enumerate(self.store.hot_plugins):
            self.plugins_table.setItem(i, 0, QtWidgets.QTableWidgetItem(name))
            self.plugins_table.setItem(i, 1, QtWidgets.QTableWidgetItem(str(score)))

        self.files_table.setRowCount(len(self.store.hot_files))
        for i, (name, score) in enumerate(self.store.hot_files):
            self.files_table.setItem(i, 0, QtWidgets.QTableWidgetItem(name))
            self.files_table.setItem(i, 1, QtWidgets.QTableWidgetItem(str(score)))


class GraphTab(QtWidgets.QWidget):
    def __init__(self, store: ReportStore, parent=None) -> None:
        super().__init__(parent)
        self.store = store
        self._build_ui()
        self.refresh()

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        filter_row = QtWidgets.QHBoxLayout()
        self.filter_edit = QtWidgets.QLineEdit()
        self.filter_edit.setPlaceholderText("Filter plugin...")
        self.hide_isolated_cb = QtWidgets.QCheckBox("Hide isolated")
        filter_row.addWidget(self.filter_edit)
        filter_row.addWidget(self.hide_isolated_cb)
        filter_row.addStretch(1)
        layout.addLayout(filter_row)
        self.edges_table = QtWidgets.QTableWidget(0, 2)
        self.edges_table.setHorizontalHeaderLabels(["Plugin", "Depends On"])
        self.edges_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.edges_table.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        layout.addWidget(self.edges_table)
        self.filter_edit.textChanged.connect(self.refresh)
        self.hide_isolated_cb.toggled.connect(self.refresh)

    def refresh(self) -> None:
        depmap = self.store.depmap or {}
        rows = []
        filt = self.filter_edit.text().lower().strip()
        hide_iso = self.hide_isolated_cb.isChecked()
        for plugin, info in depmap.items():
            deps = info.get("Dependencies") or []
            if hide_iso and not deps:
                continue
            if filt and filt not in plugin.lower():
                continue
            if deps:
                for d in deps:
                    rows.append((plugin, d))
            else:
                rows.append((plugin, "(none)"))
        self.edges_table.setRowCount(len(rows))
        for i, (p, d) in enumerate(rows):
            self.edges_table.setItem(i, 0, QtWidgets.QTableWidgetItem(p))
            self.edges_table.setItem(i, 1, QtWidgets.QTableWidgetItem(d))


class RunsTab(QtWidgets.QWidget):
    def __init__(self, runs_ref: dict, project_root: Path, parent=None) -> None:
        super().__init__(parent)
        self.runs_ref = runs_ref
        self.project_root = project_root
        self._build_ui()
        self.refresh()

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        self.table = QtWidgets.QTableWidget(0, 3)
        self.table.setHorizontalHeaderLabels(["Command", "Last Exit", "When"])
        self.table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        layout.addWidget(self.table)

        cmds_box = QtWidgets.QGroupBox("Copy commands")
        cmds_layout = QtWidgets.QHBoxLayout(cmds_box)
        for name, args in report_commands(self.project_root):
            btn = QtWidgets.QPushButton(name)
            btn.clicked.connect(lambda _=False, a=args: QtWidgets.QApplication.clipboard().setText(" ".join(a)))
            cmds_layout.addWidget(btn)
        cmds_layout.addStretch(1)
        layout.addWidget(cmds_box)
        layout.addStretch(1)

    def refresh(self) -> None:
        items = list(self.runs_ref.items())
        self.table.setRowCount(len(items))
        for i, (cmd, info) in enumerate(items):
            self.table.setItem(i, 0, QtWidgets.QTableWidgetItem(cmd))
            self.table.setItem(i, 1, QtWidgets.QTableWidgetItem(str(info.get("code"))))
            self.table.setItem(i, 2, QtWidgets.QTableWidgetItem(info.get("when", "")))


class JobsTab(QtWidgets.QWidget):
    def __init__(self, jm: JobManager, parent=None) -> None:
        super().__init__(parent)
        self.jm = jm
        self._build_ui()
        self.jm.jobs_changed.connect(self.refresh)
        self.refresh()

    def _build_ui(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        top = QtWidgets.QHBoxLayout()
        self.active_lbl = QtWidgets.QLabel("Active: none")
        self.cancel_btn = QtWidgets.QPushButton("Cancel Current")
        self.clear_btn = QtWidgets.QPushButton("Clear Completed")
        top.addWidget(self.active_lbl)
        top.addStretch(1)
        top.addWidget(self.cancel_btn)
        top.addWidget(self.clear_btn)
        layout.addLayout(top)
        self.table = QtWidgets.QTableWidget(0, 6)
        self.table.setHorizontalHeaderLabels(["ID", "Label", "State", "Exit", "Queued", "Log"])
        self.table.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        self.table.horizontalHeader().setSectionResizeMode(5, QtWidgets.QHeaderView.Stretch)
        layout.addWidget(self.table)
        self.cancel_btn.clicked.connect(self.jm.cancel_active)
        self.clear_btn.clicked.connect(self._on_clear)

    def _on_clear(self) -> None:
        self.jm.clear_history()
        self.refresh()

    def refresh(self) -> None:
        active = self.jm.active
        if active:
            self.active_lbl.setText(f"Active: {active.label} (PID {active.pid or '-'})")
        else:
            self.active_lbl.setText("Active: none")
        rows = list(self.jm.queued) + list(self.jm.history[-20:])
        self.table.setRowCount(len(rows))
        for i, job in enumerate(rows):
            self.table.setItem(i, 0, QtWidgets.QTableWidgetItem(job.id))
            self.table.setItem(i, 1, QtWidgets.QTableWidgetItem(job.label))
            self.table.setItem(i, 2, QtWidgets.QTableWidgetItem(job.state))
            self.table.setItem(i, 3, QtWidgets.QTableWidgetItem(str(job.exit_code if job.exit_code is not None else "")))
            queued_at = time.strftime("%H:%M:%S", time.localtime(job.created_ts))
            self.table.setItem(i, 4, QtWidgets.QTableWidgetItem(queued_at))
            self.table.setItem(i, 5, QtWidgets.QTableWidgetItem(str(job.log_path or "")))


class StatsHubWindow(QtWidgets.QMainWindow):
    def __init__(self, settings: Settings) -> None:
        super().__init__()
        self.settings = settings
        self.setWindowTitle("SOTS Stats Hub | OpenAI: checking…")
        try:
            icon_path = Path(__file__).resolve().parent / "sots_stats_hub.ico"
            if icon_path.exists():
                self.setWindowIcon(QtGui.QIcon(str(icon_path)))
        except Exception:
            pass
        self.resize(1400, 900)
        apply_dark_theme(self)
        self.project_root = Path(settings.get("project_root") or detect_project_root())
        self.log_dir = Path(__file__).resolve().parents[1] / "_reports" / "sots_stats_hub" / "Logs"
        self.log_pane = LogPane(self.log_dir)
        self.process_manager = ProcessManager(self.log_pane.append)
        self.tools_root = Path(__file__).resolve().parents[1]
        self.store = ReportStore(self.project_root)
        self.last_runs: dict = {}
        self.job_manager = JobManager(self.project_root, self.log_dir, self.log_pane.append)
        self.job_manager.jobs_changed.connect(lambda: self.jobs_tab.refresh() if hasattr(self, "jobs_tab") else None)
        self.job_manager.job_finished.connect(self._on_job_finished)
        self.job_manager.job_output.connect(lambda msg: self.log_pane.append(msg))
        self.enable_writes = bool(self.settings.get("ui/enable_writes", False))
        self.tools = build_tools(self.project_root, settings.get("video_control_path", ""))
        self.tools_by_id = {t.id: t for t in self.tools}
        self.log_pane.append(f"[INFO] sys.executable={sys.executable}")
        self.log_pane.append(f"[INFO] project_root={self.project_root}")
        self._kickoff_usage_fetch()

        central = QtWidgets.QWidget()
        self.setCentralWidget(central)
        main_layout = QtWidgets.QVBoxLayout(central)
        menubar = self.menuBar()
        tools_menu = menubar.addMenu("Tools")
        palette_action = QtGui.QAction("Command Palette…", self)
        palette_action.setShortcut("Ctrl+K")
        palette_action.triggered.connect(self._open_palette)
        tools_menu.addAction(palette_action)
        self.enable_writes_action = QtGui.QAction("Enable write actions", self)
        self.enable_writes_action.setCheckable(True)
        self.enable_writes_action.setChecked(self.enable_writes)
        self.enable_writes_action.triggered.connect(self._toggle_writes)
        tools_menu.addAction(self.enable_writes_action)
        self.tabs = QtWidgets.QTabWidget()
        main_layout.addWidget(self.tabs, stretch=1)
        main_layout.addWidget(self.log_pane)

        self.log_visible = True
        self.disk_tab = DiskTab(self.log_pane.append, settings, self.project_root, toggle_log_area=self._set_log_visible)
        self.tabs.addTab(self.disk_tab, "SOTS Disk")

        self.actions_tab = ActionsPanel(
            self.process_manager,
            self.tools_root,
            settings.get("video_control_path", ""),
            log=self.log_pane.append,
            enqueue_job=self._enqueue_job,
            tool_specs=self.tools,
            run_tool=self._run_tool,
        )
        self.tabs.addTab(self.actions_tab, "Actions")

        self.docs_tab = DocsWatchPanel(
            self.project_root,
            poll_seconds=float(settings.get("docs_watch_poll_seconds", 10)),
            log=self.log_pane.append,
            enqueue_job=lambda label, func: self._enqueue_job(label, func=func),
            guard_writes=self._guard_writes,
        )
        self.docs_tab.pollChanged.connect(lambda v: settings.set("docs_watch_poll_seconds", v))
        self.tabs.addTab(self.docs_tab, "Docs Watch")

        self.zip_tab = ZipPanel(
            self.process_manager,
            self.tools_root,
            self.log_pane.append,
            enqueue_job=self._enqueue_job,
            run_tool=self._run_tool,
            tool_lookup=self.tools_by_id,
        )
        self.tabs.addTab(self.zip_tab, "Zips")

        self.reports_tab = ReportsTab(
            self.project_root,
            self.store,
            self.process_manager,
            self.log_pane.append,
            self._on_reports_changed,
            self._record_run,
            self._enqueue_job,
            settings,
            run_tool=self._run_tool,
            tool_lookup=self.tools_by_id,
        )
        self.tabs.addTab(self.reports_tab, "Reports")

        self.plugins_tab = PluginsTab(self.store, self.log_pane.append, self._on_plugin_selected)
        self.tabs.addTab(self.plugins_tab, "Plugins")

        self.search_tab = SearchTab(self.store, self.project_root, self.log_pane.append)
        self.tabs.addTab(self.search_tab, "Search")

        self.hotspots_tab = HotspotsTab(self.store)
        self.tabs.addTab(self.hotspots_tab, "Hotspots")

        self.graph_tab = GraphTab(self.store)
        self.tabs.addTab(self.graph_tab, "Graph")

        self.legacy_tab = LegacyTab(self.project_root, settings.get("control_center_path", ""))
        self.tabs.addTab(self.legacy_tab, "Legacy")
        self.settings_tab = self._build_settings_tab()
        self.tabs.addTab(self.settings_tab, "Settings")
        self.tabs.addTab(QtWidgets.QLabel("Logs are below; use tabs above."), "Info")

        self.statusBar().showMessage(f"Project root: {self.project_root}")
        if not settings.get("project_root"):
            settings.set("project_root", str(self.project_root))

        # Reports watcher + poller
        self.report_watcher = QtCore.QFileSystemWatcher(self._existing_report_paths())
        self.report_watcher.fileChanged.connect(self._on_reports_changed)
        self.report_timer = QtCore.QTimer(self)
        self.report_timer.setInterval(2000)
        self.report_timer.timeout.connect(self._poll_reports)
        self.report_timer.start()
        self._refresh_tabs()

    def _existing_report_paths(self) -> List[str]:
        paths = [str(p) for p in self.store.paths.values() if p.exists()]
        rep_dir = str(reports_root(self.project_root))
        if rep_dir not in paths:
            paths.append(rep_dir)
        return paths

    def _on_reports_changed(self, _path: str = "") -> None:
        if self.store.refresh():
            self._refresh_tabs()
            self.log_pane.append("[REPORTS] refreshed from file change")
        self.reports_tab.refresh_view()

    def _poll_reports(self) -> None:
        if self.store.has_changes():
            self._on_reports_changed()

    def _refresh_tabs(self) -> None:
        self.plugins_tab.refresh()
        self.search_tab.refresh()
        self.hotspots_tab.refresh()
        self.graph_tab.refresh()
        self.reports_tab.refresh_view()

    def _on_plugin_selected(self, name: str) -> None:
        self.log_pane.append(f"[PLUGIN] selected {name}")

    def _record_run(self, cmd: str, code: int) -> None:
        stamp = time.strftime("%Y-%m-%d %H:%M:%S")
        self.last_runs[cmd] = {"code": code, "when": stamp}
        self.runs_tab.refresh()

    def _toggle_writes(self, checked: bool) -> None:
        self.enable_writes = bool(checked)
        self.settings.set("ui/enable_writes", self.enable_writes)
        self.enable_writes_action.setChecked(self.enable_writes)
        if hasattr(self, "enable_writes_cb"):
            self.enable_writes_cb.setChecked(self.enable_writes)
        state = "enabled" if self.enable_writes else "disabled"
        self.log_pane.append(f"[INFO] write actions {state}")

    def _guard_writes(self, reason: str) -> bool:
        if self.enable_writes:
            return True
        msg = QtWidgets.QMessageBox(self)
        msg.setWindowTitle("Write actions disabled")
        msg.setText(f"{reason} is blocked because write actions are disabled.\n\nEnable them in Settings if you understand the impact.")
        settings_btn = msg.addButton("Open Settings", QtWidgets.QMessageBox.ActionRole)
        msg.addButton(QtWidgets.QMessageBox.Close)
        msg.exec()
        if msg.clickedButton() == settings_btn and hasattr(self, "settings_tab"):
            self.tabs.setCurrentWidget(self.settings_tab)
        return False

    def _enqueue_job(
        self,
        label: str,
        func=None,
        args: Optional[List[str]] = None,
        cwd: Optional[Path] = None,
        *,
        tool_id: Optional[str] = None,
        outputs_hint: Optional[List[str]] = None,
        category: str = "job",
    ) -> None:
        if func:
            self.job_manager.enqueue(label, func=lambda log_fn: func(log_fn), tool_id=tool_id, outputs_hint=outputs_hint, category=category)
        elif args:
            self.job_manager.enqueue(label, args=args, cwd=cwd or self.project_root, tool_id=tool_id, outputs_hint=outputs_hint, category=category)

    def _cwd_for_tool(self, spec: ToolSpec) -> Path:
        if spec.cwd_policy == "devtools_root":
            return self.tools_root
        if spec.cwd_policy == "script_dir":
            return Path(spec.entrypoint).resolve().parent if isinstance(spec.entrypoint, str) else self.project_root
        return self.project_root

    def _run_tool(self, spec: ToolSpec) -> None:
        if spec.safety_level != "read_only" and not self._guard_writes(spec.label):
            return
        if spec.requires_confirm:
            text = spec.confirm_text or f"Run {spec.label}?"
            res = QtWidgets.QMessageBox.question(self, "Confirm action", text)
            if res != QtWidgets.QMessageBox.Yes:
                return
        if spec.id == "capture_ffmpeg":
            # Match legacy behavior: open the capture script file, do not execute
            script = self.tools_root / "sots_capture_ffmpeg.py"
            self._open_explorer(script)
            self.log_pane.append(f"[TOOL:{spec.id}] opened capture script (not running): {script}")
            return
        if spec.id == "sots_tools":
            script = self.tools_root / "sots_tools.py"
            self._open_explorer(script)
            self.log_pane.append(f"[TOOL:{spec.id}] opened sots_tools script (not running): {script}")
            return
        self.log_pane.append(f"[TOOL:{spec.id}] queued {spec.label}")
        if spec.kind == "function":
            if spec.entrypoint in {"start_bridge", "stop_bridge"}:
                script = self.tools_root / "sots_bridge_server.py"
                def _bridge_job(log_fn):
                    if spec.entrypoint == "start_bridge":
                        log_fn("[JOB] starting bridge server")
                        self.process_manager.start_bridge(script)
                        log_fn("[JOB] bridge server start requested")
                    else:
                        log_fn("[JOB] stopping bridge server")
                        self.process_manager.stop_bridge()
                        log_fn("[JOB] bridge server stop requested")
                    return 0
                _bridge_job(self.log_pane.append)
                return
            if callable(spec.entrypoint):
                def _func_job(log_fn):
                    try:
                        res = spec.entrypoint()
                        return int(res) if res is not None else 0
                    except Exception as exc:  # noqa: BLE001
                        log_fn(f"[ERR] tool {spec.id} failed: {exc}")
                        return -1
                _func_job(self.log_pane.append)
                return
        if spec.kind == "external_exe":
            args = [spec.entrypoint] + list(spec.default_args)
            self.log_pane.append(f"[TOOL:{spec.id}] launch external: {' '.join(args)}")
            self.process_manager.run_detached(args)
            return
        args = [sys.executable, spec.entrypoint] + list(spec.default_args) if isinstance(spec.entrypoint, str) else list(spec.default_args)
        creationflags = 0
        if sys.platform.startswith("win"):
            creationflags = 0x00000008 | 0x00000200  # DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP
        try:
            import subprocess

            self.log_pane.append(f"[TOOL:{spec.id}] running direct: {' '.join(args)}")
            subprocess.Popen(args, cwd=str(self._cwd_for_tool(spec)), creationflags=creationflags)
        except Exception as exc:  # noqa: BLE001
            self.log_pane.append(f"[ERR] tool {spec.id} failed to start: {exc}")

    def _latest_output(self, hints: List[str]) -> Optional[Path]:
        newest: tuple[float, Path] | None = None
        for hint in hints:
            hint_path = Path(hint)
            candidates = [hint_path]
            if any(ch in hint_path.name for ch in ["*", "?"]):
                candidates = list(hint_path.parent.glob(hint_path.name))
            for cand in candidates:
                if cand.exists():
                    ts = cand.stat().st_mtime
                    if newest is None or ts > newest[0]:
                        newest = (ts, cand)
        return newest[1] if newest else None

    def _on_job_finished(self, job) -> None:
        if job.tool_id:
            self.log_pane.append(f"[TOOL:{job.tool_id}] finished {job.label} exit={job.exit_code}")
        if job.tool_id and job.outputs_hint:
            last = self._latest_output(job.outputs_hint)
            if last:
                self.log_pane.append(f"[TOOL:{job.tool_id}] last output: {last}")
                box = QtWidgets.QMessageBox(self)
                box.setWindowTitle("Job complete")
                box.setText(f"{job.label}\nExit code: {job.exit_code}")
                open_btn = box.addButton("Open Folder", QtWidgets.QMessageBox.ActionRole)
                copy_btn = box.addButton("Copy Path", QtWidgets.QMessageBox.ActionRole)
                box.addButton(QtWidgets.QMessageBox.Ok)
                box.exec()
                if box.clickedButton() == open_btn:
                    self._open_explorer(last if last.is_dir() else last.parent)
                elif box.clickedButton() == copy_btn:
                    QtWidgets.QApplication.clipboard().setText(str(last))

    def _open_explorer(self, path: Path) -> None:
        try:
            import subprocess

            subprocess.Popen(["explorer", str(path)])
        except Exception:
            self.log_pane.append(f"[WARN] Could not open {path}")

    def _refresh_tool_registry(self) -> None:
        self.tools = build_tools(self.project_root, self.settings.get("video_control_path", ""))
        self.tools_by_id = {t.id: t for t in self.tools}
        if hasattr(self, "actions_tab") and hasattr(self.actions_tab, "update_tools"):
            self.actions_tab.update_tools(self.tools)

    def _build_settings_tab(self) -> QtWidgets.QWidget:
        tab = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout(tab)
        self.enable_writes_cb = QtWidgets.QCheckBox("Enable write actions (gates zips/docs/servers)")
        self.enable_writes_cb.setChecked(self.enable_writes)
        self.enable_writes_cb.toggled.connect(self._toggle_writes)
        layout.addWidget(self.enable_writes_cb)

        video_row = QtWidgets.QHBoxLayout()
        video_row.addWidget(QtWidgets.QLabel("Video Control path:"))
        self.video_path_edit = QtWidgets.QLineEdit(self.settings.get("video_control_path", ""))
        browse_btn = QtWidgets.QPushButton("Browse")
        video_row.addWidget(self.video_path_edit, stretch=1)
        video_row.addWidget(browse_btn)
        layout.addLayout(video_row)

        def _save_video_path() -> None:
            self.settings.set("video_control_path", self.video_path_edit.text().strip())
            self._refresh_tool_registry()
            self.log_pane.append("[INFO] Video control path updated.")

        self.video_path_edit.editingFinished.connect(_save_video_path)

        def _browse() -> None:
            path, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Select Video Control executable", self.video_path_edit.text())
            if path:
                self.video_path_edit.setText(path)
                _save_video_path()

        browse_btn.clicked.connect(_browse)
        note = QtWidgets.QLabel("Write actions stay disabled until you turn them on here or via Tools > Enable write actions.")
        note.setWordWrap(True)
        layout.addWidget(note)

        openai_group = QtWidgets.QGroupBox("OpenAI usage")
        og_layout = QtWidgets.QVBoxLayout(openai_group)
        self.openai_usage_cb = QtWidgets.QCheckBox("Fetch usage on launch")
        self.openai_usage_cb.setChecked(bool(self.settings.get("openai/usage_enabled", True)))
        og_layout.addWidget(self.openai_usage_cb)
        org_row = QtWidgets.QHBoxLayout()
        org_row.addWidget(QtWidgets.QLabel("Organization (optional):"))
        self.openai_org_edit = QtWidgets.QLineEdit(self.settings.get("openai/org", ""))
        org_row.addWidget(self.openai_org_edit, stretch=1)
        og_layout.addLayout(org_row)
        layout.addWidget(openai_group)

        def _save_openai() -> None:
            self.settings.set("openai/usage_enabled", self.openai_usage_cb.isChecked())
            self.settings.set("openai/org", self.openai_org_edit.text().strip())
            self.log_pane.append("[INFO] OpenAI usage settings updated; restart or re-open window to refresh usage.")

        self.openai_usage_cb.toggled.connect(lambda _: _save_openai())
        self.openai_org_edit.editingFinished.connect(_save_openai)
        layout.addStretch(1)
        return tab

    def _palette_commands(self) -> List[tuple[str, Callable[[], None]]]:
        cmds: List[tuple[str, Callable[[], None]]] = []
        tab_map = {
            "SOTS Disk": self.disk_tab,
            "Actions": self.actions_tab,
            "Docs Watch": self.docs_tab,
            "Zips": self.zip_tab,
            "Reports": self.reports_tab,
            "Plugins": self.plugins_tab,
            "Search": self.search_tab,
            "Hotspots": self.hotspots_tab,
            "Graph": self.graph_tab,
            "Legacy": self.legacy_tab,
            "Settings": self.settings_tab,
        }
        for label, widget in tab_map.items():
            cmds.append((f"Go to {label}", lambda w=widget: self.tabs.setCurrentWidget(w)))
        cmds.append(("Generate ALL reports", lambda: self.reports_tab.run_all()))
        cmds.append(("Generate Smart reports", lambda: self.reports_tab.run_smart()))
        cmds.append(("Docs: Combo Zip", lambda: self.docs_tab._on_combo_zip()))
        cmds.append(("Docs: Scan Now", lambda: self.docs_tab.scan_once()))
        cmds.append(("Docs: ACK Baseline", lambda: self.docs_tab._on_ack()))
        cmds.append(("Zip Plugin Sources", lambda: self.zip_tab._on_zip_plugins()))
        cmds.append(("Zip All Docs", lambda: self.zip_tab._on_zip_docs()))
        cmds.append(("Open Reports Folder", lambda: self._open_explorer(reports_root(self.project_root))))
        cmds.append(("Open Logs Folder", lambda: self._open_explorer(self.log_pane.log_path.parent)))
        for spec in self.tools:
            cmds.append((f"Run tool: {spec.label}", lambda s=spec: self._run_tool(s)))
        # plugin commands (pinned first)
        names = [r.name for r in self.store.plugin_rows if r.pinned] + [r.name for r in self.store.plugin_rows if not r.pinned]
        for name in names:
            cmds.append((f"Go to plugin {name}", lambda n=name: (self.tabs.setCurrentWidget(self.plugins_tab), self.plugins_tab.select_plugin(n))))
        return cmds

    def _open_palette(self) -> None:
        commands = self._palette_commands()
        recent = self.settings.get("ui/palette_recent", []) or []
        dlg = CommandPalette(commands, recent=recent, parent=self)
        if dlg.exec() == QtWidgets.QDialog.Accepted:
            sel = dlg.filtered[dlg.list.currentRow()][0] if dlg.list.currentRow() >= 0 else None
            if sel:
                recent_list = [sel] + [c for c in recent if c != sel]
                self.settings.set("ui/palette_recent", recent_list[:10])

    def closeEvent(self, event) -> None:  # type: ignore[override]
        try:
            self.disk_tab.save_splitters()
        except Exception:
            pass
        super().closeEvent(event)

    def _set_log_visible(self, visible: bool) -> None:
        self.log_visible = bool(visible)
        self.log_pane.setVisible(self.log_visible)

    def _kickoff_usage_fetch(self) -> None:
        if not bool(self.settings.get("openai/usage_enabled", True)):
            self._set_usage_title("disabled")
            self.log_pane.append("[INFO] OpenAI usage fetch disabled in settings.")
            return
        key_env = "OPENAI_API_KEY" if os.getenv("OPENAI_API_KEY") else "OPENAI_KEY"
        key = os.getenv(key_env)
        if not key:
            self._set_usage_title("missing")
            self.log_pane.append("[INFO] OPENAI_API_KEY/OPENAI_KEY not set; usage unavailable.")
            return
        self.log_pane.append(self._mask_key_info(key_env, key))

        def worker() -> None:
            self.log_pane.append("[INFO] Fetching OpenAI usage…")
            usage = self._fetch_openai_usage(key)
            QtCore.QMetaObject.invokeMethod(
                self,
                "_set_usage_title",
                QtCore.Qt.QueuedConnection,
                QtCore.Q_ARG(str, usage),
            )

        threading.Thread(target=worker, daemon=True).start()

    def _fetch_openai_usage(self, key: str) -> str:
        end = datetime.date.today()
        start = end - datetime.timedelta(days=30)
        url = f"https://api.openai.com/dashboard/billing/usage?start_date={start}&end_date={end}"
        headers = {"Authorization": f"Bearer {key}"}
        org = self.settings.get("openai/org") or os.getenv("OPENAI_ORG")
        if org:
            headers["OpenAI-Organization"] = org
        req = urllib.request.Request(url, headers=headers)
        try:
            with urllib.request.urlopen(req, timeout=10) as resp:
                data = json.loads(resp.read().decode("utf-8"))
                total_cents = data.get("total_usage", 0)
                total_dollars = (float(total_cents) / 100.0) if total_cents else 0.0
                self.log_pane.append(f"[INFO] OpenAI usage fetched: ${total_dollars:.2f}/30d")
                return f"${total_dollars:.2f}/30d"
        except urllib.error.HTTPError as exc:
            hint = ""
            if exc.code == 401:
                hint = " (check key/org scope or billing access)"
            elif exc.code == 429:
                hint = " (rate limit/billing cap)"
            self.log_pane.append(f"[WARN] OpenAI usage fetch failed ({exc.code}): {exc.reason}{hint}")
            return "unauthorized" if exc.code == 401 else f"error {exc.code}"
        except urllib.error.URLError as exc:
            self.log_pane.append(f"[WARN] OpenAI usage fetch blocked/unreachable: {exc}")
            return "unavailable"
        except Exception as exc:  # noqa: BLE001
            self.log_pane.append(f"[WARN] OpenAI usage fetch failed: {exc}")
            return "unavailable"

    def _mask_key_info(self, env_name: str, key: str) -> str:
        prefix = key[:4]
        suffix = key[-4:] if len(key) > 4 else ""
        masked_mid = "*" * max(4, len(key) - len(prefix) - len(suffix))
        return f"[INFO] Using {env_name}: {prefix}{masked_mid}{suffix} (len={len(key)})"

    @QtCore.Slot(str)
    def _set_usage_title(self, usage: str) -> None:
        self.setWindowTitle(f"SOTS Stats Hub | OpenAI: {usage}")


def main() -> int:
    app = QtWidgets.QApplication(sys.argv)
    settings = Settings()
    window = StatsHubWindow(settings)
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
