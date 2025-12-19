# Example 02 — Variable + Set + Branch

Goal: ensure a variable exists, set it, and branch on a condition.

Steps
1) Discover a Set node for your variable if needed: `bpgen_discover(..., search_text="Set MyVar", include_pins=true)`.
2) Graph spec sketch:
```json
{
  "nodes": [
    {"node_id": "entry", "node_type": "Entry", "position": [0,0]},
    {"node_id": "set_var", "spawner_key": "<SetNodeSpawnerKey>", "position": [250, -50], "pin_defaults": {"MyVar": "5"}},
    {"node_id": "branch", "spawner_key": "K2Node_Branch", "position": [500,0]}
  ],
  "links": [
    {"from": {"node_id": "entry", "pin": "then"}, "to": {"node_id": "set_var", "pin": "execute"}, "use_schema": true},
    {"from": {"node_id": "set_var", "pin": "then"}, "to": {"node_id": "branch", "pin": "execute"}, "use_schema": true}
  ]
}
```
3) Apply with `bpgen_apply` and keep `node_id` stable for re-runs.
4) Verify with `bpgen_describe` on `set_var` and `branch` to confirm pins/links.
5) Compile + save.

Note: if ensure-variable exists later in the bridge, call that before apply; otherwise create the variable manually first.
