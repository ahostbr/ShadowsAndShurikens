#include "GameFramework/SOTS_GameModeBase.h"

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

void ASOTS_GameModeBase::StartPlay()
{
    Super::StartPlay();

    if (USOTS_CoreLifecycleSubsystem* Subsystem = GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyWorldStartPlay(GetWorld());
    }
}

void ASOTS_GameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (USOTS_CoreLifecycleSubsystem* Subsystem = GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyPostLogin(this, NewPlayer);
    }
}

void ASOTS_GameModeBase::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    if (USOTS_CoreLifecycleSubsystem* Subsystem = GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyLogout(this, Exiting);
    }
}
