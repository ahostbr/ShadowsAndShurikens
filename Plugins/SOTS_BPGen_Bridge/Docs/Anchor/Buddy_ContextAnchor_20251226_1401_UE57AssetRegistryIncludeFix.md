Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1401 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: UE57_AssetRegistryIncludeFix | Owner: Buddy
Scope: Fix missing AssetRegistry header include for UE 5.7

DONE
- Removed `#include "AssetRegistry/AssetRegistryTypes.h"` from `SOTS_BPGenBridgeAssetOps.cpp`.

VERIFIED
- UNVERIFIED (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Assumed `AssetRegistryModule.h` / `IAssetRegistry.h` provide required types for this file.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeAssetOps.cpp

NEXT (Ryan)
- Rebuild on UE 5.7.1; confirm compile.

ROLLBACK
- Revert the file above.
