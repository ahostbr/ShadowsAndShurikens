# Blueprint Workflow (BPGen)

Commands (FastMCP tool names)
- `bpgen_ping()`
- `bpgen_discover(blueprint_asset_path, search_text, max_results=200, include_pins=False)`
- `bpgen_apply(blueprint_asset_path, function_name, graph_spec)`
- `bpgen_list(blueprint_asset_path, function_name, include_pins=False)`
- `bpgen_describe(blueprint_asset_path, function_name, node_id, include_pins=True)`
- `bpgen_compile(blueprint_asset_path)`
- `bpgen_save(blueprint_asset_path)`
- `bpgen_refresh(blueprint_asset_path, function_name, include_pins=False)`

Loop
1) Ping → discover desired node spawner.
2) Build `graph_spec` nodes with `spawner_key`, `node_id`, positions, and defaults.
3) Add links (exec + data) using schema-first expectations; keep pin names from discover/describe.
4) Apply with `bpgen_apply`.
5) Verify with `bpgen_list` and targeted `bpgen_describe` calls.
6) Compile and save when stable.

Tips
- Keep `node_id` stable between runs; reuse to update instead of duplicating.
- If a pin fails, re-run `bpgen_describe` to see exact pin names/types.
- If links look stale after type changes, call `bpgen_refresh` then re-apply.
