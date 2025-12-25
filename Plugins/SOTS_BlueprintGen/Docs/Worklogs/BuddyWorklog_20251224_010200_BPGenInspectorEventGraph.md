goal
- Extend BPGen inspection so `list_nodes` can reach Ubergraph pages (e.g. `EventGraph`) without forcing callers to target a normal function name.

what changed
- Added the `SOTS_BPGenGraphResolver` include and reused its `ResolveEventGraph` helper whenever the requested graph name isn't in `Blueprint->FunctionGraphs` so event graphs are returned.
- `ListFunctionGraphNodes` now relies on the enhanced helper, giving MCP clients a working path for the EventGraph even though it lives in `Blueprint->UbergraphPages`.

files changed
- Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp

notes + risks/unknowns
- No build or editor run accompanied this change, so the new resolver branch hasn't been verified in a live editor session (the GraphResolver call assumes no graph creation).

verification status
- UNVERIFIED (no editor/build run).

follow-ups / next steps
- Rebuild SOTS_BlueprintGen, restart the editor/bridge, and rerun `list_nodes` with `function_name=EventGraph` to prove the EventGraph now returns nodes.
- If new itineraries are needed, consider surfacing `UbergraphPages` metadata through an explicit inspector API so callers can iterate every event graph.
