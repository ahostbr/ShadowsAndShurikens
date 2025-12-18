[CONTEXT_ANCHOR]
ID: 20251218_0306 | Plugin: SOTS_Input | Pass/Topic: SPINE2_BufferWindows | Owner: Buddy
Scope: IN-03 montage buffer windows (Execution/Vanish), AnimNotifyState integration, intent buffering seam.

DONE
- Added windowed intent buffering APIs to `USOTS_InputBufferComponent` (open/close window, TryBufferIntent/ConsumeBufferedIntent, clear/all clear, montage end/cancel auto-clear with AnimInstance delegates; allowed channels Execution/Vanish only, latest wins).
- Updated `UAnimNotifyState_SOTS_InputBufferWindow` to default to Execution channel and open/close windows via PlayerController buffer ensure helper; added optional AllowedIntentTags property.
- `USOTS_InputHandler_IntentTag` now buffers intents into Execution/Vanish when windows are open before broadcasting.
- Added tag `Input.Buffer.Channel.Vanish` and authored `SOTS_Input_BufferWindows.md` doc.

VERIFIED
- None (no build/editor/runtime executed).

UNVERIFIED / ASSUMPTIONS
- AnimInstance delegate binding/unbinding behaves correctly across montage end, blend-out, cancel/abort in PIE.
- Notify state resolving buffer via EnsureBufferOnPlayerController succeeds in multiplayer/possession swaps.

FILES TOUCHED
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBufferTypes.h
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBufferComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBufferComponent.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/Animation/AnimNotifyState_SOTS_InputBufferWindow.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Animation/AnimNotifyState_SOTS_InputBufferWindow.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Handlers/SOTS_InputHandler_IntentTag.cpp
- Plugins/SOTS_Input/Config/Tags/SOTS_InputTags.ini
- Plugins/SOTS_Input/Docs/SOTS_Input_BufferWindows.md

NEXT (Ryan)
- In PIE, trigger execution/vanish montages with the notify state and verify: window opens, intent buffered (latest wins), auto-clear fires on montage end/cancel/abort, and ConsumeBufferedIntent retrieves/clears as expected.
- Confirm buffer ensure helper finds the right PlayerController in multiplayer/possession cases.
- Decide if AllowedIntentTags should gate buffering in a follow-up.

ROLLBACK
- Revert the touched files or git revert the eventual commit for this pass.
