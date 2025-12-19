# Buddy Worklog - SOTS_Interaction - PASS 8 (UI Intent Payloads)

## What changed
- Introduced `FSOTS_InteractionUIIntentPayload` carrying intent tag, context, options, fail reason tag, and prompt show/hide flag in one struct.
- Subsystem now emits the unified payload, standardizes fail intents without fake options, adds editable fail reason tags, and keeps the legacy delegate for temporary compatibility.
- Driver binds to the new payload delegate and forwards the struct to BP/UI consumers.

## Files added
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionUIIntentPayload.h

## Files changed
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionDriverComponent.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionDriverComponent.cpp

## Notes / Assumptions
- Fail intents now use the dedicated fail tag field; legacy `OnUIIntent` still fires split params (no fake option payload) for existing bindings.
- Added editable standard fail tags (NoCandidate / BlockedByTags / InterfaceDenied) that fill in when interfaces do not supply a reason.
- Prompt intent includes `bShowPrompt`; options intent still carries `Options` for backward compatibility.

## Cleanup
- Removed Plugins/SOTS_Interaction/Intermediate.
- Plugins/SOTS_Interaction/Binaries still present; deletion failed because Win64 DLL was locked (likely by an open editor session).

## TODO / Next
- Update SOTS_UI glue to consume the new payload delegate and drop the deprecated split-parameter event.
- Retry plugin Binaries cleanup once binaries are not locked.
