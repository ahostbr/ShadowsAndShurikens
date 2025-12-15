# SOTS_INV — SPINE 1 (Provider Seam & Robust Resolution)

## Summary
- Added a dedicated inventory provider interface (`ISOTS_InventoryProvider`) and an InvSP adapter component (`UInvSPInventoryProviderComponent`) to avoid hard dependency on InvSP being present on every pawn.
- Introduced deterministic provider resolution and caching in `USOTS_InventoryBridgeSubsystem` with optional dev-only diagnostics; operations now use the provider seam before falling back to legacy InvSP component logic.
- Added BP helpers to resolve/check provider readiness without changing existing API signatures.

## Key changes
- **Interface seam:** `Interfaces/SOTS_InventoryProvider.h` defines readiness, tag queries/consume, add/remove, and optional UI hooks.
- **InvSP adapter:** `Public/InvSPInventoryProviderComponent.h` / `Private/InvSPInventoryProviderComponent.cpp` wraps `UInvSP_InventoryComponent` calls (verified against InvSP export: `UInvSP_InventoryComponent`, `AddCarriedItem`, `GetCarriedItems`, `ClearCarriedItems`).
- **Subsystem resolution:** `USOTS_InventoryBridgeSubsystem` caches providers, exposes `TryResolveInventoryProviderNow` / `IsInventoryProviderReady`, and gates calls through `GetResolvedProvider` before legacy fallbacks. Deterministic order: explicit resolve helper → provider interface on owner/components (including adapter) → legacy direct `UInvSP_InventoryComponent` fallback. Dev-only warning toggles: `bDebugLogProviderResolution`, `bWarnOnProviderMissing`, `bWarnOnProviderNotReady`.

## Evidence pointers
- Interface: `Plugins/SOTS_INV/Source/SOTS_INV/Public/Interfaces/SOTS_InventoryProvider.h`.
- Adapter: `Plugins/SOTS_INV/Source/SOTS_INV/Public/InvSPInventoryProviderComponent.h`, `Private/InvSPInventoryProviderComponent.cpp`.
- Subsystem resolution & caching: `TryResolveInventoryProviderNow`, `EnsureProviderResolved`, `GetResolvedProvider` in `Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryBridgeSubsystem.cpp`; config flags and cached provider fields in `Public/SOTS_InventoryBridgeSubsystem.h`.
- Legacy API routing through seam: `HasItemByTag`, `GetItemCountByTag`, `TryConsumeItemByTag`, `GetEquippedItemTag` updated to use provider interface first, then legacy fallback (same cpp).

## Behavior impact
- Default gameplay remains: legacy paths still function; new diagnostics are opt-in and dev-only. Provider seam enables future result contracts without breaking existing calls.

## Notes on BEP_EXPORTS usage
- Verified InvSP component/class names and callable methods against `E:/SAS/ShadowsAndShurikens/BEP_EXPORTS/INVSP` (e.g., `UInvSP_InventoryComponent`, `GetCarriedItems`, `AddCarriedItem`, `ClearCarriedItems`), ensuring adapter forwards to real APIs.
