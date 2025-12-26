# SOTS_KillExecutionManager → SOTS_Core Save Contract Bridge (BRIDGE6)

## Goal
Expose KEM as an optional `SOTS.CoreSaveParticipant` so SOTS_Core Save Contract queries can consider KEM’s internal save-block state.

This bridge is **opt-in** and defaults **OFF**.

## Settings
Project Settings → Plugins → **SOTS KillExecutionManager Core Bridge**

- `bEnableSOTSCoreSaveParticipantBridge` (default: `false`)
  - When enabled, KEM registers a save participant with id `KEM`.
- `bEnableSOTSCoreBridgeVerboseLogs` (default: `false`)
  - Enables verbose log lines when the participant is registered/unregistered and when a save is blocked.

## Behavior
- Participant id: `KEM`
- Save status query:
  - If the bridge is disabled: returns “can save”.
  - If enabled and the KEM manager subsystem can be resolved:
    - If `USOTS_KEMManagerSubsystem::IsSaveBlocked()` is `true`, returns `bCanSave=false` and `BlockReason="KEM: Save blocked"`.
  - If the subsystem cannot be resolved: returns “can save” (best-effort, verbose log optional).
- Payload fragments:
  - KEM does not build or apply save fragments for this bridge.

## Dependencies
- KEM adds a module dependency on `SOTS_Core` (private) to access `ISOTS_CoreSaveParticipant` and the registration helper.

## Notes
- Registration happens at module startup/shutdown only (no runtime hot-reload).
- Save-participant enumeration inside SOTS_Core is gated by `USOTS_CoreSettings::bEnableSaveParticipantQueries`.
