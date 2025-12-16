from __future__ import annotations

from nicegui import ui

from ..report_loader import ReportBundle
from .layout import page_layout


def render(bundle: ReportBundle, state, on_refresh) -> None:
    with page_layout("/plugins", state, on_refresh):
        ui.label("Plugins overview").classes("text-lg font-semibold")

        if not bundle.plugin_rows:
            ui.label("No plugin data available. Generate depmap / backlog / tag reports to populate this view.").classes("text-sm text-slate-600")
            return

        def _toggle_pin(plugin: str) -> None:
            state.toggle_pin(plugin)
            ui.notify(f"{'Pinned' if plugin in state.pins else 'Unpinned'}: {plugin}")
            ui.run_javascript("window.location.reload()")

        pinned = sorted(state.pins)
        if pinned:
            ui.label("Pinned plugins").classes("text-sm font-semibold")
            with ui.column().classes("gap-1"):
                for plugin in pinned:
                    with ui.row().classes("items-center gap-2"):
                        ui.link(plugin, f"/plugin/{plugin}").classes("font-mono text-sm")
                        ui.button("Unpin", icon="push_pin", on_click=lambda _, p=plugin: _toggle_pin(p)).props("flat dense color=primary")
        else:
            ui.label("Pinned plugins: none").classes("text-sm text-slate-600")

        plugin_names = [p.name for p in bundle.plugin_rows]
        with ui.row().classes("items-end gap-2"):
            picker = ui.select(plugin_names, label="Pin/Unpin plugin", value=plugin_names[0] if plugin_names else None)
            ui.button("Toggle pin", icon="push_pin", on_click=lambda: _toggle_pin(str(picker.value))).props("flat color=primary")

        columns = [
            {"name": "name", "label": "Plugin", "field": "name", "sortable": True},
            {"name": "pinned", "label": "Pinned", "field": "pinned", "sortable": True},
            {"name": "dependencies", "label": "Deps", "field": "dependencies", "sortable": True},
            {"name": "modules", "label": "Modules", "sortable": True, "field": "modules"},
            {"name": "todo_items", "label": "TODOs", "field": "todo_items", "sortable": True},
            {"name": "tag_matches", "label": "Tags", "field": "tag_matches", "sortable": True},
        ]

        pinned_first = sorted(bundle.plugin_rows, key=lambda p: (p.name not in state.pins, p.name))
        rows = [
            {
                "name": p.name,
                "pinned": "yes" if p.name in state.pins else "",
                "dependencies": p.dependencies,
                "modules": p.modules,
                "todo_items": p.todo_items,
                "tag_matches": p.tag_matches,
            }
            for p in pinned_first
        ]

        ui.table(columns=columns, rows=rows, row_key="name").classes("w-full")
