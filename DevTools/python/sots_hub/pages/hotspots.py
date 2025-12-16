from __future__ import annotations

from nicegui import ui

from ..services.search_index import build_hotspot_entries, filter_hotspots
from .layout import page_layout


def render(store, state, on_refresh) -> None:
    with page_layout("/hotspots", state, on_refresh):
        ui.label("Hotspots").classes("text-lg font-semibold")
        ui.label("Ranks top problem plugins/files by TODO + tag density.").classes("text-sm text-slate-600")

        plugin_entries, file_entries = build_hotspot_entries(store)
        if not plugin_entries and not file_entries:
            ui.label("No TODO/tag data loaded yet. Run the backlog/tag reports to populate hotspots.").classes("text-sm text-slate-600")
            return

        with ui.row().classes("gap-4 flex-wrap items-end"):
            sots_only = ui.checkbox("SOTS_* only", value=False)
            contains = ui.input("Contains…", placeholder="plugin or file substring").props("clearable")
            sort_choice = ui.select(
                {"score": "Score", "todo": "TODO", "tags": "Tags"},
                value="score",
                label="Sort",
            )

        chart_container = ui.column().classes("w-full gap-2")
        plugin_table_container = ui.column().classes("w-full gap-2")
        file_table_container = ui.column().classes("w-full gap-2")

        def metric_value(entry, metric: str) -> int:
            if metric == "todo":
                return entry.todo_count
            if metric == "tags":
                return entry.tag_count
            return entry.score

        def metric_label(metric: str) -> str:
            return {"todo": "TODOs", "tags": "Tags", "score": "Score"}.get(metric, "Score")

        def refresh_views() -> None:
            metric = sort_choice.value or "score"
            filt_plugins = filter_hotspots(plugin_entries, sots_only.value, contains.value, metric)[:15]
            filt_files = filter_hotspots(file_entries, sots_only.value, contains.value, metric)[:25]

            chart_container.clear()
            plugin_table_container.clear()
            file_table_container.clear()

            with chart_container:
                ui.label(f"Top plugins ({metric_label(metric)})").classes("font-semibold")
                if filt_plugins:
                    labels = [e.plugin for e in reversed(filt_plugins)]
                    values = [metric_value(e, metric) for e in reversed(filt_plugins)]
                    ui.echart(
                        {
                            "tooltip": {"trigger": "axis"},
                            "grid": {"left": "30%", "top": "10%", "bottom": "10%"},
                            "xAxis": {"type": "value", "minInterval": 1},
                            "yAxis": {"type": "category", "data": labels},
                            "series": [{"type": "bar", "data": values, "label": {"show": True, "position": "right"}}],
                        }
                    ).classes("w-full").style("height: 420px")
                else:
                    ui.label("No plugins match the current filters.").classes("text-sm text-slate-600")

            with plugin_table_container:
                plugin_rows = [
                    {"plugin": e.plugin, "todo": e.todo_count, "tags": e.tag_count, "score": e.score}
                    for e in filt_plugins
                ]
                plugin_columns = [
                    {"name": "plugin", "label": "Plugin", "field": "plugin", "sortable": True},
                    {"name": "todo", "label": "TODO", "field": "todo", "sortable": True},
                    {"name": "tags", "label": "Tags", "field": "tags", "sortable": True},
                    {"name": "score", "label": "Score", "field": "score", "sortable": True},
                    {"name": "actions", "label": "Actions", "field": "actions", "sortable": False},
                ]
                plugin_table = ui.table(columns=plugin_columns, rows=plugin_rows, row_key="plugin").classes("w-full")
                plugin_table.add_slot(
                    "body-cell-actions",
                    r"""
<q-td :props="props">
  <q-btn dense flat size="sm" icon="open_in_new" title="Open Plugin"
    @click="window.location.href = '/plugin/' + encodeURIComponent(props.row.plugin)"
  />
</q-td>
""",
                )

            with file_table_container:
                ui.label(f"Top files ({metric_label(metric)})").classes("font-semibold")
                file_rows = [
                    {
                        "plugin": e.plugin,
                        "path": e.file or "",
                        "todo": e.todo_count,
                        "tags": e.tag_count,
                        "score": e.score,
                    }
                    for e in filt_files
                ]
                file_columns = [
                    {"name": "plugin", "label": "Plugin", "field": "plugin", "sortable": True},
                    {"name": "path", "label": "File", "field": "path", "sortable": True},
                    {"name": "todo", "label": "TODO", "field": "todo", "sortable": True},
                    {"name": "tags", "label": "Tags", "field": "tags", "sortable": True},
                    {"name": "score", "label": "Score", "field": "score", "sortable": True},
                    {"name": "actions", "label": "Actions", "field": "actions", "sortable": False},
                ]
                file_table = ui.table(columns=file_columns, rows=file_rows, row_key="path").classes("w-full")
                file_table.add_slot(
                    "body-cell-actions",
                    r"""
<q-td :props="props">
  <q-btn dense flat size="sm" icon="open_in_new" title="Open Plugin"
    @click="window.location.href = '/plugin/' + encodeURIComponent(props.row.plugin)"
  />
  <q-btn dense flat size="sm" icon="content_copy" title="Copy path"
    @click="navigator.clipboard.writeText(String(props.row.path || ''))"
  />
</q-td>
""",
                )

        sots_only.on_value_change(lambda _: refresh_views())
        contains.on("update:model-value", lambda e: refresh_views())
        sort_choice.on_value_change(lambda _: refresh_views())

        refresh_views()

