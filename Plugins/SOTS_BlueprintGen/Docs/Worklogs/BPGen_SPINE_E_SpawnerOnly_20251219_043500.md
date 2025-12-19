# Buddy Worklog — SPINE_E Spawner-only path

goal
- Make spawner-first the only path for normal nodes; keep minimal special-case spawns.
- Gate pin linking fallback and replace legacy test harness with a spawner-key example.

what changed
- ApplyGraphSpec node creation now uses spawner_key exclusively for normal nodes; NodeType chain deprecated and limited to knot/select/dynamic-cast only.
- Added cvar `sots.bpgen.allow_makelink_fallback` (default 0) to disable MakeLinkTo fallback when schema rejects connections; connections now fail with a warning when fallback is disabled.
- Replaced BuildTestAllNodesGraphForBPPrintHello with a spawner-key PrintString harness.

files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

notes + risks/unknowns
- Existing specs that rely on NodeType without spawner_key will now be skipped with warnings.
- MakeLinkTo fallback is disabled by default; enable via cvar if schema gaps appear.
- NodeId still stored in NodeComment; not addressed here.

verification status
- UNVERIFIED (no build/editor run)

cleanup
- Plugins/SOTS_BlueprintGen/Binaries not present (no deletion needed)
- Plugins/SOTS_BlueprintGen/Intermediate not present (no deletion needed)

follow-ups / next steps
- Add metadata storage for NodeId to avoid comment collision.
- Confirm schema-only wiring works across typical graphs; temporarily enable cvar if needed.
- Update docs/spec guidance to require spawner_key and note NodeType deprecation.
