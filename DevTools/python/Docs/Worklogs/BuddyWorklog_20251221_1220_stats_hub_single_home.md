# Buddy Worklog — Stats Hub single home (2025-12-21)

- Backed up duplicate implementations to `DevTools/python/_legacy/dup_backups/20251221_121148/` (nested pkg + MCP-embedded copy + embedded launcher).
- Converted nested `DevTools/python/sots_stats_hub/sots_stats_hub` into shims that re-export the canonical package (with deprecation banner).
- Converted embedded `DevTools/python/sots_mcp_server/DevTools/python/sots_stats_hub` into path-injecting shims that redirect to the canonical package; embedded launcher now redirects too.
- Updated canonical README run instructions; ensured root `.gitignore` ignores `.venv`, `__pycache__`, `*.pyc`; maintained launcher parity (pythonw detached).

## Verify (commands only; no pasted output)
- `python -m sots_stats_hub`
- `python run_sots_stats_hub.py` (from DevTools/python)
- `python -c "import sots_stats_hub; import sots_stats_hub.stats_hub_gui"`
- `python -c "import sots_mcp_server"`
- `rg \"stats_hub_gui.py\" DevTools/python | findstr /C:\"sots_stats_hub\"`
