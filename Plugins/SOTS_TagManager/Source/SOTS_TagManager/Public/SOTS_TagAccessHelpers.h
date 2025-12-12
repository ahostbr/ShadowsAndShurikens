#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SOTS_GameplayTagManagerSubsystem.h"

/**
 * Lightweight helper for obtaining the SOTS tag manager
 * subsystem from an arbitrary world context object.
 *
 * Kept header-only so dependent modules can include it
 * without additional link-time coupling.
 *
 * SOTS TAG SPINE (V2): This is the preferred way to obtain the global
 * Tag Manager subsystem. Calls that intend to add/remove or query
 * shared gameplay tags should prefer this helper or the
 * USOTS_TagLibrary wrapper to ensure they go through the central API.
 */
inline USOTS_GameplayTagManagerSubsystem* SOTS_GetTagSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<USOTS_GameplayTagManagerSubsystem>();
}

