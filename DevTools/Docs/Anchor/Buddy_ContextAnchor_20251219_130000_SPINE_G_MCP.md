[CONTEXT_ANCHOR]
ID: 20251219_1300 | Plugin: DevTools | Pass/Topic: SPINE_G_MCP | Owner: Buddy
Scope: Added FastMCP server surface to drive BPGen via the bridge.

DONE
- Added DevTools/python/sots_bpgen_mcp_server.py with manage_bpgen + wrapper tools forwarding to the TCP bridge using sots_bpgen_bridge_client.
- Added BPGen prompt pack topics/examples under DevTools/prompts/BPGen to guide the discovery → apply → verify loop.
- Added ensure_function / ensure_variable MCP wrappers (bridge-dependent).
- Worklog recorded under DevTools/Docs/Worklogs for SPINE_G MCP server and prompt pack.

VERIFIED
- None (server not executed; bridge not contacted).

UNVERIFIED / ASSUMPTIONS
- FastMCP import path/versions align with workspace (script exits with install hint if missing).
- Bridge host/port/timeouts are correct for current configuration.
- Bridge currently may not expose ensure_*; wrappers will relay bridge errors until implemented UE-side.

FILES TOUCHED
- DevTools/python/sots_bpgen_mcp_server.py
- DevTools/Docs/Worklogs/BPGen_SPINE_G_MCP_Server_20251219_130000.md
- DevTools/Docs/Anchor/Buddy_ContextAnchor_20251219_130000_SPINE_G_MCP.md
- DevTools/prompts/BPGen/README.md
- DevTools/prompts/BPGen/topics/overview.md
- DevTools/prompts/BPGen/topics/blueprint-workflow.md
- DevTools/prompts/BPGen/topics/recovery-and-verification.md
- DevTools/prompts/BPGen/examples/01_print_string_function.md
- DevTools/prompts/BPGen/examples/02_ensure_var_set_branch.md
- DevTools/prompts/BPGen/examples/03_widget_bind_text.md

NEXT (Ryan)
- Run sots_bpgen_mcp_server.py against the BPGen bridge; confirm manage_bpgen + wrappers return expected responses.
- Extend MCP wrappers when new bridge actions appear (e.g., ensure_*); consider error-code passthrough.
- Consider adding ensure_* wrappers or error-code passthrough once UE actions are in place.

ROLLBACK
- Delete DevTools/python/sots_bpgen_mcp_server.py and remove the related worklog/anchor files.
