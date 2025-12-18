# Worklog — SOTS_Input SPINE_2 Buffer Windows — 2025-12-18 03:05

## Goal
Implement IN-03 minimal montage buffer windows (Execution/Vanish) with auto-clear backstop, AnimNotifyState wiring, and intent buffering seam.

## What changed
- Added windowed intent buffering surface to `USOTS_InputBufferComponent` (open/close window, queue size 1 latest-wins, consume/clear, montage end/cancel auto-clear via AnimInstance delegates).
- AnimNotifyState now opens/closes buffer windows via PlayerController ensure helper and defaults to `Input.Buffer.Channel.Execution`.
- Intent handler buffers into Execution/Vanish channels when windows are open before broadcasting.
- Added vanish/QTE buffer channel tag and new buffer windows doc.

## Files touched
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBufferTypes.h
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBufferComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBufferComponent.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/Animation/AnimNotifyState_SOTS_InputBufferWindow.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Animation/AnimNotifyState_SOTS_InputBufferWindow.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Handlers/SOTS_InputHandler_IntentTag.cpp
- Plugins/SOTS_Input/Config/Tags/SOTS_InputTags.ini
- Plugins/SOTS_Input/Docs/SOTS_Input_BufferWindows.md

## Notes / Risks
- No build/run performed (code-only review). AnimInstance delegate binding path should be validated in PIE with montage end/cancel/abort cases.
- Allowed channel enforcement is hard-coded (Execution/Vanish); other channels are ignored.

## Cleanup
- Deleted Plugins/SOTS_Input/Binaries and Plugins/SOTS_Input/Intermediate.
