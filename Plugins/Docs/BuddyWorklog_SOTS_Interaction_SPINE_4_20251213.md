# Buddy Worklog - SOTS_Interaction - SPINE 4 (Request Flow + UI Intents)

## What changed
- Added interaction request flow to `USOTS_InteractionSubsystem`:
  - `RequestInteraction()` to validate + execute or request UI choices
  - `ExecuteInteractionOption()` for UI callback execution
- Added decoupled UI intent event:
  - `OnUIIntent(PlayerController, IntentTag, Context, Options)`
- Routed prompt intent on candidate change (show/hide handled by SOTS_UI)

## Files changed
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp

## Notes / Assumptions
- SOTS_Interaction does not include or depend on SOTS_UI; it only emits UI intent events.
- Actual widget presentation and InteractionEssentials usage remain inside SOTS_UI.

## TODO (next pass)
- Add dev-only compile guards for debug draws/log noise.
- Add player tag gating using SOTS_TagManager (Required/Blocked tags).
- Polish intent tags defaults (set via config/DA).