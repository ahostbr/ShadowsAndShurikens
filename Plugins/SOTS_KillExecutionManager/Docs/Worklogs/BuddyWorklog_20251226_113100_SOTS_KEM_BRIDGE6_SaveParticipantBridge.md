# Buddy Worklog — SOTS_KillExecutionManager — BRIDGE6 Save Participant Bridge

## Goal
Implement BRIDGE6: bridge SOTS_KillExecutionManager (KEM) into the SOTS_Core Save Contract as an **opt-in** `SOTS.CoreSaveParticipant`.

Constraints followed:
- Add-only behavior.
- Defaults OFF.
- No Blueprint edits.
- No builds/tests triggered.

## What changed
- Added a new KEM DeveloperSettings surface to control the bridge.
- Implemented `FKEM_SaveParticipant : ISOTS_CoreSaveParticipant` that reports save blocking via `USOTS_KEMManagerSubsystem::IsSaveBlocked()`.
- Registered/unregistered the participant during KEM module startup/shutdown when enabled.
- Added BRIDGE6 documentation.

## Files changed
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/SOTS_KillExecutionManager.Build.cs
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEMCoreBridgeSettings.h
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEMCoreBridgeSettings.cpp
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEMCoreSaveParticipant.h
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEMCoreSaveParticipant.cpp
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KillExecutionManagerModule.cpp
- Plugins/SOTS_KillExecutionManager/Docs/SOTS_KEM_SOTSCoreSaveContractBridge.md

## Notes / Risks / Unknowns
- The save-participant query path in SOTS_Core is gated by `USOTS_CoreSettings::bEnableSaveParticipantQueries`; if that remains OFF during testing, SOTS_Core enumeration will appear empty even if KEM registered successfully.
- `FKEM_SaveParticipant::QuerySaveStatus` resolves the subsystem via `GEngine->GetWorldContexts()` as a best-effort approach because the save request context does not carry a `UWorld*`.

## Verification status
- UNVERIFIED: No build or in-editor verification performed.

## Follow-ups / Next steps (Ryan)
- Enable `bEnableSOTSCoreSaveParticipantBridge` under Project Settings → Plugins → SOTS KillExecutionManager Core Bridge.
- Enable `USOTS_CoreSettings::bEnableSaveParticipantQueries` to allow SOTS_Core to enumerate participants.
- Run `SOTS.Core.DumpSaveParticipants` and/or `SOTS.Core.BridgeHealth` and confirm `KEM` appears.
- Trigger a save while KEM is in a state where it blocks saves and confirm the participant reports `bCanSave=false` with `BlockReason="KEM: Save blocked"`.
