# Worklog — SOTS_UI BRIDGE InvSP CPPBacking (code-only)

## Goal
Add C++ backing so InvSP adapter surface is owned by SOTS_UI and callable via stable Blueprint functions, without InvSP dependencies or Blueprint asset creation.

## Changes
- Router now owns a transient `InvSPAdapterInstance` with optional `InvSPAdapterClassOverride`; `EnsureInvSPAdapter`/`GetInvSPAdapter` exposed as BlueprintCallable.
- Added BlueprintCallable request functions forwarding to adapter BlueprintNativeEvents: Toggle/Open/Close/Refresh inventory, SetShortcutMenuVisible, NotifyPickupItem, NotifyFirstTimePickup.
- Inventory/container flows call the adapter via ensure/get before UI push/pop.
- Doc added: `Docs/SOTS_UI_InvSP_CPPBacking.md`.

## Files Touched
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
- Plugins/SOTS_UI/Docs/SOTS_UI_InvSP_CPPBacking.md

## Notes / Assumptions
- No Blueprint assets created/edited; no runtime validation; no builds executed.
- Adapter override is optional; fallback uses config class or `USOTS_InvSPAdapter`.
- No hard references to InvSP modules/classes added.

## Verification
- Not run (code review stage only).

## Cleanup
- Deleted Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate.
