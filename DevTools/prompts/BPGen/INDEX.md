# BPGen Prompt Index

## Topics
- GraphSpec authoring: building nodes/links with stable `NodeId`, `Target` paths, `spec_schema/spec_version`.
- Canonicalize flow: run `canonicalize_graph_spec` before `apply_graph_spec` for ids/migrations.
- Function skeletons: `apply_function_skeleton` to create functions and parameters before node wiring.
- Edits: `delete_node`, `delete_link`, `replace_node`, `create_struct_asset`, `create_enum_asset`.
- Apply gating: set `SOTS_ALLOW_APPLY=1` for any mutation; otherwise expect errors.
- Verification: after apply, `compile_blueprint` + `save_blueprint` via bridge.

## Examples
- `examples/apply_graph_spec.md`: End-to-end apply after canonicalize.
- `examples/apply_function_skeleton.md`: Create function + params.
- `examples/delete_and_replace.md`: Delete node/link, replace node, rewire.
- `examples/create_struct_enum.md`: Create struct/enum asset shells.
