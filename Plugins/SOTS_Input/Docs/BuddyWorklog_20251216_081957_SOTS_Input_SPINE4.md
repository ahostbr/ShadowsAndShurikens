# Buddy Worklog — SOTS_Input SPINE_4 — 2025-12-16 08:19:57

## Goal
Provide a usable integration seam, concrete intent handlers, debug surface, and updated examples so SOTS_Input can be called from SOTS_UI/gameplay without tight coupling.

## What changed
- Added `USOTS_InputBlueprintLibrary` helpers for router lookup and push/pop/buffer by tag from any actor context.
- Added Interact/MenuBack handler subclasses with default intent tags; added intent tags to SOTS_InputTags.ini.
- Router gains debug getters, optional periodic logging, and keeps binding index model; buffer exposes open-channel stack.
- Docs updated for integration seam, golden-path layer usage, debug surface, and handler guidance.
- Kept example layer assets (placeholders) with recommended handler templates documented.

## Files changed
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBlueprintLibrary.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBlueprintLibrary.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/Handlers/SOTS_InputHandler_Interact.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Handlers/SOTS_InputHandler_Interact.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/Handlers/SOTS_InputHandler_MenuBack.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Handlers/SOTS_InputHandler_MenuBack.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBufferComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBufferComponent.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputRouterComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputRouterComponent.cpp
- Plugins/SOTS_Input/Config/Tags/SOTS_InputTags.ini
- Plugins/SOTS_Input/Docs/SOTS_Input_Overview.md

## Notes
- TagManager reflection still targets class `USOTS_GameplayTagManagerSubsystem` and function `ActorHasTag`.
- Example layer assets remain placeholders; open/re-save as `USOTS_InputLayerDataAsset` and add handlers as documented.
- No SOTS_UI modifications in this pass; integration seam is ready for it to call.

## Verification
- Build/tests not run (per instructions).

## Cleanup
- Deleted Plugins/SOTS_Input/Binaries and Plugins/SOTS_Input/Intermediate after edits.

## Follow-ups
- Wire SOTS_UI/gameplay to BlueprintLibrary seam and add real IMCs/IAs to layer assets.
- Enable router debug logging sparingly when investigating input flow.
