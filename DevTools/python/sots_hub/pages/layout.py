from __future__ import annotations

from contextlib import contextmanager
from typing import Any, Callable

from nicegui import ui


NAV_LINKS = [
    ("Home", "/"),
    ("Runs", "/runs"),
    ("Plugins", "/plugins"),
    ("Graphs", "/graphs"),
    ("Search", "/search"),
    ("Hotspots", "/hotspots"),
]

_CTRL_K_HINT = "Ctrl+K"

def _fuzzy_score(label: str, query: str) -> int:
    q = (query or "").strip().lower()
    if not q:
        return 0
    hay = label.lower()
    tokens = [t for t in q.split() if t]
    if any(t not in hay for t in tokens):
        return -10_000

    if q in hay:
        return 500 + (100 - min(100, hay.index(q)))

    needle = q.replace(" ", "")
    if not needle:
        return 0
    pos = -1
    gaps = 0
    for ch in needle:
        nxt = hay.find(ch, pos + 1)
        if nxt == -1:
            return -10_000
        gaps += max(0, nxt - pos - 1)
        pos = nxt
    return 200 - min(200, gaps)


def _copy_to_clipboard(text: str) -> None:
    payload = text.replace("\\", "\\\\").replace("`", "\\`")
    ui.run_javascript(f"navigator.clipboard.writeText(`{payload}`);")
    ui.notify("Copied")


@contextmanager
def page_layout(active: str, state: Any | None = None, on_refresh: Callable[[], None] | None = None):
    cmd_dialog = ui.dialog()
    results_container = ui.column().classes("w-full gap-1 hidden")

    def _run_command(row: dict) -> None:
        kind = row.get("kind")
        if kind == "nav":
            ui.open(row.get("target") or "/")
            cmd_dialog.close()
            return
        if kind == "copy":
            _copy_to_clipboard(row.get("payload") or "")
            cmd_dialog.close()
            return
        ui.notify(f"Unknown command: {row.get('label')}", color="negative")

    def _render_palette(query: str) -> None:
        rows = list(getattr(state, "command_rows", []) or [])
        q = (query or "").strip()

        def _sort_key(r: dict) -> tuple[int, int, str]:
            priority = int(r.get("priority") or 1)
            score = _fuzzy_score(str(r.get("label") or ""), q)
            return (priority, -score, str(r.get("label") or ""))

        rows.sort(key=_sort_key)
        if q:
            rows = [r for r in rows if _fuzzy_score(str(r.get("label") or ""), q) > -10_000]
        rows = rows[:30]

        results_container.clear()
        with results_container:
            if not rows:
                ui.label("No matches.").classes("text-sm text-slate-600")
            for row in rows:
                ui.button(
                    row.get("label") or "<unnamed>",
                    on_click=lambda _=None, r=row: _run_command(r),
                ).props("flat").classes("w-full justify-start")

    with cmd_dialog:
        with ui.card().classes("w-[780px] max-w-[95vw]"):
            ui.label("Command Palette").classes("text-lg font-semibold")
            palette_input = ui.input(
                label="Type to search…",
                on_change=lambda e: _render_palette(e.value or ""),
            ).props("id=cmd_palette_input clearable")
            palette_input.on("update:model-value", lambda e: _render_palette(e.args))
            ui.label(f"{_CTRL_K_HINT} to open anywhere").classes("text-xs text-slate-500")
            results_container = ui.column().classes("w-full gap-1")
            _render_palette("")

            def _focus_input() -> None:
                ui.run_javascript('setTimeout(() => document.getElementById("cmd_palette_input")?.focus(), 50)')

            cmd_dialog.on("show", lambda _: _focus_input())

    def _open_palette() -> None:
        cmd_dialog.open()
        palette_input.value = ""
        _render_palette("")
        ui.run_javascript('setTimeout(() => document.getElementById("cmd_palette_input")?.focus(), 50)')

    with ui.header().classes("items-center justify-between bg-slate-900 text-white"):
        with ui.row().classes("items-center gap-2"):
            ui.label("SOTS Hub V0").classes("font-semibold")
            ui.label("read-only").classes("text-xs bg-slate-700 px-2 py-1 rounded")
        with ui.row().classes("items-center gap-2"):
            ui.button("", on_click=_open_palette).props('id=cmd_palette_btn flat color=white').classes("hidden")
            ui.label(_CTRL_K_HINT).classes("text-xs bg-slate-700 px-2 py-1 rounded")
            if on_refresh:
                ui.button("Refresh", icon="refresh", on_click=on_refresh).props("flat color=white text-color=white")

    with ui.row().classes("w-full min-h-screen"):
        with ui.column().classes("w-52 bg-slate-100 p-3 gap-2"):
            ui.label("Navigation").classes("text-sm font-semibold")
            for name, link in NAV_LINKS:
                classes = "w-full text-left"
                if active == link:
                    classes += " text-primary font-semibold"
                ui.link(name, link).classes(classes)
        with ui.column().classes("flex-1 p-4 gap-3") as content:
            yield content
