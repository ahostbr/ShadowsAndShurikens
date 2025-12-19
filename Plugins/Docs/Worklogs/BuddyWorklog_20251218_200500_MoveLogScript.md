# Buddy Worklog — Move Plugin Logs Script (2025-12-18 20:05)

## Goal
- Automate the relocation of plugin-specific documentation worklogs from `Plugins/Docs` into their owning plugin `Docs/Worklogs/` directories.

## Changes
- Added `DevTools/python/move_plugin_logs.py`, which builds alias maps for each plugin (including manual overrides for shorthand names and SPINE abbreviations) and moves any markdown whose filename matches one of the generated aliases into the matching plugin’s `Docs/Worklogs/` folder.
- Executed the script via the configured `.venv_mcp` interpreter; it relocated dozens of SOTS worklogs (SOTS_UI, SOTS_AIPerception, SOTS_FX_Plugin, SOTS_GAS_Plugin, SOTS_GlobalStealthManager, BEP, etc.) and flagged a couple of general logs that still lack clear ownership.

## Files changed
- DevTools/python/move_plugin_logs.py
- Various relocated markdown logs now under their plugin `Docs/Worklogs/` folders (SOTS_UI, SOTS_AIPerception, SOTS_FX_Plugin, SOTS_GAS_Plugin, SOTS_GlobalStealthManager, SOTS_KillExecutionManager, SOTS_Stats, BEP, SOTS_ProfileShared, SOTS_MissionDirector, SOTS_GSM, etc.)

## Verification
- Script run using `runpy.run_path(...move_plugin_logs.py...)`; output lists each moved file and the two remaining unclaimed logs.

## Follow-ups
- Decide which plugin should own the remaining `BuddyWorklog_20241215_120000_CompileFixes.md` and `BuddyWorklog_20251213_164500_Suite_ShippingSafety_Audit.md`, then move them manually.
