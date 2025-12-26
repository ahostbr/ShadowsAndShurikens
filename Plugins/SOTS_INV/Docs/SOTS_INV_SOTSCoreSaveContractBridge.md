# SOTS_INV → SOTS_Core Save Contract Bridge (BRIDGE14)

This provides an **opt-in** bridge that registers SOTS_INV as a `SOTS.CoreSaveParticipant`, allowing `SOTS_Core` to query status and request an **opaque** inventory fragment.

## Defaults (safe)
- Disabled-by-default: `USOTS_INVCoreBridgeSettings::bEnableSOTSCoreSaveParticipantBridge = false`
- No IO introduced.
- No InvSP UI calls introduced.

## What it registers
- Participant id: `INV`
- Fragment id: `INV.Inventory`

## Data source / seam
When enabled, the participant serializes `FSOTS_InventoryProfileData` via `StaticStruct()->SerializeItem(...)` (same approach as Stats/SkillTree), using:
- `USOTS_InventoryBridgeSubsystem::BuildProfileData(...)`
- `USOTS_InventoryBridgeSubsystem::ApplyProfileData(...)`

## How to enable (Ryan)
1) Enable Core save participant queries (Core setting).
2) Enable `SOTS INV Core Bridge` setting (`bEnableSOTSCoreSaveParticipantBridge`).
3) Optional: enable verbose logs (`bEnableSOTSCoreBridgeVerboseLogs`).
4) Run `SOTS.Core.DumpSaveParticipants` or `SOTS.Core.BridgeHealth` and confirm `INV` is registered.

## Notes
- If the subsystem cannot be resolved from any world, build/apply returns false (and logs Verbose if enabled).
