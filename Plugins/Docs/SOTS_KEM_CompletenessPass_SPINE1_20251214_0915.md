# SOTS KEM Completeness Pass — SPINE1 (2025-12-14 09:15)

## Summary
- Anchors now optionally inject `EnvContextTags` into evaluation context tags (additive, config-gated).
- Anchors can bias scoring toward `PreferredExecutions` with a configurable bonus (default off).
- Deprecated CAS binding stub now returns tag-filled contexts; new helper mirrors runtime binding.

## Config flags (defaults OFF)
- `bUseAnchorEnvContextTags=false`
- `bUseAnchorPreferredExecutions=false`
- `AnchorPreferredExecutionBonus=25000.0` (used only if the flag is on)
- (Existing) `bAutoComputePositionTags` remains default false; untouched behavior.

## Files changed
- `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h`
- `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp`
- `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_Types.h`
- `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_BlueprintLibrary.h`
- `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_BlueprintLibrary.cpp`

## Evidence pointers
- EnvContextTags merged into candidate context when enabled: `SOTS_KEM_ManagerSubsystem.cpp` — function `ApplyAnchorEnvContextTags` and usage in request loop.
- Preferred execution bias applied post-validation: `SOTS_KEM_ManagerSubsystem.cpp` — functions `ComputePreferredExecutionBonus` and main request loop scoring.
- CAS binding helper now returns contexts instead of stub: `SOTS_KEM_BlueprintLibrary.cpp` — `KEM_BuildCASBindingContexts` and new `KEM_BuildCASBindingContextsForDefinition`.
- Vertical thresholds clarified: `SOTS_KEM_Types.h` (MinVerticalHeightDelta/MaxVerticalHeightDelta) with gating in `SOTS_KEM_ManagerSubsystem.cpp` height checks.

## Behavior impact
- Defaults keep prior gameplay: all new features are disabled until flags are set; vertical defaults mirror previous values.

## Usage
- Enable anchor EnvContextTags via `bUseAnchorEnvContextTags=true` to have anchor tags merged into evaluation tags.
- Enable preferred bias via `bUseAnchorPreferredExecutions=true` and adjust `AnchorPreferredExecutionBonus` if needed.
- Use `KEM_BuildCASBindingContextsForDefinition` for CAS bindings; the old function is deprecated but now returns valid contexts.
