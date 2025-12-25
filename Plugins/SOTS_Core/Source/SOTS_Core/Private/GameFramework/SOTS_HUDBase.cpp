#include "GameFramework/SOTS_HUDBase.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

namespace
{
    USOTS_CoreLifecycleSubsystem* GetLifecycleSubsystem(const UObject* WorldContext)
    {
        if (!WorldContext)
        {
            return nullptr;
        }

        UWorld* World = WorldContext->GetWorld();
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

void ASOTS_HUDBase::BeginPlay()
{
    Super::BeginPlay();

    if (USOTS_CoreLifecycleSubsystem* Subsystem = GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyHUDReady(this);
    }
}
