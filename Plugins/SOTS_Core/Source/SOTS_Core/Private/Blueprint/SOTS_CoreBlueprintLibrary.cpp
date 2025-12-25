#include "Blueprint/SOTS_CoreBlueprintLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
    USOTS_CoreLifecycleSubsystem* ResolveLifecycleSubsystem(const UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return nullptr;
        }

        UWorld* World = WorldContextObject->GetWorld();
        if (!World)
        {
            return nullptr;
        }

        UGameInstance* GameInstance = World->GetGameInstance();
        if (!GameInstance)
        {
            return nullptr;
        }

        return GameInstance->GetSubsystem<USOTS_CoreLifecycleSubsystem>();
    }
}

USOTS_CoreLifecycleSubsystem* USOTS_CoreBlueprintLibrary::GetCoreLifecycle(const UObject* WorldContextObject)
{
    return ResolveLifecycleSubsystem(WorldContextObject);
}

FSOTS_CoreLifecycleSnapshot USOTS_CoreBlueprintLibrary::GetCoreLifecycleSnapshot(const UObject* WorldContextObject)
{
    if (USOTS_CoreLifecycleSubsystem* Subsystem = ResolveLifecycleSubsystem(WorldContextObject))
    {
        return Subsystem->GetCurrentSnapshot();
    }

    return FSOTS_CoreLifecycleSnapshot();
}

APlayerController* USOTS_CoreBlueprintLibrary::GetPrimaryPlayerController(const UObject* WorldContextObject)
{
    if (USOTS_CoreLifecycleSubsystem* Subsystem = ResolveLifecycleSubsystem(WorldContextObject))
    {
        return Subsystem->GetCurrentSnapshot().PrimaryPC;
    }

    return nullptr;
}

APawn* USOTS_CoreBlueprintLibrary::GetPrimaryPawn(const UObject* WorldContextObject)
{
    if (USOTS_CoreLifecycleSubsystem* Subsystem = ResolveLifecycleSubsystem(WorldContextObject))
    {
        return Subsystem->GetCurrentSnapshot().PrimaryPawn;
    }

    return nullptr;
}
