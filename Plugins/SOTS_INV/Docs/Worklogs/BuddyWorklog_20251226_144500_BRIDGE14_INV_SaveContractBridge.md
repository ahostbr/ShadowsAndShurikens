# Buddy Worklog — 20251226_144500 — BRIDGE14 INV Save Contract Bridge

## Goal
Implement BRIDGE14: register SOTS_INV as an optional `ISOTS_CoreSaveParticipant` that emits/applies an opaque inventory fragment via `SOTS_Core` Save Contract.

## Evidence (RepoIntel)
- Core save contract interface + feature name:
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Save/SOTS_CoreSaveParticipant.h` (defines `ISOTS_CoreSaveParticipant`, `SOTS_CoreSaveParticipantFeatureName`)
- Core registry helper used by other participants:
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Save/SOTS_CoreSaveParticipantRegistry.h`
- Proven inventory snapshot seam:
  - `Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryBridgeSubsystem.h` (`BuildProfileData`, `ApplyProfileData`)
  - `Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryBridgeSubsystem.cpp` (`BuildProfileData` around 745+, `ApplyProfileData` around 784+)
- Proven reflected struct used for snapshot bytes:
  - `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileTypes.h` (`USTRUCT` `FSOTS_InventoryProfileData` around 126+)

## What Changed
- Added SOTS_Core dependency wiring to SOTS_INV.
- Added `USOTS_INVCoreBridgeSettings` (default OFF) to gate bridge enablement and verbose logs.
- Added `FINV_SaveParticipant : ISOTS_CoreSaveParticipant`:
  - `GetParticipantId()` -> `INV`
  - `BuildSaveFragment()` -> serializes `FSOTS_InventoryProfileData` into bytes, fragment id `INV.Inventory`
  - `ApplySaveFragment()` -> deserializes bytes and calls `ApplyProfileData`
- Updated SOTS_INV module startup/shutdown to register/unregister participant when enabled (mirrors Stats/SkillTree pattern via `FSOTS_CoreSaveParticipantRegistry`).

## Files Changed / Added
- Added: `Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_INVCoreBridgeSettings.h`
- Added: `Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_INVCoreBridgeSettings.cpp`
- Added: `Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_INVCoreSaveParticipant.h`
- Added: `Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_INVCoreSaveParticipant.cpp`
- Updated: `Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_INVModule.cpp`
- Updated: `Plugins/SOTS_INV/Source/SOTS_INV/SOTS_INV.Build.cs`
- Updated: `Plugins/SOTS_INV/SOTS_INV.uplugin`
- Added: `Plugins/SOTS_INV/Docs/SOTS_INV_SOTSCoreSaveContractBridge.md`

## Notes / Risks / UNKNOWNs
- UNVERIFIED: No Unreal build/run performed. Compile success not confirmed.
- Assumption: `FSOTS_InventoryProfileData::StaticStruct()->SerializeItem(...)` is stable for this struct (it is a reflected `USTRUCT`).

## Verification Status
- UNVERIFIED (per project rules): no build/editor run.

## Cleanup
- Deleted `Plugins/SOTS_INV/Binaries` and `Plugins/SOTS_INV/Intermediate` (if present) after edits.

## Next (Ryan)
- Enable Core save participant queries + INV bridge setting.
- Run `SOTS.Core.BridgeHealth` or `SOTS.Core.DumpSaveParticipants` and confirm participant id `INV`.
- Exercise save/load path that requests fragments and verify inventory restores correctly.
