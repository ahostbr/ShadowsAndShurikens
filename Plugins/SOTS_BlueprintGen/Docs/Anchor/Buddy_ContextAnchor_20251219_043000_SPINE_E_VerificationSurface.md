[CONTEXT_ANCHOR]
ID: 20251219_0430 | Plugin: SOTS_BlueprintGen | Pass/Topic: SPINE_E_VerificationSurface | Owner: Buddy
Scope: Inspection + maintenance helpers (list/describe nodes, compile/save/refresh).

DONE
- Added inspector library (ListFunctionGraphNodes, DescribeNodeById, CompileBlueprintAsset, SaveBlueprintAsset, RefreshAllNodesInFunction)
- Expanded BPGen types with node summary/description/link/default structs and maintenance result nodes output

VERIFIED
- None (no build/editor run)

UNVERIFIED / ASSUMPTIONS
- NodeId stored in NodeComment will not conflict with authored comments
- Refresh helper reconstructions are sufficient without additional cleanup steps

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenInspector.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BPGen_SPINE_E_VerificationSurface_20251219_042900.md

NEXT (Ryan)
- Call DescribeNodeById/ListFunctionGraphNodes on a sample graph to verify NodeId/NodeGuid coverage and pin/link reporting
- Compile/Save helpers on a Blueprint to confirm no warnings emitted
- RefreshAllNodesInFunction on a function with recent variable changes and confirm pins reconstruct correctly

ROLLBACK
- Revert the touched files listed above
