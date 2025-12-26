Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_0837 | Plugin: DevTools_sots_backup_gui | Pass/Topic: DarkTheme | Owner: Buddy
Scope: Apply Stats Hub dark theme to Backup GUI

DONE
- Added `apply_dark_theme()` (palette + QSS) to the Backup GUI, matching `DevTools/python/sots_stats_hub/stats_hub_gui.py`.
- Called `apply_dark_theme(window)` during Backup GUI startup.

VERIFIED
- Static: no editor diagnostics errors in `sots_backup_gui.py`.

UNVERIFIED / ASSUMPTIONS
- UNVERIFIED: runtime visual parity (GUI not launched here).

FILES TOUCHED
- DevTools/python/sots_backup_gui/sots_backup_gui.py
- DevTools/python/Docs/Worklogs/BuddyWorklog_20251226_0837_BackupGUI_DarkTheme.md

NEXT (Ryan)
- Launch Backup GUI; confirm dark theme applies across table/details/log pane.

ROLLBACK
- Revert the files listed above.
