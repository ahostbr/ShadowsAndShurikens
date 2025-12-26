# SOTS GlobalStealthManager ↔ SOTS_Core Bridge (BRIDGE12)

## Goal
Provide an **OFF-by-default**, **state-only** bridge from `SOTS_Core` lifecycle events to `SOTS_GlobalStealthManager`.

This pass does **not** change stealth scoring, tags, or gameplay behavior by default.

## What is bridged
When enabled, the module registers a `ISOTS_CoreLifecycleListener` and forwards:
- `OnSOTS_WorldStartPlay(UWorld*)` → `USOTS_GlobalStealthManagerSubsystem::HandleCoreWorldReady(UWorld*)`
- `OnSOTS_PawnPossessed(APlayerController*, APawn*)` → `USOTS_GlobalStealthManagerSubsystem::HandleCorePrimaryPlayerReady(...)`

These handlers only cache the last-seen values and broadcast internal native multicast delegates:
- `USOTS_GlobalStealthManagerSubsystem::OnCoreWorldReady`
- `USOTS_GlobalStealthManagerSubsystem::OnCorePrimaryPlayerReady`

## Enablement
Project Settings:
- **SOTS GlobalStealthManager - Core Bridge**
  - `bEnableSOTSCoreLifecycleBridge` (default: `false`)
  - `bEnableSOTSCoreBridgeVerboseLogs` (default: `false`)

## Notes
- Registration occurs in `FSOTS_GlobalStealthManagerModule::StartupModule()` only when enabled at startup.
- This bridge is intended as a safe integration seam for future work; it should remain inert when disabled.
