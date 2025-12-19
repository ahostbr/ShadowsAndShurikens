# SOTS_INV FIX InterfaceExecuteWrappers (2025-12-18 20:08)

- Declared BlueprintNativeEvent interface methods (`GetItemCountByTag`, `HasItemByTag`, `TryConsumeItemByTag`, `GetEquippedItemTag`) in `SOTS_InventoryProviderInterface.h` so UHT generates `Execute_` wrappers used by the bridge.
- Kept existing provider methods; intent is compile/unblock only.
- No runtime testing performed.
