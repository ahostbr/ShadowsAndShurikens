# Worklog — SOTS_Input SPINE_3 Device + Dragon Layer — 2025-12-18 03:15

## Goal
Provide code-only seams for device change reporting (PlayerController-owned) and dragon control layer push/pop. No builds or IMC/content wiring.

## What changed
- Router: added `ReportInputDeviceFromKey` alias and `IsGamepadActive` convenience (device state already tracked via `LastDevice` + `OnInputDeviceChanged`).
- Blueprint helpers: device report helper using PlayerController ensure path; dragon layer push/pop helpers wrapping `Input.Layer.Dragon.Control`.
- Docs: added `SOTS_Input_DeviceAndDragonLayer.md`; updated overview device section.

## Files touched
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputRouterComponent.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputRouterComponent.cpp
- Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputBlueprintLibrary.h
- Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputBlueprintLibrary.cpp
- Plugins/SOTS_Input/Docs/SOTS_Input_DeviceAndDragonLayer.md
- Plugins/SOTS_Input/Docs/SOTS_Input_Overview.md

## Notes / Risks
- No build/run performed (code-only).
- Device reporting relies on callers binding an "Any Key" event to the helper; no automatic hookup provided.

## Cleanup
- Deleted Plugins/SOTS_Input/Binaries and Plugins/SOTS_Input/Intermediate (absent).
