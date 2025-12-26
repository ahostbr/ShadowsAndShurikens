# Buddy Worklog — 2025-12-26 14:01 — UE5.7 AssetRegistry include fix

## Goal
Fix UE 5.7 compilation failure in `SOTS_BPGenBridgeAssetOps.cpp` where `AssetRegistry/AssetRegistryTypes.h` is missing.

## What changed
- Removed `#include "AssetRegistry/AssetRegistryTypes.h"` from asset ops since it is not present in this UE 5.7 install and the file already includes `AssetRegistryModule.h` + `IAssetRegistry.h`.

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeAssetOps.cpp

## Verification
- UNVERIFIED: No build run here.

## Next steps (Ryan)
- Rebuild in UE 5.7.1; confirm `SOTS_BPGen_Bridge` compiles past asset ops.
