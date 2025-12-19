# Worklog — SOTS_INV SPINE_2 Pickup Flow (Buddy)

## Goal
Define the canonical pickup request flow (payload/result structs, provider seam, events) without InvSP coupling; code-only, no build/runtime.

## What changed
- Added public structs `FSOTS_InvPickupRequest` and `FSOTS_InvPickupResult` plus pickup delegates for requested/completed signals.
- Exposed Blueprint-callable pickup entrypoints on `USOTS_InventoryBridgeSubsystem` and mirrored wrappers on `USOTS_InventoryFacadeLibrary` (payload + simple overload), keeping the legacy signature forwarding to the new path.
- Implemented provider resolution helper on the bridge (controller→pawn→instigator→bridge), provider gating via `CanPickup`/`ExecutePickup` (or `AddItemByTag` for C++ providers), and delegate broadcasts.
- Documented the flow in `Docs/SOTS_INV_PickupFlow.md` with item identity rule, provider ownership, UI deferral, and pickup actor handling (deferred).

## Files touched
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryTypes.h
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryBridgeSubsystem.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryBridgeSubsystem.cpp
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryFacadeLibrary.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryFacadeLibrary.cpp
- Plugins/SOTS_INV/Docs/SOTS_INV_PickupFlow.md

## Notes / risks / unknowns
- Behavior unverified (no build/run); provider gating and delegate flow not runtime-tested.
- Pickup actor consumption intentionally deferred; callers must handle world actor destruction if desired.
- Fail tags are left empty; no tag registry additions were introduced.

## Verification status
- Unverified (code review stage only; no builds or runtime).

## Cleanup
- Plugins/SOTS_INV/Binaries: deleted/absent after check.
- Plugins/SOTS_INV/Intermediate: deleted/absent after check.

## Follow-ups / next steps
- Runtime validation that provider resolution and pickup flow behave as expected in editor/game.
- Decide on UI notification wiring in SPINE_3; evaluate whether/where pickup actor consumption should occur.
