# Buddy Worklog - BEHAVIOR_SWEEP_17_InputBuffers_ExecQTE (20251222_090836)

Goal
- Harden input buffer windows for Execute/Vanish/QTE with queue-size-1 semantics and ensure intents are buffered across all three channels.

What changed
- Enforced queue-size-1 for buffered events by setting MaxBufferedEventsPerChannel default to 1.
- Expanded allowed buffer window channels to include QTE and routed IntentTag buffering into Execution, Vanish, and QTE channels (latest wins per channel).
- Added gameplay tag `Input.Buffer.Channel.QTE` to DefaultGameplayTags.ini (add-only).

Files changed
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBufferComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBufferComponent.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Handlers/SOTS_InputHandler_IntentTag.cpp
- Config/DefaultGameplayTags.ini

Notes / Decisions
- Buffer window allowed-channel filter now covers Execution, Vanish, and QTE; buffering remains single-slot per channel.
- No behavior changes to Interaction/KEM routing; dep graph unaffected.

Verification
- Not run (Token Guard; no builds or runtime tests).

Cleanup
- Deleted Plugins/SOTS_Input/Binaries and Plugins/SOTS_Input/Intermediate.
