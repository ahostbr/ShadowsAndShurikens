# Buddy Worklog — 2025-12-19 15:09:00

## Goal
Fix BlueprintGen unity-build collisions causing ambiguous helpers and duplicate symbol errors.

## What changed
- Prefixed BPGenDebug helper functions (load graph/node id/match/annotation) to avoid colliding with builder helpers in unity builds.
- Prefixed BPGenInspector helpers (node ids, pin/node summaries, sorters, graph loaders/savers) and redirected all call sites.
- Prefixed BPGenGraphResolver error constants to avoid clashing with Ensure constants when compiled in unity.
- Cleaned plugin `Binaries/` and `Intermediate/` after code edits.

## Files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDebug.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenGraphResolver.cpp
- Plugins/SOTS_BlueprintGen/Binaries (deleted)
- Plugins/SOTS_BlueprintGen/Intermediate (deleted)

## Notes / risks / unknowns
- No functional behavior change intended; only symbol scoping/prefix adjustments. Build not yet re-run.

## Verification
- Not run (no build/editor).

## Follow-ups / next steps
- Rebuild SOTS_BlueprintGen to confirm ambiguous overload/redefinition errors are gone.
- Spot-check BPGenDebug/Inspector functionality in editor if needed after build.
