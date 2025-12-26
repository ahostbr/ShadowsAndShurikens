# Buddy Worklog - SOTS_Input BRIDGE1 - 2025-12-25 22:19:15

## Prompt
SOTS_Core BRIDGE1 - SOTS_Input lifecycle listener (Core bridge, defaults OFF).

## Goal
Add an optional, disabled-by-default Core lifecycle bridge so SOTS_Input can respond to PC BeginPlay and Pawn Possessed using existing init seams, without changing behavior unless enabled.

## RepoIntel / evidence
- SOTS_Core lifecycle listener interface + feature name:
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListener.h:14`
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListener.h:16`
- SOTS_Input init seams:
  - `USOTS_InputRouterComponent::BeginPlay` — `Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputRouterComponent.cpp:32`
  - `USOTS_InputRouterComponent::InitializeRouter` — `Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputRouterComponent.cpp:73`
  - `USOTS_InputBlueprintLibrary::EnsureRouterOnPlayerController` — `Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBlueprintLibrary.cpp:164`

## What changed
- Added SOTS_Input core bridge settings (default OFF) for lifecycle bridge + verbose logs.
- Registered a Core lifecycle listener in the SOTS_Input module that:
  - Ensures the router on PC BeginPlay.
  - Refreshes router bindings on Pawn Possessed.
  - Early-outs unless bridge is enabled; guards duplicate PC/Pawn events.
- Declared SOTS_Core as a private module dependency for SOTS_Input.
- Documented enablement and behavior in a new bridge doc.

## Files touched
- `Plugins/SOTS_Input/Source/SOTS_Input/SOTS_Input.Build.cs`
- `Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputCoreBridgeSettings.h`
- `Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputCoreBridgeSettings.cpp`
- `Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputModule.cpp`
- `Plugins/SOTS_Input/Docs/SOTS_Input_SOTSCoreBridge.md`
- `Plugins/SOTS_Input/Docs/Worklogs/BuddyWorklog_20251225_221915_SOTS_Input_BRIDGE1.md`

## Notes / decisions
- Listener is registered at module startup; callbacks are inert unless `bEnableSOTSCoreLifecycleBridge` is true.
- Uses existing SOTS_Input seams (EnsureRouter + RefreshRouter) only; no input mode changes.
- Duplicate guards prevent re-initializing the same PC/Pawn pair.

## Verification
- Not run (per instructions).

## Cleanup
- Attempted to delete `Plugins/SOTS_Input/Binaries` and `Plugins/SOTS_Input/Intermediate`; removal failed due to locked files (likely `UnrealEditor-SOTS_Input.*`).

## Follow-ups
- Enable `SOTS_Core` listener dispatch + bridge setting in a controlled test map and confirm Core diagnostics.
