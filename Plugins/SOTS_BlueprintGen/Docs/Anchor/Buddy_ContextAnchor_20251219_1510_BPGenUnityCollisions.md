[CONTEXT_ANCHOR]
ID: 20251219_1510 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenUnityCollisions | Owner: Buddy
Scope: Resolve unity-build helper collisions in BPGen (debug/inspector/graph resolver helpers).

DONE
- Prefixed BPGenDebug helper functions (load graph, node id, match, annotation) to avoid clashes with builder helpers.
- Prefixed BPGenInspector helper functions (node ids, summaries, sorters, loader/saver) and updated all call sites.
- Prefixed BPGenGraphResolver error constants to avoid duplication with Ensure constants.
- Deleted plugin Binaries and Intermediate for a clean rebuild.

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Only symbol scope changes; runtime behavior should be unchanged.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDebug.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenGraphResolver.cpp
- Plugins/SOTS_BlueprintGen/Binaries
- Plugins/SOTS_BlueprintGen/Intermediate

NEXT (Ryan)
- Rebuild SOTS_BlueprintGen to confirm ambiguous overload/redefinition errors are resolved.
- If build passes, optionally sanity-check BPGenDebug and Inspector flows in-editor.

ROLLBACK
- Revert the three cpp files and restore plugin binaries/intermediate from source control if needed.
