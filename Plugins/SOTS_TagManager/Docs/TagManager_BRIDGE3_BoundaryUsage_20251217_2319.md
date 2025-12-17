# TagManager BRIDGE3 Boundary Usage (20251217_2319)

## Decision rule
- Use TagManager/TagLibrary when reading or writing shared runtime actor-state tags on Player/Dragon/Guards/Pickups that other plugins observe or gate on.
- Bypass TagManager only for internal/local tags that never leave the owning plugin’s implementation or private containers.

## Boundary tags seen in C++ (writers/readers)
- `SAS.Stealth.Tier.{Hidden,Cautious,Danger,Compromised}`, `SAS.Stealth.InCover`, `SAS.Stealth.InShadow`, `SAS.Stealth.Detected` — written on Player by `USOTS_PlayerStealthComponent` via TagManager (`UpdateTagPresence`); consumed by other plugins via TagManager/TagLibrary queries (stealth/UI/AI gates).

## Code sites changed
- Added union-tag getter surface in TagManager: `USOTS_GameplayTagManagerSubsystem::GetActorTags` and `USOTS_TagLibrary::GetActorTags` to expose IGameplayTagAssetInterface + loose + scoped tags via one call.
  - [Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_GameplayTagManagerSubsystem.h](Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_GameplayTagManagerSubsystem.h)
  - [Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_GameplayTagManagerSubsystem.cpp](Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_GameplayTagManagerSubsystem.cpp)
  - [Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_TagLibrary.h](Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_TagLibrary.h)
  - [Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_TagLibrary.cpp](Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_TagLibrary.cpp)
- Adopted TagManager union query for GAS player tag debug logging so loose/shared tags are included: [Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_DebugLibrary.cpp](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_DebugLibrary.cpp)

## Notes
- Priority plugins touched: SOTS_TagManager, SOTS_GAS_Plugin.
- Cleanup: Binaries/ and Intermediate/ removed for both touched plugins (no builds run).
