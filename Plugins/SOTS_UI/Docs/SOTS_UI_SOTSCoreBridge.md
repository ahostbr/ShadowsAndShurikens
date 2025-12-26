# SOTS_UI ↔ SOTS_Core Bridge (BRIDGE3)

## Purpose
BRIDGE3 provides an **optional**, **disabled-by-default** integration where SOTS_UI can receive a deterministic "HUD is real and ready" signal from **SOTS_Core**.

This bridge is **state-only**: it records a HUD host pointer in the UI router for diagnostics and future hardening, and **does not** create widgets or change UI stack behavior.

## What it listens to
When **SOTS_Core listener dispatch** is enabled in Core, and this bridge is enabled in SOTS_UI, SOTS_UI registers a ModularFeatures listener and handles:
- `ISOTS_CoreLifecycleListener::OnSOTS_HUDReady(AHUD*)`

## What it does (when enabled)
- Calls `USOTS_UIRouterSubsystem::RegisterHUDHost(AHUD*)`
- Router stores a weak HUD pointer (host) and exposes:
  - `HasHUDHost()`
  - `GetHUDHostDebugString()`

## What it does NOT do
- Does **not** create widgets
- Does **not** call AddToViewport
- Does **not** push/pop/replace any router stack entries
- Does **not** change input mode

## Enablement
This bridge requires **two gates**:
1) In SOTS_Core settings: enable lifecycle listener dispatch
   - `bEnableLifecycleListenerDispatch = true`
2) In SOTS_UI settings: enable BRIDGE3
   - `bEnableSOTSCoreHUDHostBridge = true`

Optional:
- `bEnableSOTSCoreBridgeVerboseLogs = true`

## Verification (Ryan)
- With all toggles OFF: behavior should be unchanged.
- With Core dispatch ON + BRIDGE3 ON:
  - When the HUD reaches Core's HUDReady notify path, the router should report a valid HUD host via `HasHUDHost()` and `GetHUDHostDebugString()`.

## Rollback
- Set SOTS_UI `bEnableSOTSCoreHUDHostBridge=false`.
- No BP changes were made.
