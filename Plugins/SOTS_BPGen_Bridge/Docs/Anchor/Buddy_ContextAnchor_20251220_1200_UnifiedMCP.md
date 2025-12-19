[CONTEXT_ANCHOR]
ID: 20251220_1200 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: Unified_MCP_Delegates | Owner: Buddy
Scope: Unified MCP server.py exposes agents + BPGen tools with SOTS_ALLOW_APPLY gating.

DONE
- Added env-gated agents_run_prompt and agents_server_info MCP tools in DevTools/python/sots_mcp_server/server.py
- Added BPGen bridge delegates (bpgen_call, bpgen_ping) using env host/port/timeouts with apply gating
- sots_server_info now surfaces allow_apply plus agents/BPGen config for MCP clients

VERIFIED
- None (not run)

UNVERIFIED / ASSUMPTIONS
- BPGen bridge responds to ping/call via new MCP entrypoint
- Agents orchestrator session DB path works under MCP host

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py

NEXT (Ryan)
- Launch unified MCP server and run agents_run_prompt (plan + apply with env enabled) to confirm orchestration and gating
- Call bpgen_ping and a read-only bpgen_call to validate TCP wiring; try a mutating action with SOTS_ALLOW_APPLY=0 then 1 to confirm block/pass-through
- Point MCP clients at DevTools/python/sots_mcp_server/server.py if they still use legacy wrappers

ROLLBACK
- Restore DevTools/python/sots_mcp_server/server.py to the previous revision
