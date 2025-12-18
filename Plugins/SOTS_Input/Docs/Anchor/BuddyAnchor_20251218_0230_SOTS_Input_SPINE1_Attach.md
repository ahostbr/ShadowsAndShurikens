[CONTEXT_ANCHOR]
ID: 20251218_0230 | Plugin: SOTS_Input | Pass/Topic: SPINE1_Attach | Owner: Buddy
Scope: PlayerController-first attachment/discovery helpers and docs for SOTS_Input router/buffer.

DONE
- Added PlayerController-aware lookup/ensure helpers to `USOTS_InputBlueprintLibrary` (resolve PC from world/context, create transient router/buffer components on PC when missing).
- Documented attachment flow and helper usage in new `SOTS_Input_AttachmentAndDiscovery.md`; updated overview to point to helpers and correct tag root guidance.

VERIFIED
- None (no build/editor/runtime executed).

UNVERIFIED / ASSUMPTIONS
- Runtime component creation path (`AddInstanceComponent` + `OnComponentCreated` + `RegisterComponent`) behaves correctly when called mid-play.
- Existing callers remain unaffected when a router already exists on the PlayerController.

FILES TOUCHED
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBlueprintLibrary.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBlueprintLibrary.cpp
- Plugins/SOTS_Input/Docs/SOTS_Input_Overview.md
- Plugins/SOTS_Input/Docs/SOTS_Input_AttachmentAndDiscovery.md

NEXT (Ryan)
- Call `EnsureRouterOnPlayerController`/`EnsureBufferOnPlayerController` from UI/gameplay entry points; confirm router auto-refresh initializes and buffering still works.
- Verify helpers pick the correct PlayerController in PIE (multiplayer and possession swaps) and that components register immediately.
- Decide whether to migrate existing Blueprint calls from `GetRouterFromActor` to PlayerController-aware helpers for consistency.

ROLLBACK
- Revert the four touched files in SOTS_Input or git revert the associated commit once created.
