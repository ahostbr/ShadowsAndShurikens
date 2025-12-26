# BuddyWorklog_20251225_SOTS_Core_SPINE7

## Prompt Identifier
SOTS_VSCODEBUDDY_CODEX_MAX_SOTS_CORE_SPINE7

## Goal
Add command-invoked diagnostics (console commands + health report) for SOTS_Core without any runtime spam or new dependencies.

## What Changed
- Added console command registration/unregistration in the SOTS_Core module (shipping/test gated).
- Wired command handlers to the SOTS_Core diagnostics helper to dump settings, snapshots, listeners, save participants, and health.
- Documented the SPINE7 diagnostics commands in the SOTS_Core overview.

## Files Changed
- Plugins/SOTS_Core/Source/SOTS_Core/Private/SOTS_Core.cpp
- Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md

## Notes / Decisions (RepoIntel Evidence)
- Log category evidence: Plugins/SOTS_Core/Source/SOTS_Core/Public/SOTS_Core.h:5 and Plugins/SOTS_Core/Source/SOTS_Core/Private/SOTS_Core.cpp:8.
- Console command patterns: Plugins/SOTS_Input/Source/SOTS_Input/Private/SOTS_InputConsoleCommands.cpp:15,98 and Plugins/BEP/Source/BEP/Private/BEP.cpp:54.
- Commands are registered only when not in shipping/test builds and only execute on-demand.

## Verification Notes
- Not run (per constraints: no builds/runs).

## Cleanup Confirmation
- Plugins/SOTS_Core/Binaries: not present.
- Plugins/SOTS_Core/Intermediate: not present.

## Follow-ups / TODOs
- None.
