# MCP_SPINE_A_AgentsTools (2025-12-19)

- Files: `server.py`, `../sots_agents_runner.py` (library entry), new log dir usage.
- Added MCP tools on primary server: `sots_agents_health`, `sots_agents_run`, `sots_agents_help` (read-only).
- Added `sots_agents_run_task` for library calls; keeps CLI behavior intact.
- Logging: server + runner now write to `DevTools/python/logs` (no stdout).
- Import guard: Agents SDK referenced as `agents` (no `openai_agents`).
