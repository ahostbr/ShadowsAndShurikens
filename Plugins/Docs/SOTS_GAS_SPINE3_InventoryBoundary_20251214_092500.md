# Buddy Worklog — SOTS_GAS SPINE3 Inventory Boundary

## Goal
Enforce the inventory boundary for ability costs/requirements by routing all inventory access through SOTS_INV (no InvSP), integrate ApplyCosts into the SPINE_2 activation pipeline with WithReport flows, and add diagnostics.

## Changes
- Removed all InvSP references from SOTS_GAS_Plugin and pivoted inventory queries to SOTS_INV bridge helpers.
- Added an internal inventory cost adapter that calls `HasItemByTag_WithReport` and `TryConsumeItemByTag_WithReport`, maps results to `E_SOTS_AbilityActivateResult`, and logs when enabled.
- Wired the activation pipeline to run inventory requirement checks (dry-run safe) and consume costs on commit before activating, with structured debug reasons.
- Added a debug flag to log inventory cost ops per-ability component.

## Evidence
- No remaining InvSP usage inside SOTS_GAS_Plugin; inventory count now goes through `USOTS_InventoryBridgeSubsystem::GetCarriedItemCountByTags` [Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Components/SOTS_AbilityComponent.cpp#L330-L348].
- Inventory requirement gate uses SOTS_INV `HasItemByTag_WithReport` with activation-result mapping [Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Components/SOTS_AbilityComponent.cpp#L720-L744].
- ApplyCosts step consumes via `TryConsumeItemByTag_WithReport` before activation commit [Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Components/SOTS_AbilityComponent.cpp#L780-L802].
- Inventory op adapter + logging/mapping helpers added [Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Components/SOTS_AbilityComponent.cpp#L34-L119].
- Debug flag for inventory cost logging added to ability component API [Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/Components/SOTS_AbilityComponent.h#L73-L88].

## Verification
- Not run (per instructions).

## Cleanup
- Plugin binaries/intermediate deleted after changes.

## Follow-ups
- If future abilities need multi-tag or variable quantity costs, extend the adapter to handle those cases and avoid assuming a single tag/count=1.
