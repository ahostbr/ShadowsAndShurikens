#include "GameFramework/SOTS_GameModeBase.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystemUtil.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

void ASOTS_GameModeBase::StartPlay()
{
    Super::StartPlay();

    if (USOTS_CoreLifecycleSubsystem* Subsystem = SOTS_Core::Private::GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyWorldStartPlay(GetWorld());
    }
}

void ASOTS_GameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (USOTS_CoreLifecycleSubsystem* Subsystem = SOTS_Core::Private::GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyPostLogin(this, NewPlayer);
    }
}

void ASOTS_GameModeBase::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    if (USOTS_CoreLifecycleSubsystem* Subsystem = SOTS_Core::Private::GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyLogout(this, Exiting);
    }
}
