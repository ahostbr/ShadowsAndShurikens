# Buddy Worklog — BPGen EventGraph delete-node support

Date: 2025-12-25
Owner: Buddy
Plugin: SOTS_BlueprintGen

## Goal
Enable BPGen graph edit operations (delete node/link/replace) to work against Blueprint EventGraph when the bridge is passed `function_name: "EventGraph"`, so we can clean up stray nodes created during BPGen apply attempts.

## What changed
- Extended graph resolution for edit operations to treat `function_name == "EventGraph"` as the Blueprint ubergraph (first entry in `UBlueprint::UbergraphPages`).
- Kept existing behavior for real function graphs unchanged.

## Files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

## Notes / Risks / Unknowns
- UNVERIFIED: Requires recompiling/reloading the plugin/module; no in-editor test performed here.
- Assumption: Using `UbergraphPages[0]` is the correct EventGraph for this project’s Blueprints.
- This change will affect any bridge actions that rely on `LoadBlueprintAndGraphForEdit` (e.g. delete node/link/replace) and now allows operating on EventGraph via the same parameter.

## Verification status
- NOT VERIFIED in editor/runtime.
- Static check only: file edited and workspace error scan reports no errors.

## Follow-ups / Next steps (Ryan)
- Rebuild plugins (or recompile module) so bridge uses updated `SOTS_BlueprintGen`.
- Re-run bridge action `delete_node_by_id` with `function_name:"EventGraph"` + `dangerous_ok:true` on `/Game/BPGen/BP_BPGen_Testing.BP_BPGen_Testing` to remove the older incorrect chain nodes (`BeginPlay`, `Delay`, `GetUIRouter`, `ShowNotification`).
- Compile + save blueprint and confirm in PIE the notification fires after 3 seconds.
