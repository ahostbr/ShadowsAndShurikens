# SOTS Stats Hub - SPINE 4 Registry/Safety/Legacy Worklog (2025-12-21)

- Added tool registry (ToolSpec/build_tools) covering core/report/watch/zip/external tools; Actions tab and Command Palette now source from registry.
- Implemented write-action gating (default off), confirmation prompts, and settings toggle; added Settings tab to manage gating and VideoControl path.
- Centralized tool execution through JobManager with per-job logs, [TOOL:id] prefixes, and completion toast for outputs.
- Added Legacy tab for legacy hubs, and applied DiskStat dark theme palette/QSS to the hub UI.
- Updated README with registry, safety gating, legacy notes, settings keys, and dark theme/logging details.

## Files Touched
- `DevTools/python/sots_stats_hub/tool_registry.py`
- `DevTools/python/sots_stats_hub/stats_hub_gui.py`
- `DevTools/python/sots_stats_hub/jobs.py`
- `DevTools/python/sots_stats_hub/settings.py`
- `DevTools/python/sots_stats_hub/widgets/actions_panel.py`
- `DevTools/python/sots_stats_hub/widgets/zip_panel.py`
- `DevTools/python/sots_stats_hub/widgets/docs_watch_panel.py`
- `DevTools/python/sots_stats_hub/widgets/legacy_tab.py`
- `DevTools/python/sots_stats_hub/README.md`

## Verify (commands only; do not paste output)
- `python -m sots_stats_hub`
- In UI: open Command Palette (Ctrl+K) → run a report; confirm job queue logs and per-job log file.
- Toggle “Enable write actions” in Settings, then run “Zip All Docs” via Actions/Palette; confirm gating + logs.
- Open Legacy tab links; confirm they launch Explorer targets.
