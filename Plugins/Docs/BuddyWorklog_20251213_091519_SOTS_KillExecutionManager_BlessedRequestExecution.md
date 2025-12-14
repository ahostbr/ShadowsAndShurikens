# Prompt ID
- Not provided.

# Goal
- Add a single blessed BlueprintCallable request entrypoint for executions (player/dragon/cutscene) while keeping legacy wrappers working.

# What changed
- Added `RequestExecution_Blessed` on `USOTS_KEMManagerSubsystem` to expose a unified, Sequencer-friendly API with optional context reason and fallback control.
- Routed the new entrypoint through the existing `RequestExecutionInternal` codepath, with a small fallback gate flag; existing wrappers still call their original methods unchanged.
- Extended `RequestExecutionInternal` to accept a fallback toggle (default true) without altering selection logic.

# Files changed
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp

# Notes
- Invalid inputs (missing world context/instigator/target or invalid ExecutionTag) safely return false with a VeryVerbose log.
- ContextReason selects existing context tag presets (player/dragon/cinematic) without new dependencies.
- Existing wrappers (`RequestExecution_FromPlayer`, `RequestExecution_FromDragon`, `RequestExecution_FromCinematic`, `KEM_RequestExecution_Simple`) remain intact and continue to use their prior paths.

# Verification
- Not run (no builds or automated tests).

# Cleanup confirmation
- Deleted Plugins/SOTS_KillExecutionManager/Binaries and Plugins/SOTS_KillExecutionManager/Intermediate.

# Follow-ups
- Consider updating Blueprint helper nodes to surface the new blessed entrypoint and, if desired, add a lightweight KEM request struct for richer BP inputs.
