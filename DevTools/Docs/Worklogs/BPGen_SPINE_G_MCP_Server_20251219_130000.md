# BPGen_SPINE_G_MCP_Server_20251219_130000

goal
- Add a FastMCP server surface for BPGen (SPINE_G) that forwards tool calls to the local TCP bridge.

what changed
- Added DevTools/python/sots_bpgen_mcp_server.py exposing manage_bpgen plus thin wrappers (discover/apply/list/describe/compile/save/refresh) using the bridge client.
- Added ensure_function / ensure_variable MCP wrappers that forward to bridge actions if present.
- Added BPGen prompt pack (topics + examples) under DevTools/prompts/BPGen to document the discovery → apply → verify loop.

files changed
- DevTools/python/sots_bpgen_mcp_server.py
- DevTools/prompts/BPGen/README.md
- DevTools/prompts/BPGen/topics/overview.md
- DevTools/prompts/BPGen/topics/blueprint-workflow.md
- DevTools/prompts/BPGen/topics/recovery-and-verification.md
- DevTools/prompts/BPGen/examples/01_print_string_function.md
- DevTools/prompts/BPGen/examples/02_ensure_var_set_branch.md
- DevTools/prompts/BPGen/examples/03_widget_bind_text.md

notes + risks/unknowns
- FastMCP dependency may not be installed; script exits with an install hint if missing.
- Prompt pack assumes current bridge/MCP action names; ensure_* wrappers will return bridge errors until the bridge exposes those actions.
- No runtime verification performed; socket/bridge connectivity untested in this pass.

verification status
- UNVERIFIED: server not launched; no bridge calls executed.

follow-ups / next steps
- Extend MCP wrappers when bridge exposes ensure_* or additional actions; consider error code passthrough.
- Run against live bridge to confirm request/response flow and adjust defaults (timeouts/host/port) if needed.
