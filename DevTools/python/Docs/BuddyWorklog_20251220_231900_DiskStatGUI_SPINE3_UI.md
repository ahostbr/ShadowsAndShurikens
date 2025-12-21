# Buddy Worklog
- goal: fix FileEntry comparison error and add dark-themed, nicer treemap/duplicates UI polish.
- changes: stabilized top-file heap sorting; dark theme default with View toggle + QSettings persistence; treemap fit/gutters/labels and click-to-select/zoom behavior; README updated.
- files: sots_diskstat_gui/scanner.py; sots_diskstat_gui/treemap.py; sots_diskstat_gui/diskstat_gui.py; sots_diskstat_gui/README.md.
- verification: import-only sanity (no runtime scan here); theme/treemap logic reviewed by inspection.
- follow-ups: run GUI, verify treemap resize/click/zoom, confirm dark toggle persistence, and check duplicate scans on a venv for no TypeError logs.
