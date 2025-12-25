# Buddy Worklog — 2025-12-25 — ManageBlueprintComponentRouting

## Goal
Implement VibeUE-compat `manage_blueprint_component` routing in the unified MCP server and update parity docs accordingly.

## What changed
- Implemented `manage_blueprint_component` in the unified MCP server (was previously a structured stub).
- Added action mapping to BPGen bridge component ops (`bp_component_*`).
- Implemented `compare_properties` in Python using two bridge `get_all_properties` calls + a deterministic diff.
- Updated the VibeUE parity matrix entry for tool family (5) to reflect current implementation status.

## Files changed
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

## Notes / risks / unknowns
- `reorder` is mapped but the bridge-side implementation currently returns a warning/no-op; parity marked Partial for that action.
- Mutating actions (`create/delete/reparent/set_property/reorder`) are guarded by unified MCP apply gates and forward `dangerous_ok=true`.
- `set_property` and initial `properties` in `create` use UE ImportText strings via `_ue_import_text`; correctness depends on bridge-side ImportText parsing.
- `compare_properties` normalizes dict/list values via JSON (sorted keys) when possible; non-JSON-serializable values fall back to `str()`.

## Verification status
- VERIFIED: `server.py` has no VS Code-reported errors after edit.
- UNVERIFIED: No Unreal Editor/bridge runtime validation performed (no component ops calls executed).

## Follow-ups / next steps (Ryan)
- Smoke-test `manage_blueprint_component` actions against a known Blueprint:
  - `list`, `get_property`, `set_property`, `create`, `delete`, `reparent`.
- Confirm mutation gating behavior with `SOTS_ALLOW_BPGEN_APPLY=0/1` and safe-mode handling.
- Confirm `reorder` warning/no-op matches expectations (and decide if/when to implement real SCS reorder).
