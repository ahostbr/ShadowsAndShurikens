Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_0817 | Plugin: DevTools_sots_backup_gui | Pass/Topic: ResticResolution | Owner: Buddy
Scope: Backup GUI restic executable resolution + docs

DONE
- Added portable restic.exe fallback resolution under backup root (passfile parent): `restic.exe`, `bin/restic.exe`, `restic/restic.exe`, `tools/restic.exe`.
- Improved missing-restic error message to point at `DevTools/python/sots_backup_gui/gui_config.json` `restic_exe` override.
- Mirrored portable restic detection in `DevTools/python/sots_backup/sots_backup.py` for CLI consistency.
- Updated `Docs/SOTS_Backup_GUI.md` with `restic_exe` and portable install instructions.

VERIFIED
- Static: no editor diagnostics errors in touched python files.

UNVERIFIED / ASSUMPTIONS
- UNVERIFIED: GUI successfully lists snapshots after placing portable `restic.exe` (not executed here).

FILES TOUCHED
- DevTools/python/sots_backup_gui/restic_client.py
- DevTools/python/sots_backup/sots_backup.py
- Docs/SOTS_Backup_GUI.md
- Docs/Worklogs/BuddyWorklog_20251226_081700_SOTSBackupGUI_ResticResolution.md

NEXT (Ryan)
- Install restic or place `C:\UE5\SOTS_Backup\restic.exe`; launch GUI; confirm Refresh populates snapshot list.
- If restic is elsewhere, set `restic_exe` in `DevTools/python/sots_backup_gui/gui_config.json` to the full path and retry.

ROLLBACK
- Revert the touched files listed above.
