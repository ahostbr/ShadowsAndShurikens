# Buddy Worklog — SOTS_INV Facade Surface (2025-12-13 14:25:00)

## 1) Summary
Established a clear SOTS_INV bridge contract: subsystem-first access with a Blueprint-friendly provider interface, no InvSP C++ dependency required.

## 2) Context
Goal: remove TODO ambiguity about InventorySystemPro/InvSP modules and present a stable, minimal facade for other plugins (GAS/SkillTree/UI) to query/consume inventory by gameplay tag.

## 3) Changes Made
- Clarified Build.cs to declare SOTS_INV as a SOTS-side bridge (InvSP integrates via Blueprint adapters; no InvSP C++ module required).
- Added `USOTS_InventoryProviderInterface` for BP/native providers to expose inventory counts/consume/equipped tag without string lookups.
- Exposed façade methods on `USOTS_InventoryBridgeSubsystem`: HasItem/GetItemCount/TryConsume/GetEquippedItemTag, preferring provider interface then InvSP component fallback.

## 4) Blessed API Surface
Other plugins should call on `USOTS_InventoryBridgeSubsystem` (get via `Get(WorldContext)`):
- `bool HasItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count=1) const`
- `int32 GetItemCountByTag(AActor* Owner, FGameplayTag ItemTag) const`
- `bool TryConsumeItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count=1)`
- `bool GetEquippedItemTag(AActor* Owner, FGameplayTag& OutItemTag) const` (optional; returns false if not provided)

## 5) Integration Seam (InvSP)
- InvSP C++ module is assumed absent; integration is BP-only via `USOTS_InventoryProviderInterface` implementations.
- Default fallback still supports `UInvSP_InventoryComponent` when present, with no string/class-name checks.

## 6) Testing
- Not run (C++ surface-only refactor). No behavioral changes to stored inventory data.

## 7) Risks / Considerations
- Providers must implement the interface to participate; otherwise fallback uses `UInvSP_InventoryComponent` carried items only.
- Equipped item retrieval depends on provider support; otherwise returns false.

## 8) Follow-ups
- Optionally migrate existing BP inventory actors/components to implement `USOTS_InventoryProviderInterface` for consistent semantics.
