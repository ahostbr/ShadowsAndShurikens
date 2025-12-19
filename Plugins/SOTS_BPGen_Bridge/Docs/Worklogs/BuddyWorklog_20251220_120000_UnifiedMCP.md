# Unified MCP surface for agents + BPGen (2025-12-20)

## goal
- expose agents and BPGen delegations from DevTools/python/sots_mcp_server/server.py with apply gating via SOTS_ALLOW_APPLY.

## what changed
- added env helpers and shared allow-apply flag to server.py.
- wired lazy SOTS agents orchestrator config plus new MCP tools (agents_run_prompt, agents_server_info).
- added BPGen bridge TCP config helpers and MCP tools (bpgen_call, bpgen_ping) with apply gating and env host/port/timeouts.
- expanded sots_server_info output with allow_apply + agents/BPGen config for clients.

## files changed
- DevTools/python/sots_mcp_server/server.py

## notes / risks / unknowns
- apply stays disabled unless SOTS_ALLOW_APPLY=1; not exercised here.
- BPGen bridge connectivity/auth not validated; requires running UE bridge and optional SOTS_BPGEN_AUTH_TOKEN.
- agents session DB path created under DevTools/python if missing; no runtime prompt executed yet.

## verification
- not run (server not started in this pass)

## follow-ups / next steps
- Ryan: start unified MCP server and call agents_run_prompt in plan mode; confirm apply is blocked when env off and works when on.
- Ryan: call bpgen_ping and a read-only bpgen_call to validate TCP wiring; try a mutating action with/without SOTS_ALLOW_APPLY to confirm gate.
- Ryan: update MCP client configs to point to DevTools/python/sots_mcp_server/server.py if still targeting legacy wrappers.
