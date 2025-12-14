# Buddy Worklog — SOTS_Interaction — SPINE 3 (Subsystem + Candidate Selection)

## What changed
- Added `USOTS_InteractionSubsystem` (GameInstanceSubsystem) to select the best interactable candidate per PlayerController.
- Implemented:
  - Throttled update (`UpdateCandidateThrottled`) and immediate update (`UpdateCandidateNow`)
  - Sphere sweep query + filtering by `USOTS_InteractableComponent`
  - LOS gating (optional) and deterministic scoring (distance + facing)
  - Candidate changed event: `OnCandidateChanged`

## Files added
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp

## Notes / Assumptions
- No UI integration; consumers can bind to `OnCandidateChanged`.
- Debug drawing is gated by `bEnableDebug` (compile guards can be added later).

## TODO (next pass)
- Add interaction request flow: press interact -> validate -> execute or request options.
- Add UI intent routing through SOTS_UI (law-compliant) in SPINE 4.