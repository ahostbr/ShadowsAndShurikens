# Buddy Worklog — 2025-12-26 00:01:00 — FixMcpCheckServer

goal
- Make the MCP server status checker accurately detect the real `server.py` stdio process (spawned by VS Code), instead of matching its own process.

what changed
- Updated the process-matching heuristic in the checker to:
  - exclude the current PID
  - search for a python process whose cmdline includes `sots_mcp_server\\server.py`
  - report pid + cmdline for the matched server process

files changed
- DevTools/python/sots_mcp_server/check_server.py

notes + risks/unknowns
- Heuristic-based detection: it matches by cmdline content; if the launch shape changes (wrapper scripts, different paths), the checker may need adjustment.

verification status
- VERIFIED (artifact): Ran the checker and it detected an active process running:
  - `...\\.venv_mcp312\\Scripts\\python.exe ...\\DevTools\\python\\sots_mcp_server\\server.py`

follow-ups / next steps
- If VS Code reports connect issues, run the checker again after reconnect to confirm the process respawns.
