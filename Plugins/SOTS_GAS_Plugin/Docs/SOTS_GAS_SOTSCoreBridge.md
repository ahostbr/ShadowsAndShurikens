# SOTS_GAS_Plugin ↔ SOTS_Core Bridge (BRIDGE18)

This bridge adds **optional** integration points from `SOTS_Core` into `SOTS_GAS_Plugin`.
All features are **disabled by default** and are designed to be **state-only** (no behavior change unless you enable the settings).

## Settings

Settings class: `USOTS_GASCoreBridgeSettings` (`Config=Game`, `DefaultConfig`)

- `bEnableSOTSCoreLifecycleBridge` (default **false**)
  - When enabled, registers a `SOTS.CoreLifecycleListener` that broadcasts GAS CoreBridge delegates.
- `bEnableSOTSCoreBridgeVerboseLogs` (default **false**)
  - Enables verbose logging for the bridge.
- `bEnableSOTSCoreSaveParticipantBridge` (default **false**)
  - When enabled, registers a `SOTS.CoreSaveParticipant` for GAS using existing profile snapshot seams.

## Lifecycle bridge (state-only)

Listener: `FGAS_CoreLifecycleHook : ISOTS_CoreLifecycleListener`

Broadcasts delegates (all native, state-only):
- `SOTS_GAS::CoreBridge::OnCoreWorldReady_Native()`
- `SOTS_GAS::CoreBridge::OnCorePrimaryPlayerReady_Native()`
- Optional travel relays (only if `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()` is true):
  - `SOTS_GAS::CoreBridge::OnCorePreLoadMap_Native()`
  - `SOTS_GAS::CoreBridge::OnCorePostLoadMap_Native()`

## Save Contract participant (opaque bytes)

Participant: `FGAS_SaveParticipant : ISOTS_CoreSaveParticipant`

- `GetParticipantId()` returns `"GAS"`.
- Fragment id: `"GAS.State"`.
- Payload contents: serialized `FSOTS_AbilityProfileData` (opaque bytes) via `StaticStruct()->SerializeItem`.
- Apply path: deserializes bytes back to `FSOTS_AbilityProfileData` and applies through `USOTS_AbilitySubsystem::ApplyProfileData`.

This is intentionally scoped to the existing **profile snapshot** seam (`BuildProfileData` / `ApplyProfileData`) and does not invent any new save format.
