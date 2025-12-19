# Worklog — SOTS_Interaction SPINE4 Action Seam (2025-12-18 23:12)

## Goal
Add a decoupled action request seam so interaction verbs like Pickup/Execute/Drag can be routed cross-plugin without direct dependencies or UI changes.

## Changes
- Added `FSOTS_InteractionActionRequest` payload struct (instigator/target/verb/option index/item tag/quantity/context tags/LOS/distance).
- Added subsystem multicast `OnInteractionActionRequested` and helper detection/build functions for canonical verbs.
- Short-circuited `ExecuteInteractionInternal` for canonical verbs, broadcasting the action request before interactable execution.
- Added driver-level forwarder `OnActionRequestForwarded` to rebroadcast subsystem action requests to BP consumers.
- Registered canonical verb tags in `DefaultGameplayTags.ini`.
- Documented the seam in `SOTS_Interaction_SPINE4_CrossPluginSeam.md`.

## Files Touched
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionTypes.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionDriverComponent.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionDriverComponent.cpp
- Config/DefaultGameplayTags.ini
- Plugins/SOTS_Interaction/Docs/SOTS_Interaction_SPINE4_CrossPluginSeam.md

## Notes / Risks / Unknowns
- ItemTag/Quantity remain unset (best-effort placeholder); requires future hookup when pickup metadata is exposed.
- Assumes canonical verb tags will be loaded from config; no runtime verification added.

## Verification
- No builds or runtime tests executed (per instruction).

## Cleanup
- Deleted Plugins/SOTS_Interaction/Binaries and Plugins/SOTS_Interaction/Intermediate after edits.

## Follow-ups
- Surface pickup metadata into `FSOTS_InteractionActionRequest` when available.
- Wire downstream routers (e.g., SOTS_UI) to consume `OnInteractionActionRequested`.
