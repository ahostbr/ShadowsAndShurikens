Send2SOTS
# VibeUE Upstream Parity Matrix (WIP)

Target: upstream VibeUE tool families as documented in `WORKING/VibeUE-master/README.md`.

Important: the upstream README currently documents **13 multi-action tools**, not 44 separate tools. If “44” refers to an older upstream revision or a different counting scheme (e.g., individual helper modules), update this matrix source-of-truth first.

## Parity Levels
- **Done**: Implemented in SOTS unified MCP (`DevTools/python/sots_mcp_server/server.py`) and backed by BPGen/bridge/services.
- **Partial**: Tool exists but only some actions are implemented.
- **Stubbed**: Tool exists but returns structured `not implemented`.
- **Missing**: Not exposed on unified MCP yet.

## Mapping Notes
- SOTS canonical MCP entrypoint is `DevTools/python/sots_mcp_server/server.py`.
- BPGen canonical contract: `Plugins/SOTS_BlueprintGen/Docs/Contracts/SOTS_BPGen_IntegrationContract.md`.
- BPGen action surface reference: `Plugins/SOTS_BlueprintGen/Docs/API/BPGen_ActionReference.md`.

## Tool Family Status (Upstream → SOTS)

### 1) manage_blueprint (8 actions)
Status: **Partial** (SOTS VibeUE-compat tool exists)
- create → **Done** (mapped to `bpgen_create_blueprint`)
- compile → **Done** (mapped to `bpgen_compile`)
- get_info → **Done** (mapped to BPGen bridge `bp_blueprint_get_info`)
- get_property → **Done** (mapped to BPGen bridge `bp_blueprint_get_property`)
- set_property → **Done** (mapped to BPGen bridge `bp_blueprint_set_property`; dangerous + safe-mode gated)
- reparent → **Done** (mapped to BPGen bridge `bp_blueprint_reparent`; dangerous + safe-mode gated)
- list_custom_events → **Done** (mapped to BPGen bridge `bp_blueprint_list_custom_events`)
- summarize_event_graph → **Done** (mapped to BPGen bridge `bp_blueprint_summarize_event_graph`)

### 2) manage_blueprint_function (13 actions)
Status: **Partial**
- create → **Done** (mapped to `bpgen_ensure_function`)
- list/get/list_params/delete/add_param/remove_param/update_param/locals/update_properties → Stubbed (needs signature/locals contract)

### 3) manage_blueprint_variable (7 actions)
Status: **Partial**
- create → **Done** (mapped to `bpgen_ensure_variable`; requires BPGen `pin_type` dict)
- list/delete/get_info/get_property/set_property/search_types → Stubbed

### 4) manage_blueprint_node (17 actions)
Status: **Partial**
- discover → **Done** (mapped to `bpgen_discover`)
- list → **Done** (mapped to `bpgen_list`)
- describe → **Done** (mapped to `bpgen_describe`)
- delete → **Done** (mapped to `bpgen_delete_node`)
- create → **Done (function + event graphs)**
   - function graphs: GraphSpec apply via `bpgen_apply`; returns BPGen `NodeId` as `node_id`
   - event graphs: GraphSpec apply via `bpgen_apply_to_target` with `target_type=EventGraph`
- connect/connect_pins → **Done (function + event graphs)**
   - function graphs: `bpgen_apply`
   - event graphs: `bpgen_apply_to_target`
- configure → **Done (function + event graphs)**
   - function graphs: `bpgen_apply` (`ExtraData` literal defaults)
   - event graphs: `bpgen_apply_to_target` (`ExtraData` literal defaults)
- move → **Done (function + event graphs)**
   - function graphs: `bpgen_apply` (`NodePosition`)
   - event graphs: `bpgen_apply_to_target` (`NodePosition`)
