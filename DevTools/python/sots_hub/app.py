from __future__ import annotations

import logging
from dataclasses import dataclass, field
from typing import List, Set

from nicegui import app, ui

from .config import ASSETS_DIR, REPORTS_DIR
from .report_loader import ReportBundle, load_reports
from .services.command_palette import build_command_entries, to_rows
from .services.logging_util import setup_logger
from .services.pins_store import load_pins, save_pins
from .services.report_store import ReportStore, ReportSummary
from .services.search_index import build_search_rows
from .pages import graphs, home, hotspots, plugin_detail, plugins, runs, search


@dataclass
class HubState:
    logger: logging.Logger
    bundle: ReportBundle
    report_store: ReportStore
    summary: ReportSummary
    pins: Set[str] = field(default_factory=set)
    command_rows: List[dict] = field(default_factory=list)
    search_rows: List[dict] = field(default_factory=list)

    def refresh(self) -> None:
        self.logger.info("[Hub] Manual refresh requested")
        self.report_store.refresh(self.logger, force=True)
        self.summary = self.report_store.summarize()
        self.bundle = load_reports(self.logger)
        self._rebuild_views()
        ui.notify("Reports reloaded")
        ui.run_javascript("window.location.reload()")

    def check_for_updates(self) -> set[str]:
        changed = self.report_store.refresh(self.logger, force=False)
        if changed:
            self.summary = self.report_store.summarize()
            self.bundle = load_reports(self.logger)
            self._rebuild_views()
        return changed

    def _rebuild_views(self) -> None:
        self.search_rows = [vars(r) for r in build_search_rows(self.report_store, limit=1000)]
        self.command_rows = to_rows(build_command_entries(self.report_store, self.pins))

    def toggle_pin(self, plugin: str) -> None:
        if plugin in self.pins:
            self.pins.remove(plugin)
        else:
            self.pins.add(plugin)
        save_pins(self.pins, self.logger)
        self._rebuild_views()


logger, _ = setup_logger()
logger.info("[Hub] Starting SOTS Hub (read-only). Reports folder: %s", REPORTS_DIR)

app.add_static_files("/sots_hub_static", ASSETS_DIR)
ui.add_head_html('<script src="/sots_hub_static/cytoscape.min.js"></script>', shared=True)
ui.add_head_html(
    """
<script>
document.addEventListener('keydown', function (e) {
  const key = (e.key || '').toLowerCase();
  if ((e.ctrlKey || e.metaKey) && key === 'k') {
    e.preventDefault();
    const btn = document.getElementById('cmd_palette_btn');
    if (btn) { btn.click(); }
  }
});
</script>
""",
    shared=True,
)

report_store = ReportStore()
report_store.refresh(logger, force=True)
summary = report_store.summarize()
pins = load_pins(logger)
state = HubState(
    logger=logger,
    bundle=load_reports(logger),
    report_store=report_store,
    summary=summary,
    pins=pins,
)
state._rebuild_views()


@ui.page("/")
def home_page():
    home.render(state.bundle, state.summary, state.report_store, state, state.refresh)


@ui.page("/runs")
def runs_page():
    runs.render(state.bundle, state.report_store, state, state.refresh)


@ui.page("/plugins")
def plugins_page():
    plugins.render(state.bundle, state, state.refresh)


@ui.page("/graphs")
def graphs_page():
    graphs.render(state.bundle, state.report_store, state, state.refresh)


@ui.page("/search")
def search_page():
    search.render(state.bundle, state, state.refresh)


@ui.page("/plugin/{plugin_name}")
def plugin_page(plugin_name: str):
    plugin_detail.render(plugin_name, state.report_store, state, state.refresh)


@ui.page("/hotspots")
def hotspots_page():
    hotspots.render(state.report_store, state, state.refresh)


if __name__ == "__main__":
    ui.run(title="SOTS Hub V0", host="127.0.0.1", port=7777, reload=False)
