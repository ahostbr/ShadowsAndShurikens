from __future__ import annotations

from nicegui import ui

from ..report_loader import ReportBundle
from .layout import page_layout


def render(bundle: ReportBundle, state, on_refresh) -> None:
    with page_layout("/search", state, on_refresh):
        ui.label("Deep Search").classes("text-lg font-semibold")
        ui.label("Navigate across TODOs / tags / API snapshots.").classes("text-sm text-slate-600")

        all_rows = list(getattr(state, "search_rows", []) or [])
        if not all_rows:
            ui.label("No search data loaded. Generate TODO backlog / tag usage / API surface reports.").classes("text-sm text-slate-600")
            return

        ui.label(f"Indexed rows: {len(all_rows)}").classes("text-xs text-slate-500")

        columns = [
            {"name": "type", "label": "Type", "field": "type", "sortable": True},
            {"name": "plugin", "label": "Plugin", "field": "plugin", "sortable": True},
            {"name": "file", "label": "File", "field": "file", "sortable": True},
            {"name": "line", "label": "Line", "field": "line", "sortable": True},
            {"name": "text", "label": "Text / snippet", "field": "text", "sortable": False},
            {"name": "actions", "label": "Actions", "field": "actions", "sortable": False},
        ]

        table = ui.table(columns=columns, rows=all_rows, row_key="id").classes("w-full")
        table.add_slot(
            "body-cell-actions",
            r"""
<q-td :props="props">
  <q-btn dense flat size="sm" icon="open_in_new" title="Open Plugin"
    @click="(() => {
      const type = String(props.row.type || '').toLowerCase();
      let url = '/plugin/' + encodeURIComponent(props.row.plugin);
      const params = new URLSearchParams();
      if (type) params.set('focusType', type);
      if (props.row.file) params.set('focusFile', props.row.file);
      if (props.row.line) params.set('focusLine', props.row.line);
      const qs = params.toString();
      if (qs) url += '?' + qs;
      window.location.href = url;
    })()"
  />
  <q-btn dense flat size="sm" icon="content_copy" title="Copy file path"
    @click="navigator.clipboard.writeText(String(props.row.file || ''))"
  />
  <q-btn dense flat size="sm" icon="notes" title="Copy snippet"
    @click="navigator.clipboard.writeText(String(props.row.text || ''))"
  />
</q-td>
""",
        )

        def _apply_filter(query: str) -> None:
            needle = (query or "").lower().strip()
            if not needle:
                table.update_rows(all_rows)
                return
            filtered = [
                r
                for r in all_rows
                if needle in str(r.get("text", "")).lower()
                or needle in str(r.get("plugin", "")).lower()
                or needle in str(r.get("file", "")).lower()
                or needle in str(r.get("type", "")).lower()
            ]
            table.update_rows(filtered)

        query_input = ui.input("Search", placeholder="Type to filter…").props("clearable")
        query_input.on("update:model-value", lambda e: _apply_filter(e.args))
        query_input.classes("w-96")
