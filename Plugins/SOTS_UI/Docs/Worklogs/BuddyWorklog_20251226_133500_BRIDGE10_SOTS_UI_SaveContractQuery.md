# Buddy Worklog — 2025-12-26 13:35 — SOTS_UI BRIDGE10 (Core Save Contract query seam)

## Goal
Implement BRIDGE10: add an **OFF-by-default** helper in SOTS_UI that can query SOTS_Core Save Contract participants (`ISOTS_CoreSaveParticipant`) to determine whether saving is currently allowed.

Constraints honored:
- Add-only.
- No Blueprint edits.
- No Unreal build/run.
- No automatic wiring into existing save flows.

## What changed
- Added SOTS_UI settings toggles (default OFF):
  - `bEnableSOTSCoreSaveContractBridge`
  - `bEnableSOTSCoreSaveContractBridgeVerboseLogs`
- Added UI router query helper:
  - `USOTS_UIRouterSubsystem::QueryCanSaveViaCoreContract(const FSOTS_SaveRequestContext&, FText& OutBlockReason) const`
  - Enumerates `FSOTS_CoreSaveParticipantRegistry::GetRegisteredSaveParticipants(...)` and calls `QuerySaveStatus(...)`.
  - Returns `false` on first block and prefixes reason with `UI.Save.BlockedByParticipant:`.

## Files changed
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UISettings.h
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp

## Files created
- Plugins/SOTS_UI/Docs/SOTS_UI_SOTSCoreSaveContractBridge.md

## Notes / Risks / Unknowns
- Helper is intentionally not called by `RequestSaveGame` / `RequestCheckpointSave`; it is available for later UI gating.
- The reason prefix (`UI.Save.BlockedByParticipant`) is currently a string convention (no gameplay tag registration performed).

## Verification status
- UNVERIFIED: No Unreal build/run performed (Ryan will verify).

## Follow-ups / Next steps (Ryan)
- Build and verify the SOTS_UI module compiles.
- Toggle `bEnableSOTSCoreSaveContractBridge=true` and validate:
  - With no participants, helper returns true.
  - With a participant blocking, helper returns false and returns a non-empty reason.
