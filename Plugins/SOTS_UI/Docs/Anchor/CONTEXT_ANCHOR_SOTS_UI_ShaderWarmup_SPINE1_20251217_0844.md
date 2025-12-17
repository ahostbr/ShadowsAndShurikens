[CONTEXT_ANCHOR]
ID: 20251217_0844 | Plugin: SOTS_UI | Pass/Topic: ShaderWarmup_SPINE1 | Owner: Buddy
Scope: SPINE_1 shader warmup surface in SOTS_UI (subsystem + progress widget + tags).

DONE
- Added FSOTS_ShaderWarmupRequest and USOTS_ShaderWarmupSubsystem with BeginWarmup stub that pushes the router widget and broadcasts completion.
- Added USOTS_ShaderWarmupProgressWidget base class that binds to warmup delegates (no tick).
- Added gameplay tags for shader warmup screen + loading intents/modes.
- Added AssetRegistry module dependency for upcoming warmup asset queries.

VERIFIED
- UNVERIFIED (no build/run performed in this pass).

UNVERIFIED / ASSUMPTIONS
- Widget registry entry and UMG blueprint for SAS.UI.Screen.Loading.ShaderWarmup will be wired later.
- MissionDirector will own travel and react to OnFinished.

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupProgressWidget.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupProgressWidget.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/SOTS_UI.Build.cs
- Config/DefaultGameplayTags.ini
- Plugins/SOTS_UI/Docs/BuddyWorklog_20251217_084409_SOTS_UI_ShaderWarmup_SPINE1.md
- Plugins/SOTS_UI/Docs/Anchor/CONTEXT_ANCHOR_SOTS_UI_ShaderWarmup_SPINE1_20251217_0844.md

NEXT (Ryan)
- Add the widget registry entry and UMG blueprint for W_SOTS_ShaderWarmupProgress.
- Confirm warmup policy (shipping/editor scope, asset list defaults, MoviePlayer persistence).
- Wire MissionDirector to BeginWarmup and handle OnFinished.

ROLLBACK
- Revert the files listed above and remove the added gameplay tags to undo the SPINE_1 surface.
