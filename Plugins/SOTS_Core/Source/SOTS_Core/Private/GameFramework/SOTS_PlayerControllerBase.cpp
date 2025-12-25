#include "GameFramework/SOTS_PlayerControllerBase.h"

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

void ASOTS_PlayerControllerBase::BeginPlay()
{
    Super::BeginPlay();

    if (USOTS_CoreLifecycleSubsystem* Subsystem = GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyPlayerControllerBeginPlay(this);
    }
}

void ASOTS_PlayerControllerBase::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (USOTS_CoreLifecycleSubsystem* Subsystem = GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyPawnPossessed(this, InPawn);
    }
}
