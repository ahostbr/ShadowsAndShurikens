# SOTS_FX_Plugin ↔ SOTS_Core Bridge (BRIDGE13)

## Goal
Provide an **OFF-by-default**, **state-only** bridge from `SOTS_Core` lifecycle events to `SOTS_FX_Plugin`.

This pass does **not** change FX behavior by default.

## What is bridged
When enabled, the module registers a `ISOTS_CoreLifecycleListener` and forwards:
- `OnSOTS_WorldStartPlay(UWorld*)` → `USOTS_FXManagerSubsystem::HandleCoreWorldReady(UWorld*)`
- `OnSOTS_PawnPossessed(APlayerController*, APawn*)` → `USOTS_FXManagerSubsystem::HandleCorePrimaryPlayerReady(...)`

These handlers only cache the last-seen values and broadcast internal native multicast delegates:
- `USOTS_FXManagerSubsystem::OnCoreWorldReady`
- `USOTS_FXManagerSubsystem::OnCorePrimaryPlayerReady`

## Enablement
Project Settings:
- **SOTS FX - Core Bridge**
  - `bEnableSOTSCoreLifecycleBridge` (default: `false`)
  - `bEnableSOTSCoreBridgeVerboseLogs` (default: `false`)

## Notes
- Registration occurs in module startup only when enabled at startup.
- Intended as a safe seam for future deterministic init; keep inert when disabled.
