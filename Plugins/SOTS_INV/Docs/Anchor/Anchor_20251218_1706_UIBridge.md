[CONTEXT_ANCHOR]
ID: 20251218_1706 | Plugin: SOTS_INV | Pass/Topic: UIBridge | Owner: Buddy
Scope: Route SOTS_INV pickups and inventory UI requests through SOTS_UI RequestInvSP entrypoints (no InvSP coupling).

DONE
- Added UI notification flags to FSOTS_InvPickupResult; pickup success now routes RequestInvSP_NotifyPickupItem via SOTS_UIRouterSubsystem, with first-time hook stubbed.
- Added Blueprint-callable inventory UI request functions in SOTS_InventoryFacadeLibrary forwarding to RequestInvSP_Open/Close/Toggle/Refresh/SetShortcutMenuVisible.
- Propagated bNotifyUIOnSuccess into results and invoked notifications post OnPickupCompleted.
- Declared SOTS_UI dependency in Build.cs and uplugin; documented behavior in SOTS_INV_UIBridge.md.

VERIFIED
- None (code-only; no build/runtime).

UNVERIFIED / ASSUMPTIONS
- RequestInvSP_* APIs remain stable; no runtime validation.
- bIsFirstTimePickup stays false until providers surface first-time info.

FILES TOUCHED
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryTypes.h
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryBridgeSubsystem.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryBridgeSubsystem.cpp
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryFacadeLibrary.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryFacadeLibrary.cpp
- Plugins/SOTS_INV/Source/SOTS_INV/SOTS_INV.Build.cs
- Plugins/SOTS_INV/SOTS_INV.uplugin
- Plugins/SOTS_INV/Docs/SOTS_INV_UIBridge.md

NEXT (Ryan)
- Run editor/runtime to confirm pickup notifications reach SOTS_UI and inventory UI request functions operate correctly.
- Decide how to set bIsFirstTimePickup and wire RequestInvSP_NotifyFirstTimePickup when source data exists.
- Keep an eye on any SOTS_UI RequestInvSP signature changes.

ROLLBACK
- Revert the listed files to the prior revision.
