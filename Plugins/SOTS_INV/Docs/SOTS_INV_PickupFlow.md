# SOTS_INV Pickup Flow (SPINE_2)

## Payload (FSOTS_InvPickupRequest)
- `InstigatorActor` (weak AActor): actor initiating the pickup.
- `PickupActor` (weak AActor, optional): world actor being picked up or granted.
- `ItemTag` (FGameplayTag): canonical item identifier; ItemId is `ItemTag.GetTagName()`.
- `Quantity` (int32): amount to add; defaults to 1.
- `ContextTags` (FGameplayTagContainer): optional context (e.g., Context.Interaction, Context.Loot, Context.Reward).
- `bConsumeWorldActorOnSuccess` (bool): intent for world consumption; consumption is currently deferred in C++.
- `bNotifyUIOnSuccess` (bool): intent to notify UI; actual UI routing is deferred to SPINE_3.

## Result (FSOTS_InvPickupResult)
- `bSuccess` (bool).
- `FailReason` (FText) and `FailTag` (FGameplayTag) for gating/error context.
- `QuantityApplied` (int32) applied to inventory when successful.
- `ItemId` (FName) derived from `ItemTag.GetTagName()`.

## Service surface (Blueprint-callable)
- `USOTS_InventoryBridgeSubsystem::RequestPickup(Request, OutResult)` — canonical entrypoint.
- `USOTS_InventoryBridgeSubsystem::RequestPickupSimple(Instigator, PickupActor, ItemTag, Quantity, bSuccess)` — convenience wrapper.
- Facade wrappers mirror the same signatures on `USOTS_InventoryFacadeLibrary` (WorldContext-based) and preserve the legacy signature for compatibility.

## Provider seam
- Provider remains Blueprint-canonical via `USOTS_InventoryProviderInterface`.
- Flow: resolve provider (controller → pawn → instigator → bridge fallback) → broadcast `OnPickupRequested` → call `CanPickup` → call `ExecutePickup` (or `AddItemByTag` for C++ providers) → broadcast `OnPickupCompleted`.
- No InvSP module/class dependency added.

## UI and pickup actor handling
- UI notifications are deferred to SPINE_3; `bNotifyUIOnSuccess` is recorded but not dispatched here.
- Pickup actor destruction/consumption is deferred to calling systems (C++ does not destroy the actor in SPINE_2).
