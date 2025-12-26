# Buddy Worklog — SOTS_Stats — BRIDGE7 Save Participant Bridge

## Goal
Implement BRIDGE7: bridge SOTS_Stats into the SOTS_Core Save Contract as an **opt-in** `SOTS.CoreSaveParticipant` that emits/applies an opaque payload fragment.

Constraints followed:
- Add-only behavior.
- Defaults OFF.
- No Blueprint edits.
- No Unreal build/run.

## RepoIntel evidence (existing snapshot seam)
- Stats authoritative snapshot payload type: `FSOTS_CharacterStateData` (from SOTS_ProfileShared)
- Snapshot/apply helpers:
  - `USOTS_StatsLibrary::SnapshotActorStats(AActor*, FSOTS_CharacterStateData&)`
  - `USOTS_StatsLibrary::ApplySnapshotToActor(AActor*, const FSOTS_CharacterStateData&)`
- Component build/apply helpers:
  - `USOTS_StatsComponent::BuildCharacterStateData(FSOTS_CharacterStateData&)`
  - `USOTS_StatsComponent::ApplyCharacterStateData(const FSOTS_CharacterStateData&)`

## What changed
- Added a Stats DeveloperSettings surface to enable/disable the bridge (default OFF).
- Implemented `FStats_SaveParticipant : ISOTS_CoreSaveParticipant` (id `Stats`).
  - QuerySaveStatus always allows saving.
  - BuildSaveFragment/ApplySaveFragment serialize/deserialize `FSOTS_CharacterStateData` to/from bytes using `StaticStruct()->SerializeItem`.
- Registered/unregistered the participant in `FSOTS_StatsModule` StartupModule/ShutdownModule when enabled.
- Added SOTS_Core dependency wiring (Build.cs + .uplugin).

## Files changed/created
- Plugins/SOTS_Stats/SOTS_Stats.uplugin
- Plugins/SOTS_Stats/Source/SOTS_Stats/SOTS_Stats.Build.cs
- Plugins/SOTS_Stats/Source/SOTS_Stats/Public/SOTS_StatsCoreBridgeSettings.h
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsCoreBridgeSettings.cpp
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsCoreSaveParticipant.h
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsCoreSaveParticipant.cpp
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsModule.cpp
- Plugins/SOTS_Stats/Docs/SOTS_Stats_SOTSCoreSaveContractBridge.md

## Notes / Risks / Unknowns
- The save request context does not carry a `UWorld*`, so the participant resolves the pawn via `GEngine->GetWorldContexts()` (best-effort).
- Stats snapshot bytes are opaque and rely on UStruct reflection serialization; compatibility is assumed to be managed by the owning systems.
- SOTS_Core enumeration is gated by `USOTS_CoreSettings::bEnableSaveParticipantQueries`.

## Verification status
- UNVERIFIED: No build or in-editor verification performed.

## Follow-ups / Next steps (Ryan)
- Enable Project Settings → Plugins → SOTS Stats Core Bridge → `bEnableSOTSCoreSaveParticipantBridge=true`.
- Enable SOTS_Core → `bEnableSaveParticipantQueries=true`.
- Run `SOTS.Core.DumpSaveParticipants` / `SOTS.Core.BridgeHealth` and confirm `Stats` appears.
- Trigger a save and confirm the `Stats.Snapshot` fragment is built/applied (verbose logs if enabled).
