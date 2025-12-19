[CONTEXT_ANCHOR]
ID: 20251219_2015 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenCompileFollowup | Owner: Buddy
Scope: Address UE5.7 compile errors from 13:22 log (WidgetBlueprint include, discovery API, spawn helper).

DONE
- Added UMG/UMGEditor dependencies to Build.cs for WidgetBlueprint usage.
- Updated discovery to UE5.7 APIs: PrimeDefaultUiSpec, schema class assignment, variable spawner accessors, K2 var node includes, schema-based pin conversion.
- Removed NodeCommentColor usage in debug annotations (field no longer exists).
- Implemented SpawnFunctionEntryNode/SpawnFunctionResultNode helpers via FGraphNodeCreator.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Expect BPGen to compile past prior C1083/C2039/C2129; additional API differences may surface on next UBT.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/SOTS_BlueprintGen.Build.cs
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDiscovery.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDebug.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

NEXT (Ryan)
- Re-run UBT for editor; capture new log if further errors appear.
- If BPGen binaries/intermediate reappear, delete once per plugin per SOTS law.
- Verify BPGen flows after successful build.

ROLLBACK
- Revert the touched source files and Build.cs changes.
