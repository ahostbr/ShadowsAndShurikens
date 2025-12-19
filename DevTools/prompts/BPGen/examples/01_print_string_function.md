# Example 01 — Print String Function

Goal: add a PrintString call in a function graph and verify.

Steps (call via MCP tools)
1) Discover: `bpgen_discover(blueprint_asset_path="/Game/BP_My", search_text="Print String", max_results=50)`.
2) Build `graph_spec` (example):
```json
{
  "nodes": [
    {"node_id": "entry", "node_type": "Entry", "position": [0,0]},
    {"node_id": "print", "spawner_key": "/Script/Engine.KismetSystemLibrary:PrintString", "position": [300,0], "pin_defaults": {"InString": "Hello from BPGen"}}
  ],
  "links": [
    {"from": {"node_id": "entry", "pin": "then"}, "to": {"node_id": "print", "pin": "execute"}, "use_schema": true}
  ]
}
```
3) Apply: `bpgen_apply(blueprint_asset_path="/Game/BP_My", function_name="MyFunc", graph_spec=...)`.
4) Verify: `bpgen_list(...)` and `bpgen_describe(..., node_id="print")`.
5) Compile + save: `bpgen_compile(...)`, then `bpgen_save(...)`.
