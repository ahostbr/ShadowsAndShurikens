# Buddy Worklog - SOTS_Interaction - SPINE 5 (Polish + Guardrails)

## What changed
- Added player tag gating using InteractableComponent gates:
  - `RequiredPlayerTags` and `BlockedPlayerTags` are now enforced
  - Tag lookup is best-effort via SOTS_TagManager (fallback fail-open)
- Added Shipping/Test compile guards for debug drawing and verbose logs
- Tightened validation so both candidate selection and interaction requests respect gates

## Files added
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionTagAccess.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionTagAccess_Impl.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionTagAccess_Impl.cpp

## Files changed
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/SOTS_Interaction.Build.cs
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp

## Notes / Assumptions
- Buddy adjusted TagManager includes and API call to match your actual `USOTS_GameplayTagManagerSubsystem` surface.
- Tag gating currently fails open if TagManager provider isn't available to avoid breaking gameplay.

## TODO / Next
- Add OmniTrace integration (optional) for trace standardization.
- Add standardized fail reason tags (SAS.Interaction.Fail.*).
- BodyDragging plugin should register `DragBody` as an interaction option via ISOTS_InteractableInterface.
