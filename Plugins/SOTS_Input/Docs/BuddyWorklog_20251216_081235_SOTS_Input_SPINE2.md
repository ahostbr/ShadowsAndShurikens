# Buddy Worklog — SOTS_Input SPINE_2 — 2025-12-16 08:12:35

## Goal
Finish usable spine: layer registry + push-by-tag, device detection broadcast, intent handler, value-only buffered replay, docs/worklog. No UI focus changes or builds.

## What changed
- Added `USOTS_InputLayerRegistrySettings` (DeveloperSettings) and `USOTS_InputLayerRegistrySubsystem` for tag→layer lookups.
- Router: push layer by tag via registry, device change notification, intent broadcasting, value-only buffered replay.
- Added device helpers (`ESOTS_InputDevice`, `USOTS_InputDeviceLibrary`).
- Added generic intent handler `USOTS_InputHandler_IntentTag` (live and buffered paths).
- Updated overview docs with registry, device hook, intent broadcast guidance.

## Files changed
- Plugins/SOTS_Input/Source/SOTS_Input/SOTS_Input.Build.cs
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputLayerRegistrySettings.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputLayerRegistrySettings.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputLayerRegistrySubsystem.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputLayerRegistrySubsystem.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputDeviceTypes.h
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputDeviceLibrary.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputDeviceLibrary.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/Handlers/SOTS_InputHandler_IntentTag.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/Handlers/SOTS_InputHandler_IntentTag.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputHandler.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputHandler.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputRouterComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputRouterComponent.cpp
- Plugins/SOTS_Input/Docs/SOTS_Input_Overview.md

## Notes
- Buffered replay no longer synthesizes `FInputActionInstance`; handlers receive value-only data.
- Device change broadcast is passive; UI ownership remains with SOTS_UI.
- Build/tests not run (per instructions).

## Cleanup
- Deleted Plugins/SOTS_Input/Binaries and Plugins/SOTS_Input/Intermediate after edits.
