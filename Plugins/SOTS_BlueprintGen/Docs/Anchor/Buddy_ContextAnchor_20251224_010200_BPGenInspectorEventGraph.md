Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251224_010200 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenInspectorEventGraph | Owner: Buddy
Scope: Give the inspector a direct route to event graphs (Ubergraph pages) so the MCP bridge can enumerate `EventGraph` nodes without shim functions.

DONE
- Added `#include "SOTS_BPGenGraphResolver.h"` and let `Inspector_FindFunctionGraph` reuse `USOTS_BPGenGraphResolver::ResolveEventGraph` whenever the requested name isn't already a function graph.
- `ListFunctionGraphNodes` and `DescribeNodeById` now implicitly cover Ubergraph pages because they load graphs via the updated inspector helper.

VERIFIED
- None (the new path has not been exercised in a compiled build).

UNVERIFIED / ASSUMPTIONS
- The graph names provided by clients (e.g. `EventGraph`) exactly match the Ubergraph page names that `ResolveEventGraph` searches for.

FILES TOUCHED
- Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp
- Docs/Worklogs/BuddyWorklog_20251224_010200_BPGenInspectorEventGraph.md

NEXT (Ryan)
- Rebuild the plugin/editor, run the BPGen bridge `list_nodes`/`describe_node` requests against `EventGraph`, and verify nodes and pin data return without the previous error.
- If other inspector flows need Ubergraph coverage, consider exposing a boolean or direct helper so callers can iterate every Ubergraph page by name.

ROLLBACK
- Revert Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp (and this anchor/worklog) to restore the prior function-only inspection behavior.
