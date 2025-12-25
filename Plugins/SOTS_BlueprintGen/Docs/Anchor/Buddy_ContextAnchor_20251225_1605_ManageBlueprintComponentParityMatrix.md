Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1605 | Plugin: SOTS_BlueprintGen | Pass/Topic: VibeUE parity matrix update (components) | Owner: Buddy
Scope: Parity matrix status update for tool family (5) manage_blueprint_component

DONE
- Updated `Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md` section (5) to reflect current implementation.
- Marked `reorder` as Partial (warning/no-op).

VERIFIED
- None (doc-only change).

UNVERIFIED / ASSUMPTIONS
- Assumes unified MCP `manage_blueprint_component` is wired to BPGen bridge component ops and works end-to-end.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

NEXT (Ryan)
- Confirm `manage_blueprint_component` actions behave correctly in editor.
- If `reorder` stays a no-op, keep the warning and document expected behavior.

ROLLBACK
- Revert Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md
