Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251224_181341 | Plugin: SOTS_BlueprintGen | Pass/Topic: BlueprintCreation | Owner: Buddy
Scope: Add Blueprint asset creation support and expose the action through BPGen/MCP.

DONE
- Added FSOTS_BPGenBlueprintDef/Result types and exposed CreateBlueprintAssetFromDef on the builder so it can emit package-normalized Blueprints via UBlueprintFactory.
- Taught SOTS_BPGenAPI to deserialize create_blueprint_asset payloads and return the new result struct.
- Added bpgen_create_blueprint to sots_mcp_server and ensured the action is treated as a mutation that respects SOTS_ALLOW_APPLY.

VERIFIED
- UNVERIFIED (no build/runtime run yet).

UNVERIFIED / ASSUMPTIONS
- Parent class resolution follows heuristic similar to VibeUE but has not been stress-tested; defaults to Actor when lookup fails.
- Asset creation path relies on the BlueprintFactory/GWarn path; any missing include or env issue is still unknown.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenBuilder.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenAPI.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BuddyWorklog_20251224_181341_BlueprintCreation.md
- Plugins/SOTS_BlueprintGen/Docs/Anchor/Buddy_ContextAnchor_20251224_181341_BlueprintCreation.md

NEXT (Ryan)
- Build the BlueprintGen plugin to ensure the new helper compiles and the new struct definitions are correctly exported.
- Run the MCP tool (`bpgen_create_blueprint`) with SOTS_ALLOW_APPLY=1 to confirm asset creation and result payloads.
- Verify the generated Blueprint can be opened/compiled to ensure the factory saved it cleanly.

ROLLBACK
- Revert the above files (or run `git checkout -- Plugins/SOTS_BlueprintGen/Source/SOTS_BPGenBuilder/* Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenAPI.cpp Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h DevTools/python/sots_mcp_server/server.py`).
