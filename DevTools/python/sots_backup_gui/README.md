# SOTS Backup GUI

Quickstart:
- Install deps: `pip install PySide6`
- Run: `python DevTools/python/sots_backup_gui/sots_backup_gui.py`

Logs:
- `C:\UE5\SOTS_Backup\logs\sots_backup_gui_<action>_YYYYMMDD_HHMMSS.log`

Notes:
- GUI config lives in `DevTools/python/sots_backup_gui/gui_config.json`.
- Tag notes from restic (note_* and note_title_*) are displayed; local notes override the table preview when present.
- Saving a note updates the local notes DB and also updates restic tags for that snapshot.
- If a user says "open sots_backup_gui", run the VSCode task `open sots_backup_gui` or the python command above.
