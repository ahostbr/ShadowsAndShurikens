# Buddy Worklog — 20251226_200319 — Log: LNK1120 UDeveloperSettings unresolved externals

## Goal
Resolve build-stopping linker failures observed in the latest editor build log: multiple plugins failing to link due to unresolved `UDeveloperSettings` symbols.

## Evidence (log)
- Log: `Saved/Logs/ShadowsAndShurikens.log`
- Linker failures (examples):
  - `UnrealEditor-SOTS_GlobalStealthManager.dll : fatal error LNK1120: 12 unresolved externals`
  - `UnrealEditor-SOTS_Stats.dll : fatal error LNK1120: 13 unresolved externals`
  - `UnrealEditor-SOTS_INV.dll : fatal error LNK1120: 12 unresolved externals`
  - `UnrealEditor-SOTS_AIPerception.dll : fatal error LNK1120: 12 unresolved externals`
  - `UnrealEditor-SOTS_SkillTree.dll : fatal error LNK1120: 13 unresolved externals`
  - `UnrealEditor-SOTS_KillExecutionManager.dll : fatal error LNK1120: 13 unresolved externals`
  - `UnrealEditor-SOTS_MissionDirector.dll : fatal error LNK1120: 12 unresolved externals`
- Representative unresolved symbol family:
  - `Z_Construct_UClass_UDeveloperSettings...`, `UDeveloperSettings::UDeveloperSettings(...)`, `UDeveloperSettings::~UDeveloperSettings()`, and related virtuals.

## Root cause (inferred from log + code)
Each failing module defines a `UDeveloperSettings`-derived Bridge Settings class (e.g., `USOTS_*CoreBridgeSettings`) but its module rules did not link against the Engine `DeveloperSettings` module. This produces unresolved externals at link time.

## What changed
Add-only fix: added `"DeveloperSettings"` to `PublicDependencyModuleNames` in the affected plugin `*.Build.cs` files.

## Files changed
- Modified:
  - `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/SOTS_GlobalStealthManager.Build.cs`
  - `Plugins/SOTS_Stats/Source/SOTS_Stats/SOTS_Stats.Build.cs`
  - `Plugins/SOTS_INV/Source/SOTS_INV/SOTS_INV.Build.cs`
  - `Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/SOTS_AIPerception.Build.cs`
  - `Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/SOTS_SkillTree.Build.cs`
  - `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/SOTS_KillExecutionManager.Build.cs`
  - `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/SOTS_MissionDirector.Build.cs`

## Constraints check
- Add-only: yes (dependency list additions only).
- No Blueprint edits.
- No Unreal build/run performed by Buddy in this prompt.

## Cleanup status (Binaries/Intermediate)
Per SOTS law, removed `Binaries/` and `Intermediate/` folders for the plugins touched by this prompt (when present).

## Verification status
- UNVERIFIED: full rebuild/editor startup after these changes (Ryan-run). The expectation is these changes eliminate the `UDeveloperSettings` linker errors in the listed modules.

## Next steps (Ryan)
- Re-run the build/start editor and confirm `LNK2019/LNK1120` for `UDeveloperSettings` is resolved across the affected modules.
- Remaining non-blocker items still visible in the log:
  - `Engine version string ... could not be parsed ("5.7")` warnings.
  - `StructUtils` deprecation warnings.
  - `SOTS_GlobalStealthManagerSubsystem.cpp` warning C4996 re `RemoveAt` shrink enum migration.
