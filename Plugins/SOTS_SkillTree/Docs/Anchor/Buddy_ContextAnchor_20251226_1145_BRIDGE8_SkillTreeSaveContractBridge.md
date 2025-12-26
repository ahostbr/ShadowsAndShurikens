Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1145 | Plugin: SOTS_SkillTree | Pass/Topic: BRIDGE8_CoreSaveParticipant | Owner: Buddy
Scope: Optional SOTS_Core Save Contract bridge for SkillTree (opaque fragment), defaults OFF.

DONE
- Added `USOTS_SkillTreeCoreBridgeSettings` (DefaultConfig) with `bEnableSOTSCoreSaveParticipantBridge=false` and `bEnableSOTSCoreBridgeVerboseLogs=false`.
- Added `FSkillTree_SaveParticipant : ISOTS_CoreSaveParticipant` emitting/applying fragment `SkillTree.State` by serializing `FSOTS_SkillTreeProfileData`.
- Wired registration/unregistration in `FSOTS_SkillTreeModule` startup/shutdown behind the setting.
- Added SOTS_Core dependency in SkillTree Build.cs and SkillTree.uplugin.
- Added bridge documentation.

VERIFIED
- None (no Unreal build/editor/runtime run in this pass).

UNVERIFIED / ASSUMPTIONS
- Assumes `FSOTS_SkillTreeProfileData` serialization via `StaticStruct()->SerializeItem` is stable for current schema and sufficient for Core fragment persistence.
- Assumes resolving `USOTS_SkillTreeSubsystem` via `GEngine->GetWorldContexts()` is acceptable in all runtime scenarios.

FILES TOUCHED
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/SOTS_SkillTree.Build.cs
- Plugins/SOTS_SkillTree/SOTS_SkillTree.uplugin
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Public/SOTS_SkillTreeCoreBridgeSettings.h
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreBridgeSettings.cpp
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.h
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.cpp
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeModule.cpp
- Plugins/SOTS_SkillTree/Docs/SOTS_SkillTree_SOTSCoreSaveContractBridge.md

NEXT (Ryan)
- Build and confirm `SOTS_SkillTree` loads with the added `SOTS_Core` dependency.
- Enable `USOTS_CoreSettings::bEnableSaveParticipantQueries` and SkillTree bridge setting; run `SOTS.Core.BridgeHealth` and confirm `SkillTree` participant is registered.
- Validate a save/load cycle restores unlocked skills + points and correctly reapplies tags.

ROLLBACK
- Disable the SkillTree bridge setting and revert the touched files listed above.
