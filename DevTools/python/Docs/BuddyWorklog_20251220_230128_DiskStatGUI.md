# Buddy Worklog
- goal: add duplicate finder (quick/full hashing) to DiskStat GUI with UI tab, exports, and safe controls.
- changes: new duplicate scanner module + thread worker; Duplicates tab with target/range/hash options, table, exports; README note.
- files: sots_diskstat_gui/duplicates.py; sots_diskstat_gui/diskstat_gui.py; sots_diskstat_gui/README.md.
- run: `& "E:\SAS\ShadowsAndShurikens\.venv_mcp\Scripts\python.exe" -m sots_diskstat_gui`.
- verification: import-only (PySide6 already installed) and UI wiring by inspection; duplicate scan not executed here.
- follow-ups: manual duplicate scan on a medium folder; verify cancel/exports; consider tuning quick hash bytes if collisions observed.
