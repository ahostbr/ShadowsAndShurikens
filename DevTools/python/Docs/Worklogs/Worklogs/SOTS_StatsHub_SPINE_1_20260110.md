# Worklog: SOTS Stats Hub (SPINE_1)
- Added new PySide6 app `sots_stats_hub` with tabs for Disk view, Actions, Docs Watch, Zips, and Logs.
- Reused DiskStat scan/treemap logic for the Disk tab without modifying the standalone package.
- Actions tab runs `sots_tools`, starts/stops bridge server, launches capture, and optional video control.
- Docs Watch tab surfaces `watch_new_docs.py` behavior (poll/scan/zip/ack) using the same state + report paths.
- Zips tab triggers `zip_sots_suite_plugin_sources.py` and `zip_all_docs.py` with log output.
- Logging: UI log pane plus file under `_reports/sots_stats_hub/Logs/`; settings stored via QSettings and `_state/sots_stats_hub_settings.json`.
- Added launcher `run_sots_stats_hub.py` and package README.

## How to run
- From `DevTools/python`: `python -m sots_stats_hub` (or `python run_sots_stats_hub.py`).

## Known limits
- sots_tools integration is an args dialog (no per-command UI yet).
- Bridge server stop uses terminate; no status polling beyond PID check.
- Zips tab shows last path only when populated from script output in future iterations.
