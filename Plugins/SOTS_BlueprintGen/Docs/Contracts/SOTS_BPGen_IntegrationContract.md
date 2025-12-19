# SOTS BPGen Integration Contract (SPINE_N)

## Canonical entry surfaces
- **Unreal (One True Entry)**: `USOTS_BPGenAPI::ExecuteAction|ExecuteBatch` (JSON in/out). Actions: `apply_graph_spec`, `canonicalize_graph_spec`, `apply_function_skeleton`, `delete_node`, `delete_link`, `replace_node`, `create_struct_asset`, `create_enum_asset`.
- **Bridge**: SOTS_BPGen_Bridge TCP NDJSON (`tool":"bpgen"`, `action`, `params`, `request_id`) — mirrors the actions above.
- **MCP/DevTools**: `sots_*` + `bpgen_*` tools in DevTools MCP server call the bridge; MCP apply is gated by `SOTS_ALLOW_APPLY=1`.

## Schema + idempotency
- GraphSpec: `spec_schema="SOTS_BPGen_GraphSpec"`, `spec_version=1`; requires stable `Nodes[].Id` and honors `NodeId` for create-or-update flows.
- Target: `target.blueprint_asset_path` + `target.name` (function/graph). `repair_mode` and `canonicalize` options are honored.
- Results: all apply/edit/canonicalize structs expose `bSuccess`, `ErrorCode`, `ErrorMessage`, `Errors`, `Warnings`, and deterministic arrays (sorted in SPINE_M).
- Idempotency: destructive ops are opt-in via request flags; links/nodes include `bCreateOrUpdate`, `bAllowCreate`, `bAllowUpdate`, `bBreakExistingFrom/To`.

## Verification sequence (required)
1) `canonicalize_graph_spec` (assign ids, migrate).
2) `apply_graph_spec`.
3) `describe/list` via bridge/MCP.
4) `compile_blueprint` then `save_blueprint` (bridge action) when allowed.

## Error codes (preferred)
- Use `ErrorCode` in results when available (e.g., validation, spawn failure, link failure); keep string, no enums. Unknown errors fall back to `ErrorMessage` + `Errors` array.

## Capability gating
- Apply/mutation requires `SOTS_ALLOW_APPLY=1` (MCP + JSON API).
- Editor-only: SOTS_BlueprintGen and SOTS_BPGen_Bridge remain Editor modules; no runtime dependency on editor headers.

## Determinism
- Outputs sorted (SPINE_M), stable node ids required, repair/autofix captured in result arrays.

## Prompt + tooling alignment
- Prompt packs must use the action names above.
- DevTools inbox jobs should carry `{ "action": ..., "params": { ... }, "request_id": "optional" }` or a batch array of those objects.

## Versioning
- Contract version: `SPINE_N_2025-12-19`. Breaking changes must bump spec_version and document in ReleaseNotes.
