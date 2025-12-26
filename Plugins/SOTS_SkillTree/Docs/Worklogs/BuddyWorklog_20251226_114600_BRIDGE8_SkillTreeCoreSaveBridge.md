# Buddy Worklog — 20251226_114600 — BRIDGE8 SkillTree Core Save Bridge

## Goal
Implement BRIDGE8: bridge `SOTS_SkillTree` into the `SOTS_Core` save contract as an opt-in `SOTS.CoreSaveParticipant` emitting/applying an opaque fragment.

## What changed
- Added `SOTS_Core` dependency wiring for `SOTS_SkillTree` (Build.cs + .uplugin).
- Added new DeveloperSettings surface `USOTS_SkillTreeCoreBridgeSettings` (defaults OFF).
- Implemented `FSkillTree_SaveParticipant`:
  - `ParticipantId="SkillTree"`
  - `FragmentId="SkillTree.State"`
  - Builds/applies payload via reflected serialization of `FSOTS_SkillTreeProfileData` using the existing `USOTS_SkillTreeSubsystem::BuildProfileData/ApplyProfileData` seam.
  - On apply, broadcasts `OnSkillTreeStateChanged` for each runtime state after restoring.
- Registered/unregistered the participant in `FSOTS_SkillTreeModule::StartupModule/ShutdownModule` when enabled.

## Files changed
- Plugins/SOTS_SkillTree/SOTS_SkillTree.uplugin
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/SOTS_SkillTree.Build.cs
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Public/SOTS_SkillTreeCoreBridgeSettings.h
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreBridgeSettings.cpp
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.h
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.cpp
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeModule.cpp
- Plugins/SOTS_SkillTree/Docs/SOTS_SkillTree_SOTSCoreSaveContractBridge.md

## Notes / Risks / UNKNOWN
- Payload choice: this bridge serializes `FSOTS_SkillTreeProfileData` (unlocked skill tags + unspent points) because `ApplyProfileData` performs tag re-sync.
  - `SaveSkillTreeState/LoadSkillTreeState` would preserve per-tree runtime state but does NOT reapply tags (per SkillTree subsystem comment/implementation).
  - This means per-tree point pools are not preserved (SkillTree profile data uses a single `UnspentSkillPoints`). This appears to be the intended canonical persistence representation.
- Broadcast behavior: on apply, the bridge broadcasts `OnSkillTreeStateChanged` for each tree after restoring. This avoids FX and does not attempt per-node unlock broadcasts (no safe mapping tag→node id).

## Verification status
- UNVERIFIED: No editor run / build was performed (per SOTS laws). Changes are code-reviewed against existing subsystem seams.

## Cleanup
- Planned/Performed: remove `Binaries/` and `Intermediate/` for touched plugins (SkillTree + Core) after code changes.

## Next (Ryan)
- Enable `bEnableSaveParticipantQueries` in `SOTS_Core` settings and `bEnableSOTSCoreSaveParticipantBridge` in SkillTree bridge settings.
- Run save flow that uses `SOTS.CoreSaveParticipant` registry; confirm `SkillTree.State` fragment is present.
- Load that save and confirm:
  - SkillTree unlock state restored
  - TagManager contains unlocked skill tags
  - UI listening to `OnSkillTreeStateChanged` refreshes correctly
