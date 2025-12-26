Send2SOTS
# Buddy Worklog — manage_blueprint_variable 7 actions (bridge + MCP)

Date: 2025-12-25
Owner: Buddy

## Goal
Finish VibeUE parity for `manage_blueprint_variable` (7 actions): `search_types`, `create`, `list`, `get_info`, `get_property`, `set_property`, `delete`.

## What Changed
- Added a new bridge ops module implementing variable actions (`bp_variable_*`).
- Wired new actions into the bridge dispatcher and marked mutating ones as dangerous for apply-gating.
- Updated unified Python MCP tool `manage_blueprint_variable` to accept VibeUE-style parameters (`variable_config.type_path`, `list_criteria`, `search_criteria`, etc.) while preserving the legacy `pin_type` create pathway.
- Updated the upstream parity matrix to mark the tool family as Done.

## Files Changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeVariableOps.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeVariableOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

## Notes / Risks / Unknowns
- `search_types` currently scans loaded `UClass` objects and always includes primitive canonical type paths; it does not attempt to fully replicate VibeUE’s type categorization/filter semantics.
- `create` maps `type_path` to BPGen pin spec with a minimal mapping:
  - canonical primitives map to `float/int/bool/...`
  - `/Script/*` and `/Game/*` map to `Category=object` + `SubObjectPath=type_path`
  This is sufficient for typical class/object variables but may not cover all struct/enum/container cases.
- `get_property`/`set_property` support the properties used by SOTS variable tests (`category`, `tooltip`, `default_value`, `is_editable`, `is_blueprint_readonly`) plus a metadata fallback.
- No build/editor verification performed here (per SOTS laws; Ryan validates).

## Verification Status
UNVERIFIED
- No Unreal build or in-editor run was executed.

## Next Steps (Ryan)
- Run the variable test prompts:
  - Plugins/SOTS_BlueprintGen/Docs/LLM_ALWAYS_READ/blueprint/02_manage_blueprint_variable.md
  - Plugins/VibeUE/test_prompts/blueprint/manage_blueprint_variable.md (parity reference)
- Confirm `search_types → create` works for `/Script/UMG.UserWidget` and at least one asset type.
- Confirm `set_property` updates category/tooltip/defaults as expected and persists after compile.

## Rollback
- Revert the files listed above.
