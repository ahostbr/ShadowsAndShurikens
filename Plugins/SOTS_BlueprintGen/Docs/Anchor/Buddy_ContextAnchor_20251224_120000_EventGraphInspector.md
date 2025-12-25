Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251224_120000 | Plugin: SOTS_BlueprintGen | Pass/Topic: EventGraphResolver | Owner: Buddy
Scope: Ensure the BPGen inspector can resolve EventGraph/Ubergraph targets via the graph resolver so MCP can inspect player blueprints.

DONE
- Added an include for `SOTS_BPGenGraphResolver` and a resolver call in `Inspector_FindFunctionGraph` so graphs missing from `FunctionGraphs` are resolved through `ResolveEventGraph` (Ubergraph-domain).
- Recorded the change in the matching worklog and noted the remaining verification steps.

VERIFIED
- None (code not built or exercised yet).

UNVERIFIED / ASSUMPTIONS
- The resolver path will return the same graph objects MCP expects once the plugin is rebuilt; no runtime evidence yet.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BuddyWorklog_20251224_120000_EventGraphInspectorFollowup.md
- Plugins/SOTS_BlueprintGen/Docs/Anchor/Buddy_ContextAnchor_20251224_120000_EventGraphInspector.md

NEXT (Ryan)
- Rebuild SOTS_BlueprintGen and rerun the MCP `list_nodes` request with `function_name=EventGraph`; success looks like node/pin data returning instead of the prior "graph not found" error.
- Verify the descriptor metadata from the resolver continues to match the `discover_nodes` output so the BPGen bridge can safely reference those nodes when spawning Blueprint graphs.

ROLLBACK
- Revert the updated `SOTS_BPGenInspector.cpp` and remove the anchor/worklog if this resolver change causes issues.
