# Buddy Worklog — SOTS_Input SPINE_3 — 2025-12-16 08:17:35

## Goal
Make SOTS_Input drop-in usable: safe Enhanced Input bindings, optional tag gating, buffer stack priority, layer consume policies, intent broadcast helper, example layer assets, and docs.

## What changed
- Router now tracks owned binding indices (non-destructive removal), supports tag gates (reflection to `USOTS_GameplayTagManagerSubsystem.ActorHasTag`), device notify, intents, and value-only buffered replay improvements.
- Added gate rule struct, device types/helpers, and consume policy on layer data assets.
- Buffer component uses a stack for open channels (top-priority buffering).
- Added intent handler endpoint and placeholder layer assets (NinjaDefault, UINav, Cutscene) with plugin content enabled.
- Updated overview docs for SPINE_3 behavior and guidance.

## Files changed
- Plugins/SOTS_Input/SOTS_Input.uplugin
- Plugins/SOTS_Input/Source/SOTS_Input/SOTS_Input.Build.cs
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBufferComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBufferComponent.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputGateTypes.h
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputDeviceTypes.h
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputDeviceLibrary.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputDeviceLibrary.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputLayerDataAsset.h
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputRouterComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputRouterComponent.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/Handlers/SOTS_InputHandler_IntentTag.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Handlers/SOTS_InputHandler_IntentTag.cpp
- Plugins/SOTS_Input/Docs/SOTS_Input_Overview.md
- Plugins/SOTS_Input/Content/InputLayers/DA_InputLayer_NinjaDefault.uasset (placeholder)
- Plugins/SOTS_Input/Content/InputLayers/DA_InputLayer_UINav.uasset (placeholder)
- Plugins/SOTS_Input/Content/InputLayers/DA_InputLayer_Cutscene.uasset (placeholder)

## TagManager reflection
- Optional gate integration looks for class `USOTS_GameplayTagManagerSubsystem` and calls function `ActorHasTag` via `ProcessEvent`.

## Verification
- Build/tests not run (per instructions). Placeholder assets need opening/resaving in-editor to become real data assets and then registry entries can be added.

## Cleanup
- Deleted Plugins/SOTS_Input/Binaries and Plugins/SOTS_Input/Intermediate after edits.

## Follow-ups
- In editor: re-save placeholder layer assets as `USOTS_InputLayerDataAsset` and register in Project Settings → SOTS Input Layer Registry.
- Bind Any Key to router `NotifyKeyInput` and hook `OnInputIntent`/`OnInputDeviceChanged` in listening systems.
