# SOTS_AIPerception — SOTS_Core Lifecycle Bridge (BRIDGE11)

## Goal
Provide an **OFF-by-default**, add-only bridge so SOTS_AIPerception can receive deterministic lifecycle callbacks via SOTS_Core.

This pass is **state-only**:
- No AI perception behavior is changed by default.
- Even when enabled, the bridge only caches pointers and broadcasts internal delegates.

## Settings
Project Settings → (Developer Settings) → **SOTS AIPerception - Core Bridge**
- `bEnableSOTSCoreLifecycleBridge` (default: false)
- `bEnableSOTSCoreBridgeVerboseLogs` (default: false)

## What gets wired (when enabled)
- `ISOTS_CoreLifecycleListener::OnSOTS_WorldStartPlay(UWorld*)`
  - calls `USOTS_AIPerceptionSubsystem::HandleCoreWorldReady(World)`
- `ISOTS_CoreLifecycleListener::OnSOTS_PawnPossessed(APlayerController*, APawn*)`
  - calls `USOTS_AIPerceptionSubsystem::HandleCorePrimaryPlayerReady(PC, Pawn)`
- Optional map travel hooks (only if Core `bEnableMapTravelBridge=true`):
  - binds to `USOTS_CoreLifecycleSubsystem::OnPreLoadMap_Native` → `HandleCorePreLoadMap(MapName)`
  - binds to `USOTS_CoreLifecycleSubsystem::OnPostLoadMap_Native` → `HandleCorePostLoadMap(World)`

## Notes
- Registration occurs at module startup only (no runtime hot-toggle).
- Duplicate suppression is applied for the PawnPossessed PC/Pawn pair.
