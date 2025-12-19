[CONTEXT_ANCHOR]
ID: 20251218_2355 | Plugin: SOTS_BlueprintGen | Pass/Topic: SPINE_E_NodeIdReuse | Owner: Buddy
Scope: NodeId-aware create/update handling for ApplyGraphSpecToFunction.

DONE
- Added NodeId helpers using NodeComment for storage and lookup
- ApplyGraphSpecToFunction reuses existing nodes when updates allowed, skips when updates disallowed, and warns when create_or_update=false with existing NodeId
- Apply result now records created/updated/skipped NodeIds and link stage skips endpoints that were intentionally skipped

VERIFIED
- None (no build/editor run)

UNVERIFIED / ASSUMPTIONS
- Reusing NodeComment for NodeId will not clash with hand-authored comments
- Persisting nodes not referenced by specs is acceptable for SPINE_E idempotent flow

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BPGen_SPINE_E_NodeIdReuse_20251218_235200.md

NEXT (Ryan)
- Apply a sample GraphSpec with NodeIds to confirm reuse vs create behavior and array reporting
- Check output arrays CreatedNodeIds/UpdatedNodeIds/SkippedNodeIds in downstream tool surfaces
- Verify no unintended node deletions or comment duplication in existing graphs

ROLLBACK
- Revert Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp and Plugins/SOTS_BlueprintGen/Docs/Worklogs/BPGen_SPINE_E_NodeIdReuse_20251218_235200.md
