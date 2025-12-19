[CONTEXT_ANCHOR]
ID: 20251218_1656 | Plugin: SOTS_INV | Pass/Topic: PickupFlow | Owner: Buddy
Scope: Canonical pickup payload/result structs, service entrypoints, and delegates (SPINE_2).

DONE
- Added FSOTS_InvPickupRequest/FSOTS_InvPickupResult structs plus pickup delegates in inventory types.
- Exposed bridge subsystem pickup APIs (payload + simple) and facade wrappers; legacy signature now forwards to the simple path.
- Added provider resolver helper (controller→pawn→instigator→bridge) and pickup handling with CanPickup/ExecutePickup or AddItemByTag fallback; broadcasts OnPickupRequested/OnPickupCompleted.
- Documented pickup flow, identity rule, UI deferral, and deferred pickup actor consumption.

VERIFIED
- None (code-only; no build/runtime).

UNVERIFIED / ASSUMPTIONS
- Provider resolution order and gating logic are assumed correct; not runtime-validated.
- Pickup actor destruction is deferred; assumes caller will handle if needed.

FILES TOUCHED
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryTypes.h
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryBridgeSubsystem.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryBridgeSubsystem.cpp
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryFacadeLibrary.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryFacadeLibrary.cpp
- Plugins/SOTS_INV/Docs/SOTS_INV_PickupFlow.md

NEXT (Ryan)
- Validate pickup requests in editor (provider resolution order, Can/Execute calls, delegate firing).
- Decide on UI notification wiring for SPINE_3 and where pickup actor consumption should occur.
- Run a build/editor session to confirm no load errors from new APIs.

ROLLBACK
- Revert changes in the above files to the prior revision.
