# Buddy Worklog — 2025-12-26 09:30 — SOTS_UI BRIDGE3 (Core HUDReady host seam)

## Goal
Implement BRIDGE3: optional, OFF-by-default wiring so SOTS_UI can receive `OnSOTS_HUDReady(AHUD*)` via SOTS_Core lifecycle listener dispatch and register a HUD host in the UI router **without changing UI behavior**.

## What changed
- Added `SOTS_Core` dependency to SOTS_UI module (Build.cs).
- Added SOTS_UI settings toggles (default OFF):
  - `bEnableSOTSCoreHUDHostBridge`
  - `bEnableSOTSCoreBridgeVerboseLogs`
- Registered a SOTS_UI ModularFeatures listener implementing `ISOTS_CoreLifecycleListener`.
  - On `OnSOTS_HUDReady(AHUD*)`, early-outs unless the SOTS_UI bridge toggle is enabled.
  - Calls router `RegisterHUDHost(HUD)` (state-only).
- Added router seam + diagnostics:
  - `RegisterHUDHost(AHUD*)`
  - `HasHUDHost()`
  - `GetHUDHostDebugString()`

## Files changed
- Plugins/SOTS_UI/Source/SOTS_UI/SOTS_UI.Build.cs
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UISettings.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIModule.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp

## Files created
- Plugins/SOTS_UI/Docs/SOTS_UI_SOTSCoreBridge.md

## Notes / Risks / Unknowns
- Bridge remains inert unless BOTH:
  - Core: `bEnableLifecycleListenerDispatch=true`
  - UI: `bEnableSOTSCoreHUDHostBridge=true`
- Router host registration is intentionally state-only; no widget creation.

## Verification status
- UNVERIFIED: No Unreal build/run performed (Ryan will verify).

## Follow-ups / Next steps (Ryan)
- Rebuild and run with Core dispatch ON + BRIDGE3 ON; verify host debug string updates after HUDReady.
- Confirm no UI behavior changes with all toggles OFF.
