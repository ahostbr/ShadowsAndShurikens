Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_0200 | Plugin: SOTS_BlueprintGen | Pass/Topic: EventGraphDeleteNodeSupport | Owner: Buddy
Scope: Allow BPGen delete/link/replace edit ops to resolve EventGraph via function_name="EventGraph".

DONE
- Updated graph resolution for edit ops to return `UBlueprint::UbergraphPages[0]` when `function_name == "EventGraph"`.
- Preserved existing function-graph lookup behavior for normal functions.

VERIFIED
- None (no build/editor validation performed).

UNVERIFIED / ASSUMPTIONS
- Assumed EventGraph is always `UbergraphPages[0]` for affected Blueprints.
- Assumed this enables bridge actions `delete_node_by_id` / `delete_link` / `replace_node` to operate on EventGraph.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

NEXT (Ryan)
- Rebuild/load updated plugin, then re-run EventGraph cleanup deletes on `/Game/BPGen/BP_BPGen_Testing.BP_BPGen_Testing` using `dangerous_ok:true`.
- Compile/save the Blueprint and validate in PIE that the BeginPlay→Delay(3s)→ShowNotification flow still works.

ROLLBACK
- Revert Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
