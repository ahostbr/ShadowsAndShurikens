# SOTS DiskStat GUI

Lightweight disk usage explorer (O&O DiskStat style) for SOTS DevTools. Scans a chosen root, visualizes folder sizes, treemap, largest items, and file-type breakdown, with snapshot save/load and safe Explorer actions.

## Install
- Python 3.10+
- Dependencies:
  - `pip install PySide6`
  - Optional: `pip install send2trash` (enables recycle-bin delete; otherwise the Delete button stays disabled)

## Run
From `DevTools/python`:
```bash
python -m sots_diskstat_gui
```

## Features
- Scandir-based recursion (skip symlinks default), max-depth option, cancel button.
- Views: folder tree, treemap, largest items (dirs + capped top files), file-type breakdown.
- Snapshot export/import (`*.json.gz`) with meta (tool/version/root/stats).
- Actions: open in Explorer, copy path. Delete uses Recycle Bin via `send2trash`, gated by an "Enable destructive actions" checkbox + confirmation dialog (off by default).
- Logs pane shows scan progress/errors; clear dependency error if PySide6 missing.
- Duplicates tab: scans current node or full root, defaults to 1 MB min size and quick+full hashing, shows duplicate groups and allows CSV/JSON export.
- Dark theme is default (View > Dark Theme toggle, persisted); treemap responds to clicks (select in tree) and double-click zooms that node.
- Exclusions: enable/disable toggle, presets for Windows and Unreal/SOTS noise (`$Recycle.Bin`, `System Volume Information`, `Windows\WinSxS`, `Temp`, `DerivedDataCache`, `Intermediate`, `Binaries`, `Saved`, `.vs`, `.git`, `BEP_EXPORTS`, `node_modules`), and a reset button. Patterns are case-insensitive substring matches on the full path; defaults are enabled.
- Quiet mode (default on): logs scan start, throttled progress, warnings/errors, and summary; turn it off for verbose per-directory logging.
- Treemap navigation: breadcrumb bar above the treemap plus Back/Forward buttons (next to Up) keep history in sync with the tree/treemap selection.

## Safety defaults
- No destructive operations are enabled unless the checkbox is explicitly checked and `send2trash` is available.
- Symlinks are skipped by default to avoid surprises; errors are counted and logged, but the scan continues.
