# Buddy Worklog — SOTS Backup GUI one-time restic auto-detect

Date: 2025-12-26
Owner: Buddy

## Goal
When the SOTS Backup GUI fails due to missing restic on PATH, perform a one-time scan of `C:\` to find `restic.exe`, persist the discovered path into `gui_config.json`, and prompt the user to restart.

## What changed
- Added a best-effort `C:\` scan helper that searches for `restic.exe` while skipping common Windows/system/problem paths and bailing out after a time limit.
- Hooked auto-detect into GUI startup:
  - If `restic_autodetect_done` is false/missing AND `restic_exe` is still the default `restic`, run the scan in a worker.
  - If found: write `restic_exe=<full path>` and `restic_autodetect_done=true`, show an info message, and quit so the user can restart.
  - If not found: write `restic_autodetect_done=true` and show a guidance error.
- Updated docs describing the auto-detect behavior.

## Files changed
- DevTools/python/sots_backup_gui/restic_client.py
- DevTools/python/sots_backup_gui/sots_backup_gui.py
- Docs/SOTS_Backup_GUI.md

## Notes / Risks / Unknowns
- The scan is bounded (time + dir cap) and may not find restic if it is installed in an unusual location outside the searched time window.
- Exclusion list is conservative; it may skip some uncommon install locations under excluded system folders.
- UNVERIFIED: end-to-end GUI restart flow after detection (not executed here).

## Verification status
- VERIFIED (static): no editor diagnostics errors in touched python files.
- UNVERIFIED: runtime behavior in GUI on this machine.

## Next steps (Ryan)
- Launch GUI once; if it finds restic.exe it will prompt for restart.
- Relaunch GUI; click Refresh; confirm snapshots populate.
- If not found, set `restic_exe` in `DevTools/python/sots_backup_gui/gui_config.json` to the full path to restic.exe.
