[CONTEXT_ANCHOR]
ID: 20251219_0435 | Plugin: SOTS_BlueprintGen | Pass/Topic: SPINE_E_SpawnerOnly | Owner: Buddy
Scope: Enforce spawner-first node creation, gate MakeLinkTo fallback, update harness.

DONE
- ApplyGraphSpec node spawn now uses spawner_key only for normal nodes; NodeType chain removed except special cases (knot/select/dynamic cast).
- Added cvar sots.bpgen.allow_makelink_fallback (default 0) to guard schema MakeLinkTo fallback; rejected connections now warn/skip when disabled.
- Replaced BuildTestAllNodesGraphForBPPrintHello with spawner-key PrintString harness.

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Special-case spawns (knot/select/dynamic cast) cover remaining non-spawner nodes; other NodeType-only specs will be skipped.
- Schema-only wiring should pass in common cases without needing MakeLinkTo fallback.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

NEXT (Ryan)
- Build/editor: confirm graphs spawn via spawner_key-only and connections succeed with fallback disabled; enable cvar for gap triage if needed.
- Check any existing specs for NodeType-only usage and migrate to spawner_key.
- Verify PrintString harness still executes as expected.

ROLLBACK
- Revert Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp to prior commit state; remove worklog/anchor artifacts if desired.
