Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1820 | Plugin: sots_mcp_server | Pass/Topic: GET_HELP_TOPIC_ROUTING | Owner: Buddy
Scope: Implemented topic routing for VibeUE-compatible get_help()

DONE
- `get_help(topic=...)` now loads markdown from `Plugins/VibeUE/Content/Python/resources/topics/<topic>.md`.
- Added fallbacks to `topics/topics.md` (index) and `help.md` (full reference).
- Kept prior behavior by also returning the unified `sots_help` index.

VERIFIED
- (none) No runtime/editor verification performed.

UNVERIFIED / ASSUMPTIONS
- Assumes the VibeUE resources directory exists in the project checkout.
- Assumes MCP clients tolerate the unified `ok/data/warnings/meta` wrapper around help content.

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

NEXT (Ryan)
- Validate `get_help()` and `get_help(topic="node-tools")` via MCP and confirm correct markdown is returned.

ROLLBACK
- Revert DevTools/python/sots_mcp_server/server.py and Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md
