# SOTS Interaction — SPINE3 Execution Contract

## Summary
- Added explicit execution result contract (enum + report struct) and Blueprint delegate so interaction attempts cannot silently no-op.
- Introduced new `*_WithResult` Blueprint-callable entrypoints that return detailed execution reports while keeping legacy calls intact.
- Execution pipeline now enforces candidate validity, range, LOS, option availability, interface presence, and target consent, with dev-only failure logging.

## New enum/struct
- `ESOTS_InteractionExecuteResult` and `FSOTS_InteractionExecuteReport` defined in `Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionTypes.h`.

## New entrypoints
- `RequestInteraction_WithResult(APlayerController*)`
- `ExecuteInteractionOption_WithResult(APlayerController*, FGameplayTag)`
Legacy `RequestInteraction` / `ExecuteInteractionOption` map onto these for back-compat; signatures unchanged.

## Delegate
- `FSOTS_OnInteractionExecuted OnInteractionExecuted` (BlueprintAssignable) in `SOTS_InteractionSubsystem`; fires once per attempt with `FSOTS_InteractionExecuteReport` for both success and failure.

## Gates → result mapping
- No candidate → `NoCandidate`
- Missing/invalid target/component → `CandidateInvalid` / `MissingInteractableComponent`
- Tag gate fail → `CandidateBlocked`
- Out of range → `CandidateOutOfRange`
- LOS fail → `CandidateNoLineOfSight`
- Missing interface → `MissingInteractableInterface`
- Option missing/blocked → `OptionNotFound` / `OptionNotAvailable`
- Target rejects (CanInteract=false) → `ExecutionRejectedByTarget`
- Otherwise → `Success`

## Debug
- `bDebugLogInteractionExecutionFailures` (default false) logs failure reports in non-shipping builds.

## Evidence pointers
- Enum/struct: `ESOTS_InteractionExecuteResult`, `FSOTS_InteractionExecuteReport` in `SOTS_InteractionTypes.h`.
- Execution functions: `RequestInteraction_WithResult`, `ExecuteInteractionOption_WithResult`, `ExecuteInteractionInternal` in `SOTS_InteractionSubsystem.cpp`.
- Delegate broadcast: `OnInteractionExecuted.Broadcast(...)` inside the new WithResult functions in `SOTS_InteractionSubsystem.cpp`.

## Verification
- Not run (per instructions).

## Cleanup
- Removed `Plugins/SOTS_Interaction/Binaries/` and `Plugins/SOTS_Interaction/Intermediate/` after edits.
