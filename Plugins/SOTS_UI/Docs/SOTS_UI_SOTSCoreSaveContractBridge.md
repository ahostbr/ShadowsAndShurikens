# SOTS_UI — SOTS_Core Save Contract Bridge (BRIDGE10)

## Goal
Provide an **OFF-by-default**, **add-only** seam that allows SOTS_UI (router) to query SOTS_Core's Save Contract participants to decide whether a save is currently allowed.

This bridge is **state/query only**:
- It does **not** trigger any save.
- It does **not** change any existing save flows.
- It is safe to leave enabled/disabled independently of ProfileShared saving.

## Settings
Project Settings → SOTS → UI → **SOTS Core Bridge**
- `bEnableSOTSCoreSaveContractBridge` (default: false)
- `bEnableSOTSCoreSaveContractBridgeVerboseLogs` (default: false)

## API
`USOTS_UIRouterSubsystem::QueryCanSaveViaCoreContract(const FSOTS_SaveRequestContext& RequestContext, FText& OutBlockReason)`
- Returns `true` if all registered `ISOTS_CoreSaveParticipant` report `bCanSave=true`.
- Returns `false` if any participant blocks; sets `OutBlockReason` (prefixed with `UI.Save.BlockedByParticipant:`).
- When bridge setting is disabled, returns `true` and leaves `OutBlockReason` empty.

## Notes
- Enumerates participants via `FSOTS_CoreSaveParticipantRegistry::GetRegisteredSaveParticipants(...)`.
- Uses ModularFeatures participants registered under `SOTS.CoreSaveParticipant`.
