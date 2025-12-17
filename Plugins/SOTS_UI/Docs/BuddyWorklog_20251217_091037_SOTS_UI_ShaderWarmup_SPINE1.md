# Buddy Worklog - SOTS_UI Shader Warmup SPINE1 (2025-12-17 09:10:37)

**Goal**: Land SPINE1 shader warmup surfaces in SOTS_UI (types + subsystem plumbing + MoviePlayer stub) so MissionDirector can BeginWarmup and listen for OnFinished.

**Changes**
- Added shader warmup types (request + source mode + delegates) and load list data asset class.
- Added shader warmup subsystem that pushes the loading screen, applies global time dilation freeze, arms MoviePlayer hooks, and broadcasts a stub progress + finished.
- Added shader warmup settings pointer in SOTS_UISettings and loading intent tag accessors in SOTS_UIIntentTags.
- Added tags doc, plus widget registry seed entry for SAS.UI.Screen.Loading.ShaderWarmup.
- Added MoviePlayer dependency in SOTS_UI.Build.cs.

**Files touched**
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupTypes.h
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupLoadListDataAsset.h
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UISettings.h
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIIntentTags.h
- Plugins/SOTS_UI/Source/SOTS_UI/SOTS_UI.Build.cs
- Plugins/SOTS_UI/Docs/SOTS_UI_ShaderWarmup_TagsToAdd.md
- Plugins/SOTS_UI/Docs/ProHUDV2_WidgetRegistrySeed.json

**Verification**
- Not run (code-only change; no build executed per SOTS law).

**Cleanup**
- Deleted Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate after edits.

**Follow-ups**
- Wire the actual UMG widget asset + registry entry in-game and connect MissionDirector to BeginWarmup/EndWarmup.
- Implement SPINE2 asset warmup logic (load list + level referenced fallback).
