# SOTS_INV — Completeness Pass PLAN (Inventory Bridge/Facade)

## Current state (evidence)
- **Subsystem entry**: `USOTS_InventoryBridgeSubsystem` (`Public/SOTS_InventoryBridgeSubsystem.h`, `Private/SOTS_InventoryBridgeSubsystem.cpp`).
- **Provider resolution**: `ResolveInventoryProvider` checks owner or components for `USOTS_InventoryProviderInterface` only; otherwise falls back to direct InvSP component search in call sites (e.g., `HasItemByTag`, `GetItemCountByTag`, `TryConsumeItemByTag`), no caching or pawn-change handling (`Private/SOTS_InventoryBridgeSubsystem.cpp`).
- **Provider API usage**: Calls `HasItemByTag / GetItemCountByTag / TryConsumeItemByTag / GetEquippedItemTag` on the provider interface; if missing, directly manipulates `UInvSP_InventoryComponent` carried items arrays (`Private/SOTS_InventoryBridgeSubsystem.cpp`).
- **Profile sync**: `BuildProfileData` / `ApplyProfileData` copy items/quickslots via `FindPlayerCarriedInventoryComponent`/`FindPlayerStashInventoryComponent` (first player pawn only), no validation/deferred apply; caches copies (`CachedCarriedItems`, etc.) as fallback if components absent, but reapply logic is shallow (`Private/SOTS_InventoryBridgeSubsystem.cpp`).
- **Public API surface**: Add/Clear/SetQuickSlot/HasItemByTag/GetItemCountByTag/TryConsumeItemByTag/GetEquippedItemTag; also tag-container helpers and profile build/apply (`Public/SOTS_InventoryBridgeSubsystem.h`).
- **InvSP reference**: BEP export inspected at `E:/SAS/ShadowsAndShurikens/BEP_EXPORTS/INVSP/` (StructSchemas.txt present) to confirm InvSP component naming (`UInvSP_InventoryComponent`) aligns with current bridge calls.
- **Diagnostics**: No dev-only warnings or result objects; failures return false/zero silently; no readiness state.

## Missing behaviors / weaknesses (with pointers)
- **Provider brittleness**: Only first-player pawn lookup, no explicit failure reasons, no refresh/cache, no override seam (`FindPlayerCarriedInventoryComponent`, `ResolveInventoryProvider`).
- **Silent failures**: Tag queries and consume paths return false without context; no result contract (`HasItemByTag`, `TryConsumeItemByTag`).
- **Profile robustness**: Apply does not validate provider readiness; cached items used only in some branches; no deferred apply or versioning (`ApplyProfileData`, `BuildProfileData`).
- **Integration hooks**: No GAS/SkillTree/UI adapter helpers beyond raw tag queries; no inventory-open intent.
- **Diagnostics**: No dev-only warnings for missing provider, stale cache, or apply-before-ready.

## Spine plan (next passes)
1) **SPINE_1: Provider seam & robustness**
   - Add provider resolution strategy (override/config, interface, InvSP component) with refresh on pawn change.
   - Introduce readiness flag + dev-only warnings; keep legacy APIs intact.
2) **SPINE_2: Result contracts**
   - Add `_WithReport` variants returning enum/result struct (Success/ProviderMissing/ItemNotFound/QuantityInsufficient/etc.); wire legacy APIs through them.
3) **SPINE_3: Profile hardening**
   - Validate before apply; optional deferred apply queue until provider ready; ensure cache/version safety.
4) **SPINE_4: Integration & diagnostics**
   - Add helper shims for GAS/SkillTree/UI intent (non-UI creation); add dev-only diagnostics guarded for non-shipping/test; provider missing warnings.

## Behavior promise
- Defaults preserve current behavior (no forced overrides or new logging unless enabled).
- All new diagnostics/dev warnings will be gated and non-shipping/test only.
