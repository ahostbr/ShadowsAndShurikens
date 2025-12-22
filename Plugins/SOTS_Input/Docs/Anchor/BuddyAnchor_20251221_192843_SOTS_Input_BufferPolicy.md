[CONTEXT_ANCHOR]
ID: 20251221_192843 | Plugin: SOTS_Input | Pass/Topic: BufferPolicyDoc | Owner: Buddy
Scope: Document the locked buffer windows + router/device responsibilities for player-controller-owned routing.

DONE
- Extended `SOTS_Input_BufferWindows.md` with explicit references to the PlayerController router/buffer spine, the queue-size-1 enforcement in `OpenWindows`/`BufferedIntentByChannel`, and the device-change delegate that UI layers can subscribe to.
- Clarified that `EnsureRouterOnPlayerController`/`EnsureBufferOnPlayerController` centralize runtime items on the PC, and that SOTS_UI’s router subsystem owns all `SetInputMode` switches.
- Cleaned the plugin’s Binaries and Intermediate folders now that editing is finished.

VERIFIED
- None (documentation-only change; no build/editor/runtime run).

UNVERIFIED / ASSUMPTIONS
- Device-change wiring is inferred from the `OnInputDeviceChanged` delegate; I have not confirmed an actual binding chain besides the router endpoint.
- Assume SOTS_UI continues to be the only code touching `APlayerController::SetInputMode` for UI layers.

FILES TOUCHED
- Plugins/SOTS_Input/Docs/SOTS_Input_BufferWindows.md

NEXT (Ryan)
- Confirm the UI router continues to be the exclusive location for `SetInputMode` transitions, and that any new device-change subscribers tie into the router delegate rather than rewriting mode policy.

ROLLBACK
- Revert Plugins/SOTS_Input/Docs/SOTS_Input_BufferWindows.md (or git revert the committing change that added these paragraphs).
