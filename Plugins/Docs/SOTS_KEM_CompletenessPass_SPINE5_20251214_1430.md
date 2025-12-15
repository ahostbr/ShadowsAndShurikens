# SOTS_KEM Completeness Pass — SPINE 5 (AIS Retirement + LevelSequence Backend)

## Summary
- Retired AIS backend: enum marked deprecated/hidden, evaluation now fails fast with a clear log, and AIS selections abort immediately.
- Implemented LevelSequence backend end-to-end: soft-load sequence, create player/actor, optional binding by tag/name, play, broadcast completion, and clean up.
- Added Blueprint delegate for LevelSequence completion and optional legacy broadcast toggle.

## Key details
- AIS retirement:
  - `ESOTS_KEM_BackendType::AIS` marked deprecated/hidden; validation now errors (`SOTS_KEM_Types.h`, `SOTS_KEM_ExecutionDefinition.cpp`).
  - `EvaluateAISDefinition` always fails with “AIS backend retired” (`SOTS_KEM_ManagerSubsystem.cpp`), and dispatch path aborts with explicit warning.
- LevelSequence backend:
  - Soft reference `FSOTS_KEM_LevelSequenceConfig::SequenceAsset` (ULevelSequence) with playback settings and destroy-on-finish flag (`SOTS_KEM_Types.h`).
  - Runtime flow: `StartLevelSequenceExecution` → optional async load via StreamableManager → `PlayLevelSequenceRun` creates `ULevelSequencePlayer`/`ALevelSequenceActor`, binds instigator/target via `SetBindingByTag`, plays, and hooks finish/stop (`SOTS_KEM_ManagerSubsystem.cpp`).
  - Completion: `OnLevelSequenceFinishedInternal` → `CleanupLevelSequenceRun` stops players, destroys actor if configured, broadcasts `OnKEMLevelSequenceFinished`, and calls `NotifyExecutionEnded` for cooldown/state.
  - Legacy `OnLevelSequenceExecutionChosen` broadcast is gated by `bBroadcastLegacyLevelSequenceChosen` (default false).
- New Blueprint delegate:
  - `OnKEMLevelSequenceFinished` (RequestId, ExecutionTag, Instigator, Target, bSucceeded) on `USOTS_KEMManagerSubsystem`.

## Evidence pointers
- AIS retirement: `ESOTS_KEM_BackendType` metadata (`Public/SOTS_KEM_Types.h`), `EvaluateAISDefinition` fail-fast (`Private/SOTS_KEM_ManagerSubsystem.cpp`), validation error (`Private/SOTS_KEM_ExecutionDefinition.cpp`), dispatch abort in LevelSequence switch (`Private/SOTS_KEM_ManagerSubsystem.cpp`).
- LevelSequence load/play/cleanup: `StartLevelSequenceExecution`, `OnLevelSequenceAssetLoaded`, `PlayLevelSequenceRun`, `OnLevelSequenceFinishedInternal`, `CleanupLevelSequenceRun`, `StopAllActiveLevelSequences` (`Private/SOTS_KEM_ManagerSubsystem.cpp`).
- Config & binding: `FSOTS_KEM_LevelSequenceConfig` (soft ref, binding names, playback settings, destroy flag) (`Public/SOTS_KEM_Types.h`); legacy broadcast toggle `bBroadcastLegacyLevelSequenceChosen` + completion delegate `OnKEMLevelSequenceFinished` (`Public/SOTS_KEM_ManagerSubsystem.h`).

## Behavior impact
- Default gameplay unchanged for CAS/Spawn paths. AIS executions now reject with explicit warnings. LevelSequence executions now auto-play internally; legacy “chosen” broadcast is opt-in to avoid double playback.

## Usage notes
- To handle completion in BP: bind to `OnKEMLevelSequenceFinished` on the KEM subsystem.
- To keep legacy external control, set `bBroadcastLegacyLevelSequenceChosen=true` (config) and continue listening to `OnLevelSequenceExecutionChosen`.
