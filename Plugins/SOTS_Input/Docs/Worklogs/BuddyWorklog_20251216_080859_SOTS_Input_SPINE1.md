# Buddy Worklog — SOTS_Input SPINE_1 — 2025-12-16 08:08:59

## Goal
Implement SOTS_Input SPINE_1 runtime pieces: router component, buffering hooks, anim notify buffer window, and docs without touching GAS or UI input modes.

## What changed
- Added `USOTS_InputRouterComponent` to manage layer stack, mapping contexts, and handler dispatch with buffering-aware routing.
- Added `UAnimNotifyState_SOTS_InputBufferWindow` to open/close buffer channels during animations.
- Documented plugin usage and component roles in `SOTS_Input_Overview.md`.

## Files touched
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputRouterComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputRouterComponent.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/Animation/AnimNotifyState_SOTS_InputBufferWindow.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Animation/AnimNotifyState_SOTS_InputBufferWindow.cpp
- Plugins/SOTS_Input/Docs/SOTS_Input_Overview.md

## Notes / rationale
- Router duplicates handler templates per layer to keep layer assets immutable and calls activation hooks.
- Binding only declared trigger events reduces redundant Enhanced Input bindings.
- Buffered events flush back through the router so handler logic stays centralized.

## Verification
- Build/tests not run (per instructions).

## Cleanup
- Deleted Plugins/SOTS_Input/Binaries and Plugins/SOTS_Input/Intermediate after edits.

## Follow-ups
- Wire actual layer assets/handlers in content and confirm tag registration in TagManager.
- Add unit/automation coverage if desired once runtime wiring is in place.
