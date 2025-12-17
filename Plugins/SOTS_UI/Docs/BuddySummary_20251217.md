# Buddy Summary — SOTS_UI fixes (2025-12-17)

## Changes made
- Shader warmup include: now rely on UObject/UObjectGlobals.h for FCoreUObjectDelegates (removed missing header guard).
- Shader warmup asset scan: updated AssetRegistry dependency query to UE5.7p API using Package category with hard+soft flags via FDependencyQuery().
- Shader warmup unused params: replaced UE_UNUSED with (void) casts.
- UI router access: exposed PopWidgetById as BlueprintCallable to allow warmup teardown to pop its loading widget.

## Files touched
- Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
- Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Docs/BuddyWorklog_20251217_155000_warmup_include2.md
- Docs/BuddyWorklog_20251217_160200_warmup_dependencies.md
- Docs/Anchor/Anchor_20251217_1551_WarmupDelegateInclude_v2.md
- Docs/Anchor/Anchor_20251217_1603_WarmupDependencyAPI.md

## Verification
- UNVERIFIED (no build/run executed). Ryan to rebuild on UE 5.7p.

## Cleanup
- Attempted to delete Binaries/Intermediate; DLL was locked. Close editor/UBT then delete Plugins/SOTS_UI/Binaries/Win64 manually if needed.
