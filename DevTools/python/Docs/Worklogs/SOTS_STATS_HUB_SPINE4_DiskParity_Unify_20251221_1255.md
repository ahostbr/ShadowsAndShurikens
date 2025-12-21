# SOTS Stats Hub Disk Parity Unify (2025-12-21)

- Confirmed canonical home `DevTools/python/sots_stats_hub`; legacy duplicates remain shims only.
- Added shared scan worker in `sots_diskstat_gui/workers.py`; standalone diskstat imports it.
- Stats Hub Disk tab now uses the same ScanWorker pattern (signals) as diskstat_gui for reliable completion/UI populate; treemap right-click zoom-up and resize-triggered re-render match diskstat behavior.
- Launcher/theme parity unchanged; diskstat standalone remains intact.
- Removed Runs/Jobs tabs from the hub UI per request (core logic retained in code only).

## Files Touched
- `DevTools/python/sots_diskstat_gui/workers.py`
- `DevTools/python/sots_diskstat_gui/diskstat_gui.py`
- `DevTools/python/sots_stats_hub/stats_hub_gui.py`

## Validation (run if needed)
- `python -m sots_stats_hub` → Disk tab scan shows tree/treemap.
- `python run_diskstat_gui.py` → still works.
- `python -c "import sots_stats_hub; import sots_diskstat_gui.workers"`
