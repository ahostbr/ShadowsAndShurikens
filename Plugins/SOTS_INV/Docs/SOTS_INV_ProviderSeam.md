# SOTS_INV Provider Seam (SPINE_1)

- Item identity: `ItemId == ItemTag.GetTagName()` exposed via `ItemIdFromTag` (Blueprint callable in `USOTS_InventoryFacadeLibrary`).
- Blueprint provider interface (canonical v1): implement `USOTS_InventoryProviderInterface` BlueprintNativeEvents
  - `CanPickup(Instigator, PickupActor, ItemTag, Quantity, OutFailReason)`
  - `ExecutePickup(Instigator, PickupActor, ItemTag, Quantity)`
  - `AddItemByTag(Instigator, ItemTag, Quantity)`
  - `RemoveItemByTag(Instigator, ItemTag, Quantity)`
  - `GetItemCount(Instigator, ItemTag)`
- Service entrypoint: `RequestPickup(WorldContext, Instigator, PickupActor, ItemTag, Quantity, bSuccess)` (Facade library). Resolves any actor/component implementing the provider interface on instigator/controller/pawn; falls back to inventory bridge provider resolution. No InvSP calls or dependencies.
- No runtime validation or builds performed in this pass.
