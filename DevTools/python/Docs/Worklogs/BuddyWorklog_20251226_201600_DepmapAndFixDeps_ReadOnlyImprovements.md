# Buddy Worklog — 2025-12-26 20:16:00 — Depmap + FixDeps Read-Only Improvements

## Goal
- Tighten `sots_plugin_depmap.py` so it only scans the project plugin folder (and only `SOTS*` plugins) and writes stable, diff-friendly JSON.
- Broaden `fix_plugin_dependencies.py` matching to include common UBT missing dependency warning strings (still read-only).

## What changed
### 1) `sots_plugin_depmap.py`
- Scan scope is now limited to `<ProjectRoot>/Plugins/` and only plugin directories whose names start with `SOTS`.
- `.uplugin` discovery is now per-plugin-dir (prefers `<PluginName>.uplugin`, otherwise first `*.uplugin` in that plugin directory).
- Output JSON is now stable:
  - `sort_keys=True`
  - dependency names are de-duped and sorted
  - module entries are sorted by module name
  - `Path` is stored as a project-root-relative POSIX path when possible

### 2) `fix_plugin_dependencies.py`
- Expanded the line filter to include common UnrealBuildTool / editor log dependency-warning substrings (e.g., “does not list plugin … as a dependency”, “Please add it to …”, and several related module-load/missing-manifest strings).
- No automatic fixes were added; it remains a log summarizer.

## Files changed
- DevTools/python/sots_plugin_depmap.py
- DevTools/python/fix_plugin_dependencies.py

## Verification
- Ran `python -m py_compile` on both scripts (syntax-only check): OK.

## Notes / Risks / Unknowns
- `sots_plugin_depmap.py` now expects `--root` to be the *project root* containing `Plugins/`; it will exit with an error if `Plugins/` is missing.
- The depmap tool still only reflects `.uplugin` declared plugin dependencies; it does not inspect module-level deps in `*.Build.cs`.
