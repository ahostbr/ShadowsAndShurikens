[CONTEXT_ANCHOR]
ID: 20251217_1603 | Plugin: SOTS_UI | Pass/Topic: WarmupDependencyAPI | Owner: Buddy
Scope: UE5.7p compile fixes for ShaderWarmup (asset registry API, UE_UNUSED, router pop access).

DONE
- Updated shader warmup fallback dependency scan to use UE::AssetRegistry::EDependencyCategory::Package with FDependencyQuery.
- Replaced UE_UNUSED with (void) casts in ShaderWarmup subsystem handlers.
- Exposed PopWidgetById as public BlueprintCallable on UIRouter for warmup teardown to pop its widget.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Asset registry call now matches UE5.7p signature; warmup teardown continues to pop only its widget.

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h

NEXT (Ryan)
- Rebuild SOTS_UI on UE5.7p; if new errors appear, share exact signatures required by the engine build.
- Validate warmup flow: start warmup, ensure loading widget appears and is popped on completion/cancel.

ROLLBACK
- Revert the two touched files.
