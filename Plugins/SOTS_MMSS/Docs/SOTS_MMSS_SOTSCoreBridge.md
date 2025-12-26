# SOTS_MMSS ↔ SOTS_Core Bridge (BRIDGE17)

## Goal
Provide an OPTIONAL (disabled-by-default) integration so **SOTS_MMSS** can receive deterministic lifecycle + (optional) map travel callbacks from **SOTS_Core**, without relying on timing/adapters.

This pass is **state-only**:
- It broadcasts native delegates other code can subscribe to.
- It does **not** change MMSS music selection/playback behavior.

## Enablement
Project Settings → **SOTS_MMSS - Core Bridge**
- `bEnableSOTSCoreLifecycleBridge` (default **false**)
- `bEnableSOTSCoreBridgeVerboseLogs` (default **false**)

## What it registers
When enabled, SOTS_MMSS registers a Core lifecycle listener (`ISOTS_CoreLifecycleListener`) on module startup.

## Events exposed (native delegates)
SOTS_MMSS exposes these **native, state-only** delegates:
- `SOTS_MMSS::CoreBridge::OnCoreWorldReady_Native()` → `UWorld*`
- `SOTS_MMSS::CoreBridge::OnCorePrimaryPlayerReady_Native()` → `APlayerController*, APawn*` (local controller only)
- `SOTS_MMSS::CoreBridge::OnCorePreLoadMap_Native()` → `const FString&` *(only if Core travel bridge is bound)*
- `SOTS_MMSS::CoreBridge::OnCorePostLoadMap_Native()` → `UWorld*` *(only if Core travel bridge is bound)*

## Notes
- Map travel relays bind to `USOTS_CoreLifecycleSubsystem` only if `IsMapTravelBridgeBound()` is true.
- Duplicate suppression: repeated PawnPossessed for the same (PC,Pawn) pair is ignored.

## Verification status
UNVERIFIED (no Unreal build/run in this pass). Static analysis reported no errors in the touched files.
