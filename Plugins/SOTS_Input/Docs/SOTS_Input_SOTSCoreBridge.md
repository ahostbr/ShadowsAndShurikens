# SOTS_Input Core Bridge (SOTS_Core Lifecycle)

Optional listener that lets SOTS_Input respond to SOTS_Core lifecycle callbacks without timing guesses.
Defaults OFF; no behavior changes unless explicitly enabled.

## Enablement
1) Enable lifecycle dispatch in SOTS_Core (`USOTS_CoreSettings.bEnableLifecycleListenerDispatch`).
2) Enable the SOTS_Input bridge:
   - `USOTS_InputCoreBridgeSettings.bEnableSOTSCoreLifecycleBridge`
3) Optional logging:
   - `USOTS_InputCoreBridgeSettings.bEnableSOTSCoreBridgeVerboseLogs`

## Behavior (when enabled)
- `OnSOTS_PlayerControllerBeginPlay`: ensures `USOTS_InputRouterComponent` exists on the PC.
- `OnSOTS_PawnPossessed`: refreshes router bindings for the new pawn.

No input mode changes are performed here; this is state-only wiring.

## Config example
```
[/Script/SOTS_Input.SOTS_InputCoreBridgeSettings]
bEnableSOTSCoreLifecycleBridge=false
bEnableSOTSCoreBridgeVerboseLogs=false
```
