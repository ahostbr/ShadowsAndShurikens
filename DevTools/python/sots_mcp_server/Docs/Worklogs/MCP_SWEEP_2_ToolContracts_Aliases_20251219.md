## MCP Sweep 2: Tool Contracts & Aliases (2025-12-19)

- Added response envelope helpers `_sots_ok/_sots_err` and applied to wrappers (agents, bpgen, workspace tools, reports, devtools) with request_id logging.
- Added aliases map (e.g., agents_run_prompt -> sots_agents_run, manage_bpgen -> bpgen_call) and exposed via server info.
- Added JSONL logging per tool call (DevTools/python/logs/sots_mcp_server.jsonl) with request_id.
- Added `sots_help` (see server for canonical/alias export) and enhanced `sots_server_info` with gates/roots/aliases.
- Gates preserved: SOTS_ALLOW_APPLY plus new BPGen/DevTools flags referenced; no stdout logging; no tests run.
