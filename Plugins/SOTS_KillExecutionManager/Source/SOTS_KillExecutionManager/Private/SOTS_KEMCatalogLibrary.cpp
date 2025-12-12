#include "SOTS_KEMCatalogLibrary.h"

#include "Engine/GameInstance.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "SOTS_KEM_ExecutionCatalog.h"
#include "UObject/SoftObjectPath.h"

USOTS_KEM_ExecutionCatalog* USOTS_KEMCatalogLibrary::GetExecutionCatalog(const UObject* WorldContextObject)
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

    USOTS_KEMManagerSubsystem* Manager = GameInstance->GetSubsystem<USOTS_KEMManagerSubsystem>();
    if (!Manager)
    {
        return nullptr;
    }

    if (Manager->ExecutionCatalog.IsValid())
    {
        return Manager->ExecutionCatalog.Get();
    }

    if (Manager->ExecutionCatalog.ToSoftObjectPath().IsValid())
    {
        return Manager->ExecutionCatalog.LoadSynchronous();
    }

    return nullptr;
}
