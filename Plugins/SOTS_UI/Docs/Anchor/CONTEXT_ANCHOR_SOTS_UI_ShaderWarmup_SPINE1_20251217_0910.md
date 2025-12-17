[CONTEXT_ANCHOR]
ID: 20251217_0910 | Plugin: SOTS_UI | Pass/Topic: ShaderWarmup_SPINE1 | Owner: Buddy
Scope: SPINE1 shader warmup surface (types, subsystem plumbing, MoviePlayer stub, tags/doc updates).

DONE
- Added shader warmup types (request + source mode + delegates) and load list data asset class.
- Added shader warmup subsystem that pushes the loading screen, applies global time dilation freeze, arms MoviePlayer hooks, and broadcasts a stub progress + finished.
- Added shader warmup settings pointer in SOTS_UISettings and loading intent tag accessors in SOTS_UIIntentTags.
- Added tags doc and widget registry seed entry for SAS.UI.Screen.Loading.ShaderWarmup.
- Added MoviePlayer dependency in SOTS_UI.Build.cs.

VERIFIED
- UNVERIFIED (no build/run performed in this pass).

UNVERIFIED / ASSUMPTIONS
- DefaultGameplayTags.ini already contains the shader warmup tags; no tag changes were made here.
- MissionDirector will own travel and call EndWarmup when appropriate.

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupTypes.h
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupLoadListDataAsset.h
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UISettings.h
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIIntentTags.h
- Plugins/SOTS_UI/Source/SOTS_UI/SOTS_UI.Build.cs
- Plugins/SOTS_UI/Docs/SOTS_UI_ShaderWarmup_TagsToAdd.md
- Plugins/SOTS_UI/Docs/ProHUDV2_WidgetRegistrySeed.json
- Plugins/SOTS_UI/Docs/BuddyWorklog_20251217_091037_SOTS_UI_ShaderWarmup_SPINE1.md
- Plugins/SOTS_UI/Docs/Anchor/CONTEXT_ANCHOR_SOTS_UI_ShaderWarmup_SPINE1_20251217_0910.md

NEXT (Ryan)
- Wire the widget registry entry + UMG asset for W_SOTS_ShaderWarmupProgress.
- Hook MissionDirector to BeginWarmup/EndWarmup and listen for OnFinished.
- Implement SPINE2 load list + level referenced fallback warmup logic.

ROLLBACK
- Revert the files listed above to remove the SPINE1 shader warmup surface.
[/CONTEXT_ANCHOR]
