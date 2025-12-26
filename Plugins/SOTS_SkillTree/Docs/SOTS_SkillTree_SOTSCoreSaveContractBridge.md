# SOTS_SkillTree ↔ SOTS_Core Save Contract Bridge (BRIDGE8)

## Goal
Provide an **opt-in** bridge that registers SOTS_SkillTree as a `SOTS.CoreSaveParticipant` so `SOTS_Core` can query SkillTree save status and request an opaque save fragment.

This bridge is **disabled by default**.

## Settings
Project Settings:
- **Plugins → SOTS SkillTree Core Bridge**
  - `bEnableSOTSCoreSaveParticipantBridge` (default: `false`)
  - `bEnableSOTSCoreBridgeVerboseLogs` (default: `false`)

## Save Participant
- ParticipantId: `SkillTree`
- FragmentId: `SkillTree.State`

### Payload format
The payload bytes are a reflected UStruct serialization of `FSOTS_SkillTreeProfileData` (from `SOTS_ProfileShared`).

This uses the existing SkillTree persistence seam:
- `USOTS_SkillTreeSubsystem::BuildProfileData(FSOTS_SkillTreeProfileData&)`
- `USOTS_SkillTreeSubsystem::ApplyProfileData(const FSOTS_SkillTreeProfileData&)`

Notably, `ApplyProfileData` will **reapply global gameplay tags** for unlocked skills via the TagManager, matching the subsystem’s intended save/load path.

## Notes / Risks / UNKNOWN
- This bridge intentionally serializes **profile-level** SkillTree data (unlocked skill tags + unspent points) rather than the more detailed `FSOTS_SkillTreeProfileState` runtime dump.
- SkillTree’s `SaveSkillTreeState/LoadSkillTreeState` helpers do not re-sync gameplay tags; this bridge avoids that path to keep tag state correct.
- Apply does **not** call `UnlockNode/TryUnlockNode` (so it does not directly spawn unlock FX). It restores state via `ApplyProfileData`.

## Verification (manual)
- Enable `USOTS_CoreSettings::bEnableSaveParticipantQueries` so Core diagnostics can enumerate participants.
- Enable `bEnableSOTSCoreSaveParticipantBridge`.
- Run `SOTS.Core.BridgeHealth` and confirm `SkillTree` appears in the participant list.

## Rollback
- Disable `bEnableSOTSCoreSaveParticipantBridge`.
- No Blueprint changes are involved.

## Files
- `Source/SOTS_SkillTree/Public/SOTS_SkillTreeCoreBridgeSettings.h`
- `Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreBridgeSettings.cpp`
- `Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.h`
- `Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.cpp`
- `Source/SOTS_SkillTree/Private/SOTS_SkillTreeModule.cpp`
