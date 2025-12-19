# SOTS_KEM Completeness Pass — SPINE 3 (Blueprint API & Dispatch Diagnostics)

## Summary
- Audited Blueprint-callable surface (ManagerSubsystem, BlueprintLibrary, Authoring/Catalog libraries) and ensured no callable remains a silent stub.
- Added dev-only warnings for unbound dispatch delegates (CAS/LevelSequence/AIS) gated by a new config flag.
- Added dev-only failure logging in CAS binding helpers so Blueprint callers see why a request failed instead of silent false.

## Blueprint API audit
- Checked UFUNCTIONs under: `USOTS_KEMManagerSubsystem`, `USOTS_KEM_BlueprintLibrary`, `USOTS_KEMAuthoringLibrary`, `USOTS_KEMCatalogLibrary`, `ASOTS_ExecutionHelperActor`, sandbox controller + debug widgets.
- Deprecated helper `KEM_BuildCASBindingContexts` remains as a thin passthrough; supported path is `KEM_BuildCASBindingContextsForDefinition` (now logs failure reasons in dev).

## Changes
- New config toggle (default false): `bWarnOnUnboundDispatchDelegates` to surface unbound delegate dispatches in dev builds.
- Dev-only warnings for CAS/LevelSequence/AIS dispatch with no listeners bound.
- Dev-only failure logging for `KEM_ResolveWarpPointByName` and `KEM_BuildCASBindingContextsForDefinition` when inputs are missing/invalid.

## Evidence pointers
- Config toggle and usage: `SOTS_KEM_ManagerSubsystem.h` (`bWarnOnUnboundDispatchDelegates`), `SOTS_KEM_ManagerSubsystem.cpp` (`LogUnboundDispatchIfNeeded`, calls before `OnCASExecutionChosen`, `OnLevelSequenceExecutionChosen`, `OnAISExecutionChosen`).
- Failure logging for CAS helpers: `SOTS_KEM_BlueprintLibrary.cpp` (`KEM_ResolveWarpPointByName`, `KEM_BuildCASBindingContextsForDefinition`).

## Behavior impact
- Defaults keep previous gameplay: warnings/logging are dev-only and gated off by default; no selection/scoring behavior changed.
- Deprecated BP helper still available but marked; supported helper now surfaces failure reasons without affecting runtime logic.

## Usage notes
- Enable delegate warnings in config/ini: `bWarnOnUnboundDispatchDelegates=true` under `[/Script/SOTS_KillExecutionManager.SOTS_KEMManagerSubsystem]`.
- Blueprint callers can check log output in dev if CAS binding helpers return false to see which input was missing.
