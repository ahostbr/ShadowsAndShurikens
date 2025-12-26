# SOTS_Stats → SOTS_Core Save Contract Bridge (BRIDGE7)

## Goal
Register SOTS_Stats as an optional `SOTS.CoreSaveParticipant` that can emit/apply an **opaque** binary fragment representing the current authoritative stats snapshot.

This bridge is **opt-in** and defaults **OFF**.

## Settings
Project Settings → Plugins → **SOTS Stats Core Bridge**

- `bEnableSOTSCoreSaveParticipantBridge` (default: `false`)
  - When enabled, SOTS_Stats registers a save participant with id `Stats`.
- `bEnableSOTSCoreBridgeVerboseLogs` (default: `false`)
  - Enables verbose log lines for registration/unregistration and fragment build/apply.

## Participant behavior
- Participant id: `Stats`
- QuerySaveStatus:
  - Always returns can-save (Stats does not gate saving).
- BuildSaveFragment:
  - When enabled, takes a best-effort snapshot from the resolved primary pawn:
    - `USOTS_StatsLibrary::SnapshotActorStats(Pawn, FSOTS_CharacterStateData)`
  - Serializes `FSOTS_CharacterStateData` to bytes using UStruct reflection serialization.
  - Emits `FragmentId = "Stats.Snapshot"`.
- ApplySaveFragment:
  - When enabled, deserializes bytes into `FSOTS_CharacterStateData` and applies to the resolved primary pawn:
    - `USOTS_StatsLibrary::ApplySnapshotToActor(Pawn, Snapshot)`

## Notes / Caveats
- The save contract request context does not provide a `UWorld*`, so this bridge resolves a pawn via `GEngine->GetWorldContexts()` (best-effort).
- SOTS_Core enumeration of save participants is gated by `USOTS_CoreSettings::bEnableSaveParticipantQueries`.
- This bridge introduces no IO and does not trigger autosaves.
