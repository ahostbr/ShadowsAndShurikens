[CONTEXT_ANCHOR]
ID: 20251219_1710 | Plugin: SOTS_BlueprintGen | Pass/Topic: SPINE_H2_UMG | Owner: Buddy
Scope: UMG ensure/binding surface added (widget creation, property set, binding apply + bridge actions)

DONE
- Added UMG structs for widget/property/binding requests and results (FSOTS_BPGenWidgetSpec/Property/Binding).
- New ensure APIs: EnsureWidgetComponent, SetWidgetProperties, EnsureWidgetBinding (path-based property apply, binding entry updates, optional graph apply).
- GraphResolver + builder treat target_type=WidgetBinding as function-like for graph apply.
- Bridge routes for ensure_widget / set_widget_properties / ensure_binding + protocol doc updates.

VERIFIED
- None (code-only; no editor/build/run).

UNVERIFIED / ASSUMPTIONS
- Property path setter supports object/struct chains only (no array/map indices); relies on ImportText formatting.
- Binding function creation flags are best-effort (EnsureFunction does not expose create/update outcome).
- GraphSpec application for WidgetBinding assumed equivalent to Function graphs.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenEnsure.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenEnsure.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenGraphResolver.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenGraphResolver.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md

NEXT (Ryan)
- Run bridge actions against a widget BP: ensure_widget (create + slot props), set_widget_properties, ensure_binding with graph_spec; verify binding attaches and graph executes.
- Confirm property path ImportText handles common UMG fields (Text, Color, Anchors, Padding) and slot path usage (Slot.Padding.Left, etc.).
- Validate WidgetBinding target graph apply compiles correctly in editor.

ROLLBACK
- Revert touched files in SOTS_BlueprintGen and SOTS_BPGen_Bridge or git revert the corresponding commit.
