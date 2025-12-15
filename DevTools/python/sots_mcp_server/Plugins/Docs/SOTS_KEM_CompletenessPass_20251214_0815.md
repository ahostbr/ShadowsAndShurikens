# SOTS KEM Completeness Pass (2025-12-14 08:15)

## Summary
- Wired execution anchors to optionally inject EnvContextTags and bias toward preferred executions (config-gated, default OFF).
- Added optional auto position-tag classification plus clearer CAS vertical thresholds (backward-compatible defaults).
- Implemented Blueprint CAS binding helper and added preferred-execution/anchor debug logs; filled BP stub so it no longer returns empty contexts.

## New config flags (defaults preserve behavior)
- `bUseAnchorEnvContextTags` (default false) — merge anchor EnvContextTags into request ContextTags.
- `bUseAnchorPreferredExecutions` (default false) + `AnchorPreferredExecutionBonus` (default 5.0) — score bias when anchor prefers a definition.
- `bAutoComputePositionTags` (default false) + `AutoPositionVerticalThreshold` (default 30.f) — auto inject SOTS.KEM.Position.* when none present.
- CAS vertical thresholds: `MinVerticalHeightDelta` (default 15.f), `MaxVerticalHeightDelta` (default 0 = no cap).

## Files touched
- `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_Types.h` — added vertical min/max fields.
- `.../Public/SOTS_KEM_ManagerSubsystem.h` — added config flags, bonus tuning, helper declarations, auto position settings.
- `.../Private/SOTS_KEM_ManagerSubsystem.cpp` — anchor EnvContextTags merge, preferred-execution bias, auto position-tag injection, vertical gating cleanup, debug logs.
- `.../Public/SOTS_KEM_BlueprintLibrary.h` & `.../Private/SOTS_KEM_BlueprintLibrary.cpp` — deprecated stub, added functional CAS binding builder.

## Behavior impact (defaults OFF)
- No gameplay change unless new config flags are enabled; vertical defaults mirror previous thresholds.
- Preferred-execution scoring and anchor EnvContextTags are opt-in; auto position tags disabled unless requested.

## Usage notes
- To use anchor EnvContextTags or preferred executions, enable the corresponding flags on the KEM subsystem config (ini or defaults).
- To auto-classify position tags, set `bAutoComputePositionTags=true` and configure `AutoPositionVerticalThreshold` if needed.
- Use `KEM_BuildCASBindingContextsForDefinition` for CAS bindings; the old function is deprecated but returns valid contexts with tags.
