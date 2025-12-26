Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1131 | Plugin: SOTS_KillExecutionManager | Pass/Topic: BRIDGE6_SaveParticipantBridge | Owner: Buddy
Scope: Opt-in bridge registering KEM as a SOTS.CoreSaveParticipant for SOTS_Core Save Contract queries

DONE
- Added USOTS_KEMCoreBridgeSettings (Config=Game, DefaultConfig) with bEnableSOTSCoreSaveParticipantBridge and bEnableSOTSCoreBridgeVerboseLogs (both default false)
- Added FKEM_SaveParticipant : ISOTS_CoreSaveParticipant (id=KEM) that blocks saves when USOTS_KEMManagerSubsystem::IsSaveBlocked() is true
- Registered/unregistered FKEM_SaveParticipant via FSOTS_CoreSaveParticipantRegistry in FSOTS_KillExecutionManagerModule StartupModule/ShutdownModule
- Added KEM private dependency on SOTS_Core

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- Save participant appears in SOTS_Core enumeration only when USOTS_CoreSettings::bEnableSaveParticipantQueries is enabled
- Subsystem resolution via GEngine world contexts is sufficient for save-status queries

FILES TOUCHED
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/SOTS_KillExecutionManager.Build.cs
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEMCoreBridgeSettings.h
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEMCoreBridgeSettings.cpp
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEMCoreSaveParticipant.h
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEMCoreSaveParticipant.cpp
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KillExecutionManagerModule.cpp
- Plugins/SOTS_KillExecutionManager/Docs/SOTS_KEM_SOTSCoreSaveContractBridge.md

NEXT (Ryan)
- In Project Settings, enable SOTS KillExecutionManager Core Bridge -> bEnableSOTSCoreSaveParticipantBridge
- In Project Settings, enable SOTS_Core -> bEnableSaveParticipantQueries
- Use SOTS.Core.DumpSaveParticipants and/or SOTS.Core.BridgeHealth to confirm participant id KEM is registered
- Validate that a save attempt while KEM is blocking saves reports bCanSave=false with BlockReason "KEM: Save blocked"

ROLLBACK
- Revert the touched files listed above
