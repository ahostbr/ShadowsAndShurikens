# Buddy Worklog — Stats Hub canonical + disk scan UX (2025-12-21)

- Ensured single canonical home by keeping DevTools/python/sots_stats_hub as source; prior duplicates remain shims.
- Disk tab improvements: auto-scan on launch (setting), preset scan buttons (Project/DevTools/Plugins), progress bar/status with last scan stats, cancel support, and safe default depth.
- Added sys.path injection guard for DevTools/python to keep sots_diskstat_gui imports robust; initial log records sys.executable/project_root.
- README updated with canonical home, entrypoints, auto-scan/presets, and ignore notes for venv/pycache/_reports/_state.
- Root .gitignore already updated to ignore .venv/__pycache__/pyc (hygiene).

## Files Touched
- `DevTools/python/sots_stats_hub/stats_hub_gui.py`
- `DevTools/python/sots_stats_hub/settings.py`
- `DevTools/python/sots_stats_hub/README.md`
- `.gitignore`

## Verify (commands only)
- `python -m sots_stats_hub`
- `python run_sots_stats_hub.py`
- `python -c "import sots_stats_hub; import sots_stats_hub.stats_hub_gui"`
- Confirm Disk tab auto-scans, shows progress + last scan stats, and preset buttons work.
