[CONTEXT_ANCHOR]
ID: 20251219_184600 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenFollowupUE57 | Owner: Buddy
Scope: Clear remaining UE5.7 compile errors (graph resolver, inspector statics, spawner API drift, bridge module header gap).

DONE
- Added UFunction template parameter to AddFunctionGraph in GraphResolver.
- Hoisted inspector helper statics to namespace scope; TWeakObjectPtr access uses Get().
- Spawner registry builds keys with const UFunction*, FieldVariant outer, and VarProperty name (no GetVarName).
- Introduced stub bridge server and bridge module headers/impl so ControlCenter compiles.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Bridge server stub is placeholder; real behavior still needed for UI.
- Spawner key formatting assumes VarProperty exists.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenGraphResolver.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenSpawnerRegistry.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGen_BridgeModule.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGen_BridgeModule.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenBridgeServer.h

NEXT (Ryan)
- Re-run UBT for ShadowsAndShurikensEditor.
- If bridge UI needs functionality, replace stub server/module with real implementation.
- Confirm inspector outputs and spawner keys remain correct after refactors.

ROLLBACK
- Revert the listed files; rebuild module to restore previous behavior.
