# Buddy Worklog — 2025-12-26 10:35 — SOTS_Core BRIDGE5 (BridgeHealth diagnostics)

## Prompt Identifier
SOTS_VSCODEBUDDY_CODEX_MAX_SOTS_CORE_BRIDGE5

## Goal
Add a command-driven diagnostics audit so Ryan can quickly verify BRIDGE1–BRIDGE4 wiring from SOTS_Core:
- Are lifecycle listeners registered? (and how many)
- Are save participants registered? (and how many)
- Are relevant Core settings enabled (dispatch / delegate bridges / save query gate)
- What does the lifecycle snapshot currently look like (World/PC/Pawn/HUD/PrimaryReady)

## RepoIntel Evidence (RAG-first)
- Existing diagnostics console commands are registered in:
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/SOTS_Core.cpp (see existing `SOTS.Core.*` commands)
- Lifecycle listener modular feature name constant:
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListener.h:14 (`SOTS_CoreLifecycleListenerFeatureName`)
- Save participant modular feature name constant:
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Save/SOTS_CoreSaveParticipant.h:7 (`SOTS_CoreSaveParticipantFeatureName`)

## What changed
- Added a new diagnostics entrypoint `FSOTS_CoreDiagnostics::DumpBridgeHealth(UWorld*)`.
- Added a new console command `SOTS.Core.BridgeHealth` registered alongside existing SPINE7 commands.
- Updated SOTS_Core overview docs to include the new command and describe what it prints.

## Output behavior
`SOTS.Core.BridgeHealth` prints:
- Core version + schema information.
- Core settings flags relevant to bridges.
- Lifecycle listener list (pointer + best-effort type name when RTTI is available; origin may be UNKNOWN).
- Save participant list (participant id + pointer + best-effort type name; reports what is registered even if queries are disabled).
- Snapshot summary (World/PC/Pawn/HUD/PrimaryReady) when a world context is available.

## Notes / Risks / Unknowns
- Origin identification is best-effort; module/plugin attribution may remain `UNKNOWN`.
- Type identification depends on RTTI availability (`PLATFORM_COMPILER_HAS_RTTI`); otherwise prints `UNKNOWN`.
- No runtime/editor verification performed (per constraints).

## Files changed
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Diagnostics/SOTS_CoreDiagnostics.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Diagnostics/SOTS_CoreDiagnostics.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/SOTS_Core.cpp
- Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md

## Verification Notes
- Not run (per constraints: no builds/runs).

## Cleanup Confirmation
- SOTS_Core plugin Binaries/ and Intermediate/ removed (if present).