- disconnect → **Done (function graphs only)** (per-connection `bpgen_delete_link`)
- split/recombine/refresh_node(s)/reset_pin_defaults/get_details → Stubbed
    - Note: event-graph delete/disconnect requires target-based graph edit actions (not available today).

### 5) manage_blueprint_component (12 actions)
Status: **Partial**
- search_types → **Done** (mapped to BPGen bridge `bp_component_search_types`)
- get_info → **Done** (mapped to BPGen bridge `bp_component_get_info`)
- list → **Done** (mapped to BPGen bridge `bp_component_list`)
- get_property → **Done** (mapped to BPGen bridge `bp_component_get_property`)
- set_property → **Done** (mapped to BPGen bridge `bp_component_set_property`; dangerous + safe-mode gated)
- get_all_properties → **Done** (mapped to BPGen bridge `bp_component_get_all_properties`)
- compare_properties → **Done** (implemented in unified MCP as two `get_all_properties` calls + diff)
- create → **Done** (mapped to BPGen bridge `bp_component_create`; dangerous + safe-mode gated)
- delete → **Done** (mapped to BPGen bridge `bp_component_delete`; dangerous + safe-mode gated)
- reparent → **Done** (mapped to BPGen bridge `bp_component_reparent`; dangerous + safe-mode gated)
- reorder → **Partial** (mapped to BPGen bridge `bp_component_reorder`; currently returns a warning/no-op)
- get_property_metadata → **Done** (mapped to BPGen bridge `bp_component_get_property_metadata`)

### 6) manage_umg_widget (11 actions)
Status: **Partial**
- add_component → **Done** (mapped to BPGen `ensure_widget`)
- set_property → **Done** (mapped to BPGen `set_widget_properties`; values must be UE ImportText strings or simple auto-converted shapes)
- list_components/get_property/get_component_properties/remove_component/validate/search_types/get_available_events/bind_events/list_properties → Stubbed
   - Note: BPGen supports property bindings via `ensure_binding`, but VibeUE `bind_events` targets input delegates (OnClicked/etc.) and is not mapped yet.

### 7) manage_asset (10 actions)
Status: **Partial**
- search → **Done** (mapped to BPGen bridge `asset_search`)
- open_in_editor → **Done** (mapped to BPGen bridge `asset_open_in_editor`)
- duplicate → **Done** (mapped to BPGen bridge `asset_duplicate`)
- delete → **Done** (mapped to BPGen bridge `asset_delete`; dangerous + safe-mode gated)
- import_texture → **Done** (mapped to BPGen bridge `asset_import_texture`)
- export_texture → **Done** (mapped to BPGen bridge `asset_export_texture`)
- svg_to_png → **Partial** (Python-local; requires optional `cairosvg` dependency)

### 8) manage_enhanced_input (24 actions)
Status: **Stubbed**
- Requires new Enhanced Input services/bridge actions.

### 9) manage_level_actors (18 actions)
Status: **Stubbed**
- Likely out-of-scope for BPGen unless explicitly approved; requires editor/world manipulation services.

### 10) manage_material (24 actions)
Status: **Stubbed**
- Requires material editor services/bridge actions.

### 11) manage_material_node (21 actions)
Status: **Stubbed**
- Requires material graph services/bridge actions.

### 12) check_unreal_connection (1 action)
Status: **Done**
- Implemented as BPGen bridge `ping` + unified server info synthesis.

### 13) get_help (1 action)
Status: **Partial**
- Implemented as a shim returning `sots_help` payload; topic routing not yet implemented.

## Next Implementation Work (ordered)
1) Finish node-level parity gaps:
   - `refresh_node(s)`, `reset_pin_defaults`, `split`, `recombine`, and event graph parity via `apply_graph_spec_to_target`.
2) Define a signature mapping contract for `manage_blueprint_function` parameter/local operations.
3) Decide governance for non-BPGen domains (assets/materials/level actors/enhanced input) and either:
   - add new BPGen bridge actions + services, or
   - declare out-of-scope for BPGen parity.
