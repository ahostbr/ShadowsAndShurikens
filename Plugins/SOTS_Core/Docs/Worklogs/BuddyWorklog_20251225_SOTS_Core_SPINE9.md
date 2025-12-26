# BuddyWorklog_20251225_SOTS_Core_SPINE9

## Prompt Identifier
SOTS_VSCODEBUDDY_CODEX_MAX_SOTS_CORE_SPINE9

## Goal
Add version constants, a config schema guard + migration hook, and log-only sanity checks without changing runtime behavior.

## What Changed
- Added SOTS_Core version constants header (semantic version + config schema version).
- Introduced settings schema version with migration hooks (log-only, no behavior changes).
- Added diagnostics validation for map travel binding, empty dispatch, and duplicate-suppression warnings.
- Documented SPINE9 versioning/migration notes in the overview.

## Files Changed
- Plugins/SOTS_Core/Source/SOTS_Core/Public/SOTS_CoreVersion.h
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Settings/SOTS_CoreSettings.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Settings/SOTS_CoreSettings.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Diagnostics/SOTS_CoreDiagnostics.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Diagnostics/SOTS_CoreDiagnostics.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md

## Notes / Decisions (RepoIntel Evidence)
- Version header pattern: Plugins/DismembermentSystem/Source/DismembermentSystem/Public/Version/DismembermentVersioning.h:10,12.
- Snapshot version constant pattern: Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileTypes.h:7.
- No existing config schema version in suite found during rg scan (see search results).

## Verification Notes
- Not run (per constraints: no builds/runs).

## Cleanup Confirmation
- Plugins/SOTS_Core/Binaries: not present.
- Plugins/SOTS_Core/Intermediate: not present.

## Follow-ups / TODOs
- None.
