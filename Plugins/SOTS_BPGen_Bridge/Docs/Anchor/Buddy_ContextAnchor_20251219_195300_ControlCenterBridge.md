[CONTEXT_ANCHOR]
ID: 20251219_195300 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: ControlCenterHosting | Owner: Buddy
Scope: Host BPGen Control Center UI inside bridge plugin; add plugin dependency.

DONE
- Added SSOTS_BPGenControlCenter widget to bridge plugin and registered Control Center tab/menu in the bridge module.
- Bridge Build.cs now includes Slate/ToolMenus; uplugin declares dependency on SOTS_BlueprintGen.
- Bridge Binaries and Intermediate deleted after edits.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Control Center tab loads and operates the bridge server correctly after rebuild; circular dependency resolved.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SSOTS_BPGenControlCenter.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SSOTS_BPGen_BridgeModule.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Public/SOTS_BPGen_BridgeModule.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.Build.cs
- Plugins/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.uplugin
- Plugins/SOTS_BPGen_Bridge/Binaries (deleted)
- Plugins/SOTS_BPGen_Bridge/Intermediate (deleted)

NEXT (Ryan)
- Rebuild ShadowsAndShurikensEditor; confirm no circular dependency warnings and bridge compiles.
- Open Control Center via Window → SOTS Tools; verify start/stop/status/recent/annotate behaviors.

ROLLBACK
- Remove bridge tab/menu registration changes; revert Build.cs/uplugin to prior dependency lists; restore deleted widget files if needed.
