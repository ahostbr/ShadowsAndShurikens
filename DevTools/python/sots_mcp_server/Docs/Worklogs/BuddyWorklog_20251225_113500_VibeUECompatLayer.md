Send2SOTS
# Buddy Worklog — 2025-12-25 11:35 — Implement: VibeUE Upstream Compat Layer (Phase 1 scaffold)

## Goal
Begin implementing the VibeUE-upstream parity plan by adding a VibeUE-compatible tool layer to the unified SOTS MCP server (`DevTools/python/sots_mcp_server/server.py`), backed by BPGen where possible.

## What Changed
- Added VibeUE-upstream tool names to the unified MCP server (13 tools) as compatibility shims.
- Implemented a minimal subset backed by existing BPGen bridge actions:
  - `manage_blueprint`: `create`, `compile`
  - `manage_blueprint_function`: `create` (mapped to `bpgen_ensure_function`)
  - `manage_blueprint_variable`: `create` (mapped to `bpgen_ensure_variable`; requires BPGen `pin_type` dict)
  - `manage_blueprint_node`: `discover`, `list`, `describe`, `delete`
  - `check_unreal_connection`: implemented via BPGen ping + server_info
  - `get_help`: shim to `sots_help`
- All other actions/tools currently return a structured `not implemented` response (keeps prompts readable while tracking gaps).
- Added these new tool names to `sots_help` canonical tools list for discoverability.

## Files Changed
- `DevTools/python/sots_mcp_server/server.py`

## New/Updated Docs
- Added parity matrix doc:
  - `Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md`

## Notes / Risks / Unknowns
- Upstream README in `WORKING/VibeUE-master/README.md` documents 13 multi-action tools, not 44 separate tools. If “44” is required, we need the exact upstream revision/source list to avoid building toward the wrong target.
- Many VibeUE node operations can likely be expressed via BPGen GraphSpec apply + delete_link/replace_node/refresh wrappers, but that mapping is not implemented yet.
- UMG/asset/material/enhanced input/level actor parity requires new Unreal-side editor services + BPGen bridge actions (not started).

## Verification
UNVERIFIED
- No Unreal/editor validation performed in this pass. Syntax checked via VS Code diagnostics only.

## Next (Ryan)
- Run `sots_help` and confirm the new VibeUE tool names appear.
- Decide whether the parity target is “13 tools/167 actions” (as per current upstream README) or another upstream variant.
- If approved, next implementation step is `manage_blueprint_node.create/connect_pins/configure/move` using BPGen GraphSpec recipes.
