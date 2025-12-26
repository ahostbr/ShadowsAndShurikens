# SOTS_ProfileShared Core Bridge (SOTS_Core Lifecycle)

Optional listener that lets SOTS_ProfileShared receive deterministic lifecycle callbacks via SOTS_Core.
Defaults OFF; no save/load behavior changes unless explicitly enabled.

## Enablement
1) Enable lifecycle dispatch in SOTS_Core (`USOTS_CoreSettings.bEnableLifecycleListenerDispatch`).
2) Enable the ProfileShared bridge:
   - `USOTS_ProfileSharedCoreBridgeSettings.bEnableSOTSCoreLifecycleBridge`
3) Optional logging:
   - `USOTS_ProfileSharedCoreBridgeSettings.bEnableSOTSCoreBridgeVerboseLogs`

## Behavior (when enabled)
- `OnSOTS_WorldStartPlay`: caches the world and fires `OnCoreWorldStartPlay` (state-only).
- `OnSOTS_PostLogin`: marks the player session as started (state-only).
- `OnSOTS_PawnPossessed`: updates primary player ready state and fires `OnCorePrimaryPlayerReady`.

No IO, no autosave, and no travel actions are triggered in this bridge.

## Config example
```
[/Script/SOTS_ProfileShared.SOTS_ProfileSharedCoreBridgeSettings]
bEnableSOTSCoreLifecycleBridge=false
bEnableSOTSCoreBridgeVerboseLogs=false
```
