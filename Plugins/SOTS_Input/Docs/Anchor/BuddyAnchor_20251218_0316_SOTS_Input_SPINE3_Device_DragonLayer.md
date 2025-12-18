[CONTEXT_ANCHOR]
ID: 20251218_0316 | Plugin: SOTS_Input | Pass/Topic: SPINE3_Device_DragonLayer | Owner: Buddy
Scope: Device change reporting seam and dragon control layer push/pop helpers (code-only).

DONE
- Router: added `ReportInputDeviceFromKey` alias and `IsGamepadActive`; device state already tracked via `LastDevice` + `OnInputDeviceChanged`.
- Blueprint helpers: PlayerController-aware `ReportInputDeviceFromKey`, `PushDragonControlLayer`, `PopDragonControlLayer` using `Input.Layer.Dragon.Control` tag.
- Docs: added `SOTS_Input_DeviceAndDragonLayer.md`; updated overview device section.

VERIFIED
- None (no build/editor/runtime executed).

UNVERIFIED / ASSUMPTIONS
- Callers will bind an "Any Key" input to the helper to drive device change events; not auto-wired.
- Helpers behave correctly across multiplayer/possession swaps via ensure path.

FILES TOUCHED
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputRouterComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputRouterComponent.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBlueprintLibrary.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBlueprintLibrary.cpp
- Plugins/SOTS_Input/Docs/SOTS_Input_DeviceAndDragonLayer.md
- Plugins/SOTS_Input/Docs/SOTS_Input_Overview.md

NEXT (Ryan)
- In PIE, bind an "Any Key" event on the PlayerController to `ReportInputDeviceFromKey` and confirm `OnInputDeviceChanged` fires and `IsGamepadActive` reflects state.
- Exercise `PushDragonControlLayer`/`PopDragonControlLayer` and verify layer stack responds (no IMC wiring yet).

ROLLBACK
- Revert the touched files or git revert the commit for this pass.
