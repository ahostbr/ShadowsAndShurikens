# Buddy Worklog 2025-12-25 - SOTS_Core SPINE6

## RepoIntel Evidence (save terminology + duplication check)
- Save terminology uses "snapshot/provider/priority" in ProfileShared policy:
  - Plugins/SOTS_ProfileShared/Docs/SOTS_ProfileShared_SnapshotPolicy.md:13-14
  - Plugins/SOTS_ProfileShared/Docs/SOTS_ProfileShared_SnapshotPolicy.md:34
- No existing save-contract interface found (rg: "SaveParticipant|SaveContract|SaveFragment|SavePayload|CoreSaveParticipant") -> NONE.

## Settings Added (defaults)
- bEnableSaveParticipantQueries = false
- bEnableSaveContractLogs = false

## Files Created/Modified
- Created:
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Save/SOTS_SaveTypes.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Save/SOTS_CoreSaveParticipant.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Save/SOTS_CoreSaveParticipantRegistry.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/Save/SOTS_CoreSaveParticipantRegistry.cpp
  - Plugins/SOTS_Core/Docs/SOTS_Core_SaveContract.md
- Modified:
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Settings/SOTS_CoreSettings.h
