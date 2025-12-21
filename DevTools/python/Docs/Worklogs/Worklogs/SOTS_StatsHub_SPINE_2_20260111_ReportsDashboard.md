# Worklog: SOTS Stats Hub (SPINE_2 Reports Dashboard)
- Added reports_core (paths, models, report_store, plugin_view, search_index, command_catalog) to load DepMap/TODO/Tag/API reports, track mtimes/errors, pins, search, and hotspots.
- Expanded UI with new tabs: Reports (generate buttons + status cards), Plugins (pinning, detail panes), Search, Hotspots, Graph fallback (edges table), Runs (recent commands), with auto-refresh via watcher/poll.
- Integrated project-root settings, report watcher/poller, run tracking, and refreshed tabs into the main window.
- Copied package and launcher into correct `DevTools/python` location; preserved SPINE_1 tabs (Disk, Actions, Docs Watch, Zips, Logs).

## How to run
- From `DevTools/python`: `python -m sots_stats_hub` (or `python run_sots_stats_hub.py`).

## Verify
- Generate ALL on Reports tab -> JSONs update, status cards refresh, logs stream.
- Plugins tab shows inbound/outbound/TODO/Tags/API; pin/unpin persists in `DevTools/reports/sots_stats_hub_pins.json`.
- Search tab finds TODO/Tag/API entries; double-click opens in Explorer.
- Hotspots tab lists top plugins/files; Graph tab shows edges table with filters.
- Runs tab records exit codes/timestamps for report commands.

## Follow-ups
- Enhance graph with QtWebEngine/Cytoscape when available.
- Improve error surfacing for report parsing and missing files.
