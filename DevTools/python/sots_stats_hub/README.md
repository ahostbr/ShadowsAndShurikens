# SOTS Stats Hub

Dashboard-style PySide6 app that combines DiskStat-style browsing with SOTS DevTools shortcuts.

## Run
From `DevTools/python`:
```bash
python -m sots_stats_hub
```

Optional launcher: `python run_sots_stats_hub.py` (uses pythonw when available); double-click works from Explorer. Canonical home is `DevTools/python/sots_stats_hub`.

## Tabs
- **SOTS Disk**: tree + treemap view of a root (default: auto-detected project root). Uses existing scan logic; skip-symlinks + depth controls.
- **Actions**: buttons to run `sots_tools` (args dialog), start/stop `sots_bridge_server.py`, launch `sots_capture_ffmpeg.py`, open video control (config path).
- **Docs Watch**: GUI view of `watch_new_docs.py` state; poll/scan, zip new+modified docs, mark handled, open reports folder.
- **Zips**: run `zip_sots_suite_plugin_sources.py` or `zip_all_docs.py`, track last path.
- **Reports**: generate DepMap/TODO/Tag/API reports, show status/paths/mtimes, auto-refresh on file change.
- **Plugins**: list plugins with inbound/outbound/TODO/Tags, pinning persisted to `DevTools/reports/sots_stats_hub_pins.json`, detail panes for TODO/Tags/API.
- **Search**: unified search over TODO/Tags/API with Explorer open.
- **Hotspots**: top plugins/files by score (todo+tags).
- **Graph**: fallback edges table (plugin -> dependency) with filters.
- **Runs**: recent report command runs + copy-to-clipboard shortcuts.
- **Jobs**: serial job queue with cancel + history; per-job logs under `_reports/sots_stats_hub/JobLogs/`.
- **Command Palette**: Ctrl+K (Tools > Command Palette) for fuzzy navigation/actions.
- **Smart reports**: toggle + stale-minutes threshold; Generate Smart only runs missing/stale reports.
- **Docs combo zip**: one-click “Zip New/Modified” combo that scans, zips, and ACKs baseline.
- **Logs**: rolling log pane + file under `_reports/sots_stats_hub/Logs/`.
- **Legacy**: quick links to NiceGUI `sots_hub` and control center, marked as retiring after SPINE_4 parity.
- **Settings**: enable/disable write actions, configure VideoControl path, refresh tool registry.
- **Tool Registry**: single source for Actions/Palette/Jobs; safety levels + confirmations enforced; write actions gated OFF by default.
- **Disk tab**: optional auto-scan on launch (default on) with presets (Project Root / DevTools / Plugins), status line showing last scan stats.

## Settings
Stored via QSettings (`SOTS/StatsHub`) and mirrored to `_state/sots_stats_hub_settings.json`:
- `project_root`
- `video_control_path`
- `last_disk_root`
- `docs_watch_poll_seconds`
- `reports/smart_enabled`
- `reports/stale_minutes`
- `ui/palette_recent`
- `ui/enable_writes`
- `disk/auto_scan`
- `disk/auto_scan_depth`

## Reports consumed
- `DevTools/reports/sots_plugin_depmap.json`
- `DevTools/reports/sots_todo_backlog.json`
- `DevTools/reports/sots_tag_usage_report.json`
- `DevTools/reports/sots_api_surface.json`

## Legacy note
- `sots_hub` (NiceGUI) is legacy and will be retired after SPINE_4 parity is confirmed. Links remain for reference.
- VideoControl remains separate; set its path in Settings.

## Notes
- Uses `sys.executable` to honor the current venv.
- DiskStat package remains standalone; this hub reuses its scanner/treemap only.
- Dark theme matches DiskStat GUI palette/QSS (including scrollbars).
- All jobs/actions log to the UI pane plus per-job files under `_reports/sots_stats_hub/JobLogs/`.
- `_reports/`, `_state/`, `.venv/`, `__pycache__`, and `*.pyc` are not tracked in VCS.
