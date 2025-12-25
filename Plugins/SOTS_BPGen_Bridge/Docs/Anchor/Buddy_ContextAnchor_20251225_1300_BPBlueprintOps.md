Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1300 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BP_BLUEPRINT_OPS | Owner: Buddy
Scope: Added bridge actions for blueprint-level manage_blueprint parity

DONE
- Added `SOTS_BPGenBridgeBlueprintOps` with actions: `bp_blueprint_get_info`, `bp_blueprint_get_property`, `bp_blueprint_set_property`, `bp_blueprint_reparent`, `bp_blueprint_list_custom_events`, `bp_blueprint_summarize_event_graph`.
- Routed the new actions in `SOTS_BPGenBridgeServer`.
- Marked `bp_blueprint_set_property` and `bp_blueprint_reparent` as dangerous (safe-mode gated).
- Added `BlueprintGraph` to module dependencies.

VERIFIED
- (none) No build/editor verification performed.

UNVERIFIED / ASSUMPTIONS
- Assumes `UK2Node_CustomEvent` usage requires `BlueprintGraph` dependency (added).
- Assumes first `UbergraphPages[0]` is acceptable as the Event Graph for summarization.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeBlueprintOps.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeBlueprintOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.Build.cs

NEXT (Ryan)
- Build the plugin and ensure it loads in-editor.
- Validate each new action against a real Blueprint asset.
- Confirm safe-mode blocks the mutating actions unless `dangerous_ok=true`.

ROLLBACK
- Revert the above files; restore plugin `Binaries/` and `Intermediate/` via rebuild.
