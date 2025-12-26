Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1148 | Plugin: SOTS_Stats | Pass/Topic: BRIDGE7_SaveParticipantBridge | Owner: Buddy
Scope: Opt-in bridge registering SOTS_Stats as a SOTS.CoreSaveParticipant emitting/applying opaque stats snapshot bytes

DONE
- Added USOTS_StatsCoreBridgeSettings (Config=Game, DefaultConfig) with bEnableSOTSCoreSaveParticipantBridge and bEnableSOTSCoreBridgeVerboseLogs (both default false)
- Added FStats_SaveParticipant : ISOTS_CoreSaveParticipant (id=Stats)
  - BuildSaveFragment emits FragmentId "Stats.Snapshot" with serialized FSOTS_CharacterStateData bytes (best-effort primary pawn)
  - ApplySaveFragment deserializes bytes and applies via USOTS_StatsLibrary::ApplySnapshotToActor
- Registered/unregistered participant in SOTS_Stats module StartupModule/ShutdownModule when enabled
- Added SOTS_Core dependency in Build.cs and .uplugin

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- Pawn resolution via GEngine world contexts is sufficient for save-status and fragment build/apply
- SOTS_Core enumeration requires USOTS_CoreSettings::bEnableSaveParticipantQueries

FILES TOUCHED
- Plugins/SOTS_Stats/SOTS_Stats.uplugin
- Plugins/SOTS_Stats/Source/SOTS_Stats/SOTS_Stats.Build.cs
- Plugins/SOTS_Stats/Source/SOTS_Stats/Public/SOTS_StatsCoreBridgeSettings.h
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsCoreBridgeSettings.cpp
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsCoreSaveParticipant.h
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsCoreSaveParticipant.cpp
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsModule.cpp
- Plugins/SOTS_Stats/Docs/SOTS_Stats_SOTSCoreSaveContractBridge.md

NEXT (Ryan)
- Enable SOTS Stats Core Bridge -> bEnableSOTSCoreSaveParticipantBridge
- Enable SOTS_Core -> bEnableSaveParticipantQueries
- Run SOTS.Core.DumpSaveParticipants and/or SOTS.Core.BridgeHealth to confirm participant id Stats is registered
- Validate that a save request builds a Stats.Snapshot payload and that applying it restores pawn stats

ROLLBACK
- Revert the touched files listed above
