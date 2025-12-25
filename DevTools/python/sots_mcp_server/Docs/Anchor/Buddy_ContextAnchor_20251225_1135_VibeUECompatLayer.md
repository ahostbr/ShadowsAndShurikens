Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1135 | Plugin: sots_mcp_server | Pass/Topic: VIBEUE_COMPAT_LAYER | Owner: Buddy
Scope: Add VibeUE-upstream-compatible tool names to the unified SOTS MCP server, backed by BPGen where possible.

DONE
- Added VibeUE tool-name compatibility layer in `DevTools/python/sots_mcp_server/server.py`:
  - Implemented subsets:
    - `manage_blueprint`: create/compile
    - `manage_blueprint_function`: create→bpgen_ensure_function
    - `manage_blueprint_variable`: create→bpgen_ensure_variable
    - `manage_blueprint_node`: discover/list/describe/delete
    - `check_unreal_connection`: BPGen ping + server_info synthesis
    - `get_help`: shim to sots_help
  - Other VibeUE tools/actions return structured "not implemented" errors (scaffold for parity work).
- Added these tool names to `sots_help` tool list.
- Added parity tracking doc: `Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md`.

VERIFIED
- VS Code diagnostics: no syntax/type errors reported for `server.py`.

UNVERIFIED / ASSUMPTIONS
- Tool behavior not validated in Unreal/editor; requires running MCP server + bridge.
- Node create/connect/configure parity can be expressed via GraphSpec apply + edit wrappers.

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md
- DevTools/python/sots_mcp_server/Docs/Worklogs/BuddyWorklog_20251225_113500_VibeUECompatLayer.md

NEXT (Ryan)
- Start the unified MCP server and call `sots_help` to confirm tool presence.
- Run `check_unreal_connection` and confirm BPGen ping results.
- Try a minimal `manage_blueprint(action="create", ...)` with apply enabled, then `manage_blueprint(action="compile")`.
- Approve extending `manage_blueprint_node` with GraphSpec-backed create/connect/configure/move.

ROLLBACK
- Revert `DevTools/python/sots_mcp_server/server.py` and delete the added docs if undesired.
