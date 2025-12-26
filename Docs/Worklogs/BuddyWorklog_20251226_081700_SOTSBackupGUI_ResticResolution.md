# Buddy Worklog — SOTS Backup GUI restic resolution

Date: 2025-12-26
Owner: Buddy

## Goal
Reduce friction when launching the SOTS Backup GUI on machines where `restic` is not installed on PATH (or winget is unavailable), and provide clearer instructions for configuring `restic_exe`.

## What changed
- Improved `ResticClient` executable resolution to check a few portable drop locations under the SOTS backup root (where the passfile lives), e.g. `C:\UE5\SOTS_Backup\restic.exe`.
- Improved error text when restic cannot be found, pointing users at the `restic_exe` config field.
- Broadened the shared `sots_backup` CLI restic detection to also accept a portable `C:\UE5\SOTS_Backup\restic.exe` (and a couple common subfolders) for consistency with the GUI.
- Updated user docs to describe the `restic_exe` override and portable option.

## Files changed
- DevTools/python/sots_backup_gui/restic_client.py
- DevTools/python/sots_backup/sots_backup.py
- Docs/SOTS_Backup_GUI.md

## Notes / Risks / Unknowns
- This change does not install restic; it only adds additional places to look for it.
- UNVERIFIED: GUI end-to-end refresh after placing a portable `restic.exe` (no execution performed per rules).

## Verification status
- VERIFIED (static): code paths compile/typecheck (no reported errors from editor diagnostics).
- UNVERIFIED: runtime behavior in GUI/CLI on this machine.

## Next steps (Ryan)
- Either install restic normally (PATH/winget) OR download a portable `restic.exe` and place it at `C:\UE5\SOTS_Backup\restic.exe`.
- Relaunch the GUI and click Refresh; confirm snapshots populate.
- If using a non-standard location, set `restic_exe` in `DevTools/python/sots_backup_gui/gui_config.json` to the full path.
