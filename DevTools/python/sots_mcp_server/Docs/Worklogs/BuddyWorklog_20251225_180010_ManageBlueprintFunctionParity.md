Send2SOTS
# Buddy Worklog — 2025-12-25 18:00:10 — Unified MCP: manage_blueprint_function parity

## Goal
Finish parity item (2) by expanding the unified MCP `manage_blueprint_function` tool beyond `create`, routing actions to the new `bp_function_*` bridge operations with correct mutation gating.

## What Changed
- Expanded `manage_blueprint_function` to support:
  - `list`, `get`, `delete`
  - `list_params`, `add_param`, `remove_param`, `update_param`
  - `list_locals`, `add_local`, `remove_local`, `update_local`
  - `update_properties`
- Added `bp_function_*` actions to `BPGEN_READ_ONLY_ACTIONS` and `BPGEN_MUTATION_ACTIONS` for consistent gating behavior.
- Mutating actions call `_bpgen_guard_mutation(...)` and pass `dangerous_ok=True`.

## Evidence Used
- RAG query report: `Reports/RAG/rag_query_20251225_174113.*`

## Files Changed
- `DevTools/python/sots_mcp_server/server.py`

## Notes / Risks / Unknowns
- Tool signature changed to accept VibeUE-like arguments (`param_name`, `direction`, `type`, etc.). Existing `create(signature=...)` behavior remains.
- No live Unreal/editor run performed here.

## Verification
UNVERIFIED
- No MCP runtime test against a running BPGen bridge was performed in this pass.

## Next (Ryan)
- Start the BPGen bridge server in-editor and call the new actions from MCP.
- Confirm `SOTS_ALLOW_BPGEN_APPLY=0` blocks the mutating actions, while read-only function actions still work.
