[CONTEXT_ANCHOR]
ID: 20251218_0459 | Plugin: SOTS_BlueprintGen | Pass/Topic: NodeTypeRetire | Owner: Buddy
Scope: Removed NodeType-only spawn helpers; spawner_key required except for minimal synthetic nodes.

DONE
- Deleted legacy Spawn* helpers for standard K2 nodes (functions/arrays/vars/branch/loops/macros/switch/literal/delegate/event/tag/name helpers) tied to NodeType path.
- NodeType fallback now only supports Knot, Select, DynamicCast; others warn to use spawner_key.
- Removed unused helpers (e.g., ResolveStructPath, macro/switch scaffolding) supporting deprecated paths.

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Spawner coverage is sufficient; NodeType-only specs will be skipped with warnings until migrated.
- Minimal synthetic set (knot/select/dynamic-cast) remains adequate; no additional bespoke nodes required.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

NEXT (Ryan)
- Build/editor: confirm minimal synthetic set works and other NodeType specs surface warnings; migrate specs to spawner_key.
- Watch for missing node types not covered by spawners and decide on additions only if required.

ROLLBACK
- Revert Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp and remove this worklog/anchor if necessary.
