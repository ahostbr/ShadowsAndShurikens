# SOTS Hub NiceGUI SPINE_4

**What changed**
- Interactive Cytoscape graph page (no CDN; we vendor `cytoscape.min.js` under `DevTools/python/sots_hub/assets/` and mount it at `/sots_hub_static`). Filters, tooltips, and node clicks now drive `/plugin/<name>` drilldowns, and the graph redraws when the depmap report changes.
- Plugin drilldown page surfaces outbound/inbound deps, TODOs, tag hits, and API surface data when available; inbound counts are computed from any depmap edge that targets the chosen plugin.
- Live timers on Home, Graphs, and plugin pages watch report mtimes, refresh the data model, and update the UI without restarting the hub. Runs now include log viewers with copy-path buttons.
- Hub still prints to stdout + `DevTools/logs/sots_hub_<timestamp>.log` on each start.

**How to run**
```
python -m pip install nicegui
python DevTools\python\sots_hub\app.py
```
Open `http://127.0.0.1:7777` and use the tabbed nav.

**Offline JS**
- Cytoscape.js is stored locally under `DevTools/python/sots_hub/assets/cytoscape.min.js` and exposed via `/sots_hub_static` so the graph works without network.

**Expected reports (read-only by default)**
- `DevTools/reports/sots_plugin_depmap.json`
- `DevTools/reports/sots_todo_backlog.json`
- `DevTools/reports/sots_tag_usage_report.json`
- `DevTools/reports/sots_api_surface.json`

**Plugin drilldowns**
- Dropdown/URL selects a plugin; outbound deps come from `sots_plugin_depmap.json`, inbound deps from any other plugin listing it as a dependency.
- TODOs/tags list the first 200 entries with “Show more” toggles; tables refresh when those reports are updated.
- API surface info appears only when the snapshot was generated for that plugin; otherwise you see a note and can copy the plugin name to rerun `sots_api_surface.py` via the Runs page.

**Read-only policy reminder**
- The hub never runs DevTools scripts automatically—every DevTool command is copy/paste only.
- All data comes from existing JSON/log artifacts; writes are restricted to the hub log and the user copy clipboard.
