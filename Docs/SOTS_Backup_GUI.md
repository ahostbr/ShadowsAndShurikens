# SOTS Backup GUI

## What it does
The SOTS Backup GUI is a simple desktop app that lists restic snapshots, lets you add local notes, and supports restore, forget, and prune operations while streaming logs in-app.

## Requirements
- Python 3.12
- PySide6 (`pip install PySide6`)
- Restic installed (either on PATH, or configured via `restic_exe`)
- Restic passfile exists at `C:\UE5\SOTS_Backup\.restic_pass.txt`

## Fixing "restic not found" in the GUI
The GUI reads `DevTools/python/sots_backup_gui/gui_config.json`.

On first launch only, if restic is not found, the GUI will do a best-effort scan of `C:\\` (skipping common Windows system paths), then write the discovered full path into `restic_exe` and ask you to restart.

If restic is not on PATH, set `restic_exe` to a full path, for example:
```json
{
	"restic_exe": "C:/UE5/SOTS_Backup/restic.exe"
}
```

Portable option (no PATH / no winget): place a `restic.exe` at `C:\UE5\SOTS_Backup\restic.exe`.

If you need to re-run auto-detection, set `restic_autodetect_done` back to `false` (or remove it) in `gui_config.json`.

## Install
```
pip install PySide6
```

## Run
```
python DevTools/python/sots_backup_gui/sots_backup_gui.py
```

Optional VSCode task:
- Task label: `open sots_backup_gui`

## Notes behavior
Notes are stored locally only in `C:\UE5\SOTS_Backup\snapshot_notes.json`. They are keyed by snapshot id and are not written into restic metadata.
The GUI shows restic note tags (`note_*` and `note_title_*`) in the Tag Note field; the table preview uses the local note if present, otherwise it falls back to the tag note.
Saving a note updates the local JSON and updates the restic tags for the snapshot.

## Restore workflow + warnings
1) Select a snapshot.
2) Click Restore and choose a target folder.
3) Restic restores files into that folder.

Warning: Restore can overwrite files in the target directory. Pick an empty or dedicated folder.

## Delete/Forget vs Prune
- Forget: removes snapshot references from the repository (fast, safer default).
- Prune: removes unreferenced data from the repository (slower, reclaims space).
Run prune only when you explicitly want cleanup; it is separate from forget in the GUI.
