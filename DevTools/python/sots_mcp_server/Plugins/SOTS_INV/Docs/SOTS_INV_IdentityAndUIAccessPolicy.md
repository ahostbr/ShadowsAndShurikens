# SOTS_INV Identity and UI Access Policy

## Item identity rule
- `ItemId` is always derived from the owning `FGameplayTag` name so `ItemId == ItemTag.GetTagName()`, with `USOTS_InventoryFacadeLibrary::ItemIdFromTag` delivering the canonical `FName`.
- `USOTS_InventoryBridgeSubsystem::RequestPickup` seeds `FSOTS_InvPickupResult::ItemId` this way and every provider traversal in `InvSPInventoryProviderComponent` or the bridge compares `Item.ItemId` against the same `TagName`, rejecting entries that drift.
- Snapshot validation and QuickSlot binding checks drop or warn about `None`, duplicate, or mismatched ItemIds, ensuring the identity rule continues to hold for persisted/replicated data.

## Provider canonicalization
- `USOTS_InventoryBridgeSubsystem` registers with `USOTS_ProfileSubsystem`, resolves providers via `ResolveInventoryProvider`/`TryResolveInventoryProviderNow`, and caches the object that implements either `ISOTS_InventoryProvider` or `USOTS_InventoryProviderInterface`.
- `USOTS_InventoryFacadeLibrary::GetInventoryProviderFromWorldContext` first probes controller/pawn components that support the interface, then falls back to the bridge's `GetResolvedProvider_ForUI`. `RunUIHelper` wraps the open/close/toggle helpers so UI helpers only call providers that report ready while logging failures consistently.
- The instigator → context actor → bridge chain is the only path through which SOTS_INV reads or mutates inventory data for gameplay or UI flows, keeping provider resolution deterministic and documented.

## UI access policy
- Every UI intent (`RequestOpen/Close/Toggle/RefreshInventoryUI`, `RequestSetShortcutMenuVisible`, `RequestInvSP_NotifyPickupItem`, `RequestInvSP_NotifyFirstTimePickup`) routes through `USOTS_InventoryFacadeLibrary` helpers that resolve `SOTS_UIRouterSubsystem` via `FacadeResolveUIRouter` and invoke the `RequestInvSP_*` entrypoints.
- Because the facade only calls into the router, SOTS_INV never owns Push/Pop/Focus or input policy; it just emits intents and leaves widget ownership to `USOTS_UIRouterSubsystem`.
- A workspace search for `RequestInvSP_*` shows only the router implementation, its docs, and these facade helpers, so no other plugin directly touches InvSP, keeping SOTS_UI as the sole widget owner while the bridge/facade cooperate on intent delivery.
