goal
- Add Blueprint creation support to BPGen so DevTools can bootstrap new assets the way VibeUE can.

what changed
- Introduced FSOTS_BPGenBlueprintDef/Result and a new builder helper that normalizes paths, resolves parents, and saves a UBlueprint via UBlueprintFactory.
- Wired BPGenAPI to accept the new create_blueprint_asset action and added a dedicated MCP tool with mutation gating for DevTools.

files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenBuilder.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenAPI.cpp
- DevTools/python/sots_mcp_server/server.py

notes + risks/unknowns
- Blueprint creation defaults to Actor when the requested parent class cannot be resolved; no overwrite flow is implemented yet.
- Changes not built/validated yet; parsing errors or asset registry issues might remain.

verification status
- UNVERIFIED (not run)

follow-ups / next steps
- Build the BlueprintGen plugin and confirm the new helper compiles.
- Trigger bpgen_create_blueprint via the MCP tool to ensure the asset is created and the action returns the expected result.
