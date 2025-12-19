# Example 03 — Widget Text Binding (if supported)

Goal: create or update a text binding function and wire it via graph apply.

Steps
1) Discover the binding function context (widget blueprint) and target spawner(s) for your binding nodes.
2) Build `graph_spec` for the binding function (e.g., `GetLabelText`):
```json
{
  "nodes": [
    {"node_id": "entry", "node_type": "Entry", "position": [0,0]},
    {"node_id": "return", "spawner_key": "K2Node_ReturnNode", "position": [400,0], "pin_defaults": {"ReturnValue": "Label"}}
  ],
  "links": [
    {"from": {"node_id": "entry", "pin": "then"}, "to": {"node_id": "return", "pin": "execute"}, "use_schema": true}
  ]
}
```
3) Apply: `bpgen_apply(blueprint_asset_path="/Game/UI/WBP_Label", function_name="GetLabelText", graph_spec=...)`.
4) Verify with `bpgen_describe(node_id="return")` to confirm return pin and value.
5) Compile + save.

Note: If the bridge adds `ensure_widget`/`ensure_binding` actions later, run those before applying the graph.
