goal
- Add the EventGraph-specific fallback to the BPGen inspector so MCP `list_nodes` can target Ubergraph pages, and capture the worklog/anchor for this change.

what changed
- Updated `SOTS_BPGenInspector.cpp` to include `SOTS_BPGenGraphResolver` and call `ResolveEventGraph` when a graph is missing from `FunctionGraphs`.
- Logged the effort and intent in this follow-up worklog plus the matching anchor.

files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BuddyWorklog_20251224_120000_EventGraphInspectorFollowup.md

notes + risks/unknowns
- Not yet built or run; the new resolver path is unverified and depends on the graph resolver returning Ubergraph targets as expected.

verification status
- Unverified (no build/test performed).

follow-ups / next steps
- Build SOTS_BlueprintGen and rerun the MCP `list_nodes` request targeting `EventGraph` to ensure nodes appear.
- Compare the resolver output to the discovered node descriptors to confirm metadata alignment.
