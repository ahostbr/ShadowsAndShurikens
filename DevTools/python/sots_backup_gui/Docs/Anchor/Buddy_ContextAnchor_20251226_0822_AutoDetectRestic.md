Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_0822 | Plugin: DevTools_sots_backup_gui | Pass/Topic: AutoDetectRestic | Owner: Buddy
Scope: One-time restic.exe scan + config persistence on GUI startup

DONE
- Added `scan_for_restic_exe_on_c_drive()` with bounded scan and excluded Windows/system paths.
- Added `ResticClient.try_resolve_restic_exe()` to allow non-throwing resolution checks.
- GUI startup now runs `_startup()` which:
  - If `restic_autodetect_done` is false/missing and `restic_exe` is default, runs `autodetect_restic` worker.
  - On success: writes `restic_exe=<full path>` + `restic_autodetect_done=true` to `gui_config.json`, prompts restart, quits.
  - On failure: writes `restic_autodetect_done=true` and shows guidance.
- Docs updated to describe one-time scan and how to re-run by clearing `restic_autodetect_done`.

VERIFIED
- Static: no editor diagnostics errors in touched files.

UNVERIFIED / ASSUMPTIONS
- UNVERIFIED: scan duration is acceptable on target machines; bounded by 120s default.
- UNVERIFIED: restart flow works as expected in practice.

FILES TOUCHED
- DevTools/python/sots_backup_gui/restic_client.py
- DevTools/python/sots_backup_gui/sots_backup_gui.py
- Docs/SOTS_Backup_GUI.md
- Docs/Worklogs/BuddyWorklog_20251226_082200_SOTSBackupGUI_AutoDetectRestic.md

NEXT (Ryan)
- Run GUI once with restic missing; confirm it scans and persists `restic_exe` if found.
- Relaunch GUI; confirm Refresh lists snapshots.
- If scan misses, set `restic_exe` manually to full path and retry.

ROLLBACK
- Revert the touched files listed above.
