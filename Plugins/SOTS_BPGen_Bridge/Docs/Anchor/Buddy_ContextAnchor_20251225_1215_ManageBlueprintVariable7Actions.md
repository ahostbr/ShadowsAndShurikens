Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1215 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: manage_blueprint_variable_7_actions | Owner: Buddy
Scope: Adds bp_variable_* bridge ops and wires unified MCP manage_blueprint_variable to full 7-action parity.

DONE
- Added bp_variable_* bridge ops: search_types/create/delete/list/get_info/get_property/set_property.
- Added action dispatch + dangerous-action gating for bp_variable_create/bp_variable_delete/bp_variable_set_property.
- Updated unified MCP manage_blueprint_variable to accept VibeUE-style args (variable_config/type_path, list_criteria, search_criteria, etc.) and route to bp_variable_*.

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- search_types is a best-effort scan of loaded UClass paths + primitives; may not match VibeUE category filters.
- type_path→pin mapping is minimal (primitives + object refs); struct/enum/container cases may need follow-up.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeVariableOps.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeVariableOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- DevTools/python/sots_mcp_server/server.py

NEXT (Ryan)
- Run blueprint variable test prompts and confirm create/list/get/set/delete behave in-editor.
- Validate search_types finds /Script/UMG.UserWidget and create works with that type_path.
- If struct/enum/container types are needed, extend type_path→pin mapping and search_types output.

ROLLBACK
- Revert the files above.
