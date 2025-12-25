Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1300 | Plugin: sots_mcp_server | Pass/Topic: MANAGE_BLUEPRINT_STUBS | Owner: Buddy
Scope: Completed unified MCP manage_blueprint action stubs by mapping to bridge actions

DONE
- Implemented `manage_blueprint` actions: `get_info`, `get_property`, `set_property`, `reparent`, `list_custom_events`, `summarize_event_graph`.
- Added optional parameters to `manage_blueprint` for these actions.
- Mutating actions (`set_property`, `reparent`) are mutation-gated and send `dangerous_ok=true`.

VERIFIED
- (none) No runtime verification performed.

UNVERIFIED / ASSUMPTIONS
- Assumes the BPGen bridge exposes/accepts the new `bp_blueprint_*` action names.

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py

NEXT (Ryan)
- Exercise `manage_blueprint` actions against a known Blueprint asset.
- Validate safe-mode behavior for the mutating actions.

ROLLBACK
- Revert DevTools/python/sots_mcp_server/server.py.
