goal
- Use the BPGen bridge to read the player blueprint via the MCP server so we know what data is available for inspection.

what changed
- None; this was an inspection-only task (no code edits).

files changed
- None.

notes + risks/unknowns
- `list_nodes` for `/Game/SOTS/Player/BP_SOTS_Player` with `function_name=EventGraph` fails because `USOTS_BPGenInspector` only looks at `Blueprint->FunctionGraphs`, so the Ubergraph (EventGraph) is not exposed yet.
- `discover_nodes` returns node descriptors from the global node spawner library rather than the actual blueprint contents, so it does not help for reading the player graph directly.

verification status
- Ran a TCP request through the running MCP BPGen bridge; `list_nodes` returned an explicit "Function graph 'EventGraph' not found..." error and `discover_nodes` succeeded in enumerating node spawner descriptors.

follow-ups / next steps
- Enhance the inspector (or add a new helper) to resolve Ubergraph pages via the graph resolver so BPGen can enumerate event graph nodes.
- Once event graph support exists, re-run the `list_nodes` request to serialize the player blueprint itself.
