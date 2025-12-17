[CONTEXT_ANCHOR]
ID: 20251217_1000 | Plugin: SOTS_UI | Pass/Topic: ShaderWarmup_SPINE2 | Owner: Buddy
Scope: Real shader warmup run (load list + level fallback, async load/touch, compile polling, time dilation restore).

DONE
- Confirmed SOTS_UI already includes Niagara + RenderCore dependencies for warmup asset touch and PSO polling.
- Implemented asset queue build using load list data asset with level dependency fallback (materials + Niagara), de-duped.
- Implemented sequential async load with transient touch actor/components and progress updates.
- Added compile/PSO polling via FTSTicker and finish flow that restores time dilation and pops the widget.

VERIFIED
- UNVERIFIED (no build/run performed in this pass).

UNVERIFIED / ASSUMPTIONS
- Warmup widget binds to OnProgress/OnFinished in UMG.
- Default shader warmup load list asset is configured in project settings.

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
- Plugins/SOTS_UI/Docs/BuddyWorklog_20251217_100058_SOTS_UI_ShaderWarmup_SPINE2.md
- Plugins/SOTS_UI/Docs/Anchor/CONTEXT_ANCHOR_SOTS_UI_ShaderWarmup_SPINE2_20251217_1000.md

NEXT (Ryan)
- Hook MissionDirector BeginWarmup/EndWarmup and confirm UI updates during warmup.
- Validate warmup in shipping to confirm PSO polling path behaves as expected.

ROLLBACK
- Revert the files listed above to remove the SPINE2 shader warmup implementation.
[/CONTEXT_ANCHOR]
