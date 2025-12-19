# Example 05 - AutoFix Insert Conversion

Goal: link an `int` output to a `float` output and let `auto_fix_insert_conversions` insert `Conv_IntToFloat`.

Steps
1) Ensure a function with an `int` input and `float` output:
   `bpgen_ensure_function(blueprint_asset_path="/Game/BP_My", function_name="Auto_Convert", signature={ "Inputs":[{"Name":"Count","Category":"int"}], "Outputs":[{"Name":"ReturnValue","Category":"float"}] })`
2) Apply with auto-fix enabled:
```json
{
  "target": {
    "blueprint_asset_path": "/Game/BP_My",
    "target_type": "Function",
    "name": "Auto_Convert",
    "create_if_missing": true
  },
  "nodes": [
    {"Id":"Entry","NodeType":"K2Node_FunctionEntry","NodeId":"Entry","NodePosition":{"X":0.0,"Y":0.0}},
    {"Id":"Result","NodeType":"K2Node_FunctionResult","NodeId":"Result","NodePosition":{"X":360.0,"Y":0.0}}
  ],
  "links": [
    {"FromNodeId":"Entry","FromPinName":"Count","ToNodeId":"Result","ToPinName":"ReturnValue","use_schema":true}
  ],
  "auto_fix": true,
  "auto_fix_insert_conversions": true,
  "auto_fix_max_steps": 3
}
```
3) Verify:
   - `bpgen_list(...)` should show a new conversion node with a deterministic `node_id`.
   - `bpgen_describe(..., node_id="<entry>__to__<result>__conv__IntToFloat")` confirms links.
4) Compile + save.

Expected
- `auto_fix_steps` includes `FIX_INSERT_CONVERSION`.
- Warnings note the inserted conversion node and the original schema rejection.
