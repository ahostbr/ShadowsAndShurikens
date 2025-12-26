# BuddyWorklog_20251225_SOTS_Core_SPINE8

## Prompt Identifier
SOTS_VSCODEBUDDY_CODEX_MAX_SOTS_CORE_SPINE8

## Goal
Add compile-guarded automation tests for lifecycle dispatch gating, duplicate suppression, and map travel notification behavior (no editor/runtime dependency).

## What Changed
- Added WITH_AUTOMATION_TESTS-only tests for dispatch gating, duplicate suppression, snapshot-before-dispatch ordering, and map travel duplicate suppression.
- Documented SPINE8 test scope in the SOTS_Core overview.

## Files Changed
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Tests/SOTS_CoreLifecycleTests.cpp
- Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md

## Notes / Decisions (RepoIntel Evidence)
- No existing automation test macros found in repo (rg for IMPLEMENT_SIMPLE_AUTOMATION_TEST returned no hits).
- AutomationTest header usage example: Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSTestAdapterCommandlet.h:8.
- Core delegate availability (used in subsystem, no extra modules): Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp:25,39.

## Verification Notes
- Not run (per constraints: no builds/runs).

## Cleanup Confirmation
- Plugins/SOTS_Core/Binaries: not present.
- Plugins/SOTS_Core/Intermediate: not present.

## Follow-ups / TODOs
- Map travel bridge binding/unbinding handle tests are not covered; requires engine-managed subsystem init or test-only access to delegate handles.
