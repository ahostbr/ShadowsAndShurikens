# Worklog — SOTS_INV SPINE_3 UIBridge (Buddy)

## Goal
Wire SOTS_INV to request inventory UI/pickup notifications via SOTS_UI (no InvSP coupling), code-only.

## What changed
- Added UI notification flags to `FSOTS_InvPickupResult` and routed successful pickups to `USOTS_UIRouterSubsystem::RequestInvSP_NotifyPickupItem`; first-time pickup hook present but not tracked yet.
- Added Blueprint-callable inventory UI request functions on `USOTS_InventoryFacadeLibrary` forwarding to `RequestInvSP_Open/Close/Toggle/Refresh/SetShortcutMenuVisible` on the SOTS_UI router.
- Propagated `bNotifyUIOnSuccess` into pickup results and invoked UI notifications after successful pickup completion events.
- Added SOTS_UI dependency in `SOTS_INV.Build.cs` and `SOTS_INV.uplugin`.
- Documented the UI bridge behavior in `Docs/SOTS_INV_UIBridge.md`.

## Files changed
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryTypes.h
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryBridgeSubsystem.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryBridgeSubsystem.cpp
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryFacadeLibrary.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryFacadeLibrary.cpp
- Plugins/SOTS_INV/Source/SOTS_INV/SOTS_INV.Build.cs
- Plugins/SOTS_INV/SOTS_INV.uplugin
- Plugins/SOTS_INV/Docs/SOTS_INV_UIBridge.md

## Notes / risks / unknowns
- First-time pickup detection is deferred; `bIsFirstTimePickup` remains false until a provider/adapter supplies the signal.
- No runtime/build validation performed; integration with SOTS_UI router assumes existing RequestInvSP_* signatures remain stable.

## Verification status
- Unverified (no build/run; code review stage only).

## Cleanup
- Plugins/SOTS_INV/Binaries: deleted/absent after cleanup.
- Plugins/SOTS_INV/Intermediate: deleted/absent after cleanup.

## Follow-ups / next steps
- Runtime check that pickup notifications reach SOTS_UI and that UI request functions open/close/toggle/refresh correctly.
- Decide on first-time pickup detection source and wire `bIsFirstTimePickup` when available.
