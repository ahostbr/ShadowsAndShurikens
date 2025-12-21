[CONTEXT_ANCHOR]
ID: 20251219_195320 | Plugin: SOTS_BlueprintGen | Pass/Topic: BridgeDecouple | Owner: Buddy
Scope: Remove BPGen Control Center and bridge dependency from BlueprintGen.

DONE
- Removed Control Center tab/menu registration from BlueprintGen module; deleted Control Center widget files.
- Removed SOTS_BPGen_Bridge from BlueprintGen Build.cs and uplugin dependencies.
- Deleted BlueprintGen Binaries and Intermediate after edits.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- BlueprintGen now builds without depending on SOTS_BPGen_Bridge; Control Center is provided by the bridge plugin.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BlueprintGen.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BlueprintGen.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/SOTS_BlueprintGen.Build.cs
- Plugins/SOTS_BlueprintGen/SOTS_BlueprintGen.uplugin
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SSOTS_BPGenControlCenter.cpp (deleted)
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SSOTS_BPGenControlCenter.h (deleted)
- Plugins/SOTS_BlueprintGen/Binaries (deleted)
- Plugins/SOTS_BlueprintGen/Intermediate (deleted)

NEXT (Ryan)
- Rebuild ShadowsAndShurikensEditor; confirm circular dependency warning is resolved and BlueprintGen compiles.
- Use Window → SOTS Tools → SOTS BPGen Control Center (from bridge plugin) to verify parity.

ROLLBACK
- Restore Control Center files and tab/menu registration; re-add SOTS_BPGen_Bridge to Build.cs and uplugin.
