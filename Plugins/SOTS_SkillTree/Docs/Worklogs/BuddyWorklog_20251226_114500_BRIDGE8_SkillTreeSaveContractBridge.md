# Buddy Worklog — BRIDGE8 SkillTree Save Contract Bridge

Date: 2025-12-26
Owner: Buddy
Scope: Plugins/SOTS_SkillTree (+ cleanup of SOTS_Core build artifacts)

## Goal
Implement BRIDGE8: register SOTS_SkillTree as an optional `ISOTS_CoreSaveParticipant` that emits/applies an opaque fragment representing unlocked skills + points, **disabled-by-default**, with no IO and no BP edits.

## Evidence-first notes
- SkillTree already exposes profile snapshot seams:
  - `USOTS_SkillTreeSubsystem::BuildProfileData(FSOTS_SkillTreeProfileData&)`
  - `USOTS_SkillTreeSubsystem::ApplyProfileData(const FSOTS_SkillTreeProfileData&)`
- Authoritative runtime state is maintained by `USOTS_SkillTreeSubsystem` and can be queried via `GetAllRuntimeStates()`.

## What changed
- Added a new DeveloperSettings surface for enabling the bridge (defaults OFF).
- Added a new `ISOTS_CoreSaveParticipant` implementation that:
  - emits `FragmentId="SkillTree.State"` by serializing `FSOTS_SkillTreeProfileData` to bytes
  - applies the fragment by deserializing bytes and calling `USOTS_SkillTreeSubsystem::ApplyProfileData`
  - broadcasts `OnSkillTreeStateChanged` per restored runtime state
- Registered/unregistered the participant in `FSOTS_SkillTreeModule` startup/shutdown when enabled.
- Declared SOTS_Core dependency (Build.cs + .uplugin).

## Files changed/created
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/SOTS_SkillTree.Build.cs — add `SOTS_Core` private dependency.
- Plugins/SOTS_SkillTree/SOTS_SkillTree.uplugin — add plugin dependency on `SOTS_Core`.
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Public/SOTS_SkillTreeCoreBridgeSettings.h — new settings class (defaults OFF).
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreBridgeSettings.cpp — settings defaults.
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.h — participant declaration.
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeCoreSaveParticipant.cpp — participant implementation.
- Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Private/SOTS_SkillTreeModule.cpp — registration/unregistration wiring.
- Plugins/SOTS_SkillTree/Docs/SOTS_SkillTree_SOTSCoreSaveContractBridge.md — bridge documentation.

## Notes / risks / unknowns
- UNVERIFIED: No Unreal build/editor/runtime run was performed.
- The participant resolves the SkillTree subsystem by scanning `GEngine->GetWorldContexts()`. This matches the existing Stats participant strategy and avoids coupling to project-level types.
- Apply path broadcasts `OnSkillTreeStateChanged` to refresh listeners; it intentionally does **not** broadcast per-node unlock events to avoid unintentionally triggering listener-side FX.

## Verification status
- Static check only: no VS Code problem list errors reported for the touched C++ files during this pass.

## Cleanup
- Required by SOTS laws / BRIDGE8: delete `Binaries/` and `Intermediate/` under:
  - Plugins/SOTS_SkillTree
  - Plugins/SOTS_Core

## Next (Ryan)
- Build the project and confirm module loads.
- In editor/runtime, enable:
  - Core setting: `bEnableSaveParticipantQueries`
  - SkillTree setting: `bEnableSOTSCoreSaveParticipantBridge`
- Run `SOTS.Core.BridgeHealth` and confirm `SkillTree` participant appears.
- Exercise save/load path that consumes Core save fragments and confirm skill tags/points restore correctly.
