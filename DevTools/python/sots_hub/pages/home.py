from __future__ import annotations

import json

from typing import Any

from nicegui import ui

from ..report_loader import ReportBundle, ReportStatus
from ..services.report_store import ReportStore, ReportSummary
from .layout import page_layout


SUMMARY_FIELDS = [
    (
        "Dependency map",
        [
            ("Plugins", lambda s: s.plugins_count),
            ("Dependency edges", lambda s: s.dependency_edges),
        ],
    ),
    (
        "TODO backlog",
        [
            ("Items", lambda s: s.todo_total_items),
            ("Files", lambda s: s.todo_total_files),
        ],
    ),
    (
        "Tag usage",
        [
            ("Tag matches", lambda s: s.tag_total_matches),
            ("Files", lambda s: s.tag_total_files),
        ],
    ),
    (
        "API surface",
        [
            ("Plugin", lambda s: s.api_plugin or "unknown"),
            ("Headers scanned", lambda s: s.api_headers_scanned),
        ],
    ),
]


def render(bundle: ReportBundle, summary: ReportSummary, store: ReportStore, state, on_refresh) -> None:
    summary_widgets: dict[tuple[str, str], Any] = {}

    def _update_summary_labels() -> None:
        current_summary = state.summary
        for (title, label), getter in summary_getters.items():
            widget = summary_widgets[(title, label)]
            widget.set_text(f"{label}: {getter(current_summary)}")
        if current_summary.errors:
            error_text = "\n".join(f"{k}: {v}" for k, v in current_summary.errors.items())
            error_label.set_text(error_text)
        else:
            error_label.set_text("No report issues detected.")

    summary_getters: dict[tuple[str, str], callable] = {}

    with page_layout("/", state, on_refresh):
        ui.label("Home of SOTS (DevTools-only, read-only by default)").classes("text-lg font-semibold")

        with ui.row().classes("gap-4 flex-wrap"):
            for title, metrics in SUMMARY_FIELDS:
                with ui.card().classes("w-72"):
                    ui.label(title).classes("font-semibold")
                    for label, getter in metrics:
                        label_widget = ui.label(f"{label}: {getter(summary)}").classes("text-sm text-slate-500")
                        summary_widgets[(title, label)] = label_widget
                        summary_getters[(title, label)] = getter

        with ui.card().classes("border border-rose-500 bg-rose-50") as error_card:
            ui.label("Report issues").classes("font-semibold text-rose-800")
            error_label = ui.label("No report issues detected.").classes("text-sm text-rose-600")

        with ui.row().classes("gap-4 flex-wrap"):
            for key in bundle.reports:
                status = bundle.reports.get(key)
                if not status:
                    continue
                _render_report_card(status)

        _update_summary_labels()

        def _poll_updates() -> None:
            changed = state.check_for_updates()
            if changed:
                _update_summary_labels()

        ui.timer(2.0, _poll_updates)


def _render_report_card(status: ReportStatus) -> None:
    chip = "ok" if status.present else "missing"
    with ui.card().classes("w-72"):
        with ui.row().classes("items-center justify-between"):
            ui.label(status.title).classes("font-semibold")
            ui.label(chip).classes("text-xs bg-slate-200 px-2 py-1 rounded")

        if status.present and status.summary:
            for k, v in status.summary.items():
                ui.label(f"{k}: {v}").classes("text-sm")
        else:
            ui.label("No data loaded").classes("text-sm text-slate-600")

        ui.label(f"JSON: {status.json_path}").classes("text-xs text-slate-500")
        ui.label(f"Log: {status.log_path}").classes("text-xs text-slate-500")
