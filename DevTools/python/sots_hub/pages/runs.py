from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path

from nicegui import ui

from ..services.command_catalog import COMMAND_TEMPLATES
from ..services.report_store import ReportStore
from ..report_loader import ReportBundle
from .layout import page_layout


def _copy_to_clipboard(command: str) -> None:
    ui.run_javascript(f"navigator.clipboard.writeText(`{command}`);")
    ui.notify("Copied")


def _copy_text(text: str) -> None:
    payload = json.dumps(text)
    ui.run_javascript(f"navigator.clipboard.writeText({payload});")
    ui.notify("Copied")


def _freshness_label(path: Path) -> tuple[str, str]:
    if not path.exists():
        return "Missing", "text-xs text-rose-600"
    try:
        mtime = datetime.fromtimestamp(path.stat().st_mtime, tz=timezone.utc)
    except Exception:
        return "Found", "text-xs text-emerald-600"

    delta = datetime.now(tz=timezone.utc) - mtime
    secs = int(delta.total_seconds())
    if secs < 60:
        return f"Updated {secs}s ago", "text-xs text-emerald-600"
    mins = secs // 60
    if mins < 60:
        return f"Updated {mins}m ago", "text-xs text-emerald-600"
    hrs = mins // 60
    if hrs < 48:
        return f"Updated {hrs}h ago", "text-xs text-slate-600"
    days = hrs // 24
    return f"Updated {days}d ago", "text-xs text-slate-600"


def _output_label(output: str, present: bool, error: str | None) -> str:
    status = "found" if present else "missing"
    suffix = f" ({error})" if error and not present else ""
    return f"{output} - {status}{suffix}"


def render(bundle: ReportBundle, store: ReportStore, state, on_refresh) -> None:
    with page_layout("/runs", state, on_refresh):
        ui.label("Runs (copy-only)").classes("text-lg font-semibold")
        ui.label("DevTools scripts are not executed automatically. Copy a command and run it yourself.").classes("text-sm text-slate-600")

        with ui.row().classes("gap-4 flex-wrap"):
            for template in COMMAND_TEMPLATES:
                result = store.results.get(template.key)
                present = bool(result and result.error is None)
                json_path = Path(template.outputs[0])
                log_path = json_path.with_suffix(".log")
                if result and result.log_path:
                    log_path = result.log_path

                with ui.card().classes("w-96 gap-2"):
                    with ui.row().classes("items-center justify-between w-full"):
                        ui.label(template.title).classes("font-semibold")
                        freshness, freshness_class = _freshness_label(json_path)
                        ui.label(freshness).classes(freshness_class)
                    ui.label(template.description).classes("text-sm text-slate-600")
                    if template.note:
                        ui.label(template.note).classes("text-xs text-slate-500 italic")

                    ui.label(template.command).classes("text-xs font-mono break-all bg-slate-100 px-2 py-1 rounded")
                    ui.button("Copy command", icon="content_copy", on_click=lambda cmd=template.command: _copy_to_clipboard(cmd)).props("flat color=primary")

                    ui.label("Expected outputs").classes("text-xs font-semibold")
                    for output in template.outputs:
                        error = result.error if result else "Not yet generated"
                        label = _output_label(output, present, error)
                        color_class = "text-xs text-emerald-600" if present else "text-xs text-rose-600"
                        with ui.row().classes("items-center justify-between w-full"):
                            ui.label(label).classes(color_class)
                            ui.button(
                                "Copy output path",
                                icon="content_copy",
                                on_click=lambda p=output: _copy_text(p),
                            ).props("flat dense")

                    if log_path.exists():
                        log_text = log_path.read_text(encoding="utf-8", errors="ignore")
                        if len(log_text) > 8000:
                            log_text = log_text[-8000:]
                        with ui.dialog() as log_dialog:
                            ui.label(f"Log: {log_path}").classes("text-sm font-semibold")
                            ui.textarea(log_text).props("readonly").classes("w-full h-60")
                            ui.button(
                                f"Copy log path",
                                on_click=lambda path=json.dumps(str(log_path)): ui.run_javascript(f"navigator.clipboard.writeText({path})"),
                            ).props("flat color=primary")
                        ui.button("View log", on_click=log_dialog.open).props("flat")
                    else:
                        ui.label("Log not generated yet.").classes("text-xs text-slate-500")
