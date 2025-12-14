# Buddy Worklog - SOTS_Interaction - PASS 7 (Interaction Driver)

## What changed
- Added `USOTS_InteractionDriverComponent`, a lightweight actor component to drive candidate updates and interaction requests.
- Driver forwards subsystem UI intents via `OnUIIntentForwarded` for SOTS_UI router binding, without taking SOTS_UI dependencies.
- Driver supports auto-update cadence (timer-based), manual update, request/execute helpers for input/UI.

## Files added
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionDriverComponent.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionDriverComponent.cpp

## Files changed
- None

## Notes / Assumptions
- Component caches owning PlayerController; attaches to pawn or controller actors.
- Auto-update uses timer; actual throttling still enforced in subsystem.

## TODO / Next
- Pass 8: standardize fail reasons and intent payload polish.