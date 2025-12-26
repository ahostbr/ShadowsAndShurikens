Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1146 | Plugin: SOTS_SkillTree | Pass/Topic: BRIDGE8_CoreSaveParticipant | Owner: Buddy
Scope: Opt-in SkillTree bridge to SOTS_Core save contract (fragment build/apply).

DONE
- Added `USOTS_SkillTreeCoreBridgeSettings` (defaults OFF).
- Added `FSkillTree_SaveParticipant` registered as `SOTS.CoreSaveParticipant` when enabled.
- FragmentId `SkillTree.State` stores bytes for `FSOTS_SkillTreeProfileData` using reflected serialization.
- Module wiring: Startup registers / Shutdown unregisters via `FSOTS_CoreSaveParticipantRegistry`.
- Dependency wiring: added `SOTS_Core` to SkillTree Build.cs + .uplugin plugin deps.

VERIFIED
- UNVERIFIED (no build/editor run performed).

UNVERIFIED / ASSUMPTIONS
- Assumes `FSOTS_SkillTreeProfileData` is the intended canonical persistence slice because `ApplyProfileData` re-syncs tags.
- Assumes broadcasting `OnSkillTreeStateChanged` post-apply is safe for UI refresh and does not trigger unintended FX.

FILES TOUCHED
- Plugins/SOTS_SkillTree/SOTS_SkillTree.uplugin
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/SOTS_SkillTree.Build.cs
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Public/SOTS_SkillTreeCoreBridgeSettings.h
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreBridgeSettings.cpp
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.h
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.cpp
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeModule.cpp
- Plugins/SOTS_SkillTree/Docs/SOTS_SkillTree_SOTSCoreSaveContractBridge.md
- Plugins/SOTS_SkillTree/Docs/Worklogs/BuddyWorklog_20251226_114600_BRIDGE8_SkillTreeCoreSaveBridge.md

NEXT (Ryan)
- Enable `SOTS_Core` setting for save-participant queries and enable SkillTree bridge setting.
- Trigger save and confirm `SkillTree.State` fragment is produced and persisted.
- Trigger load and confirm:
  - tags re-applied (unlocked skill tags present on player)
  - UI refreshes via `OnSkillTreeStateChanged`

ROLLBACK
- Revert the files listed under FILES TOUCHED.
