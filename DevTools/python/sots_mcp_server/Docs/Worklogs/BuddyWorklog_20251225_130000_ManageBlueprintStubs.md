Send2SOTS
# Buddy Worklog — 2025-12-25 13:00 — MCP manage_blueprint stubs

## Goal
Finish the stubbed `manage_blueprint` actions in the unified MCP layer by mapping them to new BPGen bridge `bp_blueprint_*` actions.

## What Changed
- Extended `manage_blueprint` tool signature with optional parameters:
  - `property_name`, `property_value`, `new_parent_class`, `include_class_defaults`, `max_nodes`
- Implemented actions:
  - `get_info` → `bp_blueprint_get_info`
  - `get_property` → `bp_blueprint_get_property`
  - `set_property` → `bp_blueprint_set_property` (mutation gated; sends `dangerous_ok=true`)
  - `reparent` → `bp_blueprint_reparent` (mutation gated; sends `dangerous_ok=true`)
  - `list_custom_events` → `bp_blueprint_list_custom_events`
  - `summarize_event_graph` → `bp_blueprint_summarize_event_graph`

## Files Changed
- DevTools/python/sots_mcp_server/server.py

## Notes / Risks / Unknowns
- `property_value` is converted to UE ImportText via existing `_ue_import_text`.
- Mutation gating uses `_bpgen_guard_mutation()`; actions also rely on bridge safe-mode/dangerous checks.

## Verification Status
UNVERIFIED
- No runtime validation via Unreal/bridge.

## Follow-ups / Next Steps (Ryan)
- Run MCP calls for `manage_blueprint` actions and confirm bridge responses.
- Confirm `SOTS_ALLOW_BPGEN_APPLY` gating behavior for `set_property`/`reparent`.
