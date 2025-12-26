Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1400 | Plugin: SOTS_Core (+ SOTS_BPGen_Bridge) | Pass/Topic: UE57_NextCompilePass | Owner: Buddy
Scope: Fix next UE 5.7 compile blockers from log (macros, automation flags, modular features, includes, missing header)

DONE
- Replaced `UE_UNUSED(...)` uses with `(void)Var;` in core console commands and tests.
- Fixed `SOTS_CoreDiagnostics.cpp` broken `UE_LOG` call (stray statement in argument list) and added missing includes for gameplay framework classes.
- Updated ModularFeatures calls to `GetModularFeatureImplementations<IModularFeature>(...)` for UE 5.7.
- Updated automation tests to use `EAutomationTestFlags::EditorContext` instead of missing `ApplicationContextMask`.
- Removed missing include `AssetRegistry/AssetRegistryTypes.h` from BPGen bridge asset ops.

VERIFIED
- UNVERIFIED (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Assumed UE 5.7 supports the templated `GetModularFeatureImplementations<T>(...)` overload (per header signature in build log).

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/Private/SOTS_Core.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Diagnostics/SOTS_CoreDiagnostics.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Save/SOTS_CoreSaveParticipantRegistry.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Tests/SOTS_CoreLifecycleTests.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeAssetOps.cpp

NEXT (Ryan)
- Rebuild in UE 5.7.1; confirm `SOTS_Core` + `SOTS_BPGen_Bridge` compile.

ROLLBACK
- Revert the files listed above.
