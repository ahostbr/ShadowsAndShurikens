# Buddy Worklog — SOTS_Interaction — SPINE 2 (Core Data Contracts)

## What changed
Added the foundational API surface for SOTS interactions:
- Canonical context payload `FSOTS_InteractionContext`
- Option payload `FSOTS_InteractionOption`
- Smart actor interface `ISOTS_InteractableInterface`
- Marker/config component `USOTS_InteractableComponent`

## Files added
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionTypes.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractableInterface.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractableComponent.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractableComponent.cpp

## Notes / Assumptions
- No subsystem yet; no selection/trace logic in this pass.
- No UI calls. These structs are intended to be used as routed payloads by SOTS_UI later.

## TODO (next pass)
- Add `USOTS_InteractionSubsystem` (GameInstanceSubsystem)
- Candidate selection: trace/filter/score/select
- Candidate changed delegates/events (for UI routing in a later pass)