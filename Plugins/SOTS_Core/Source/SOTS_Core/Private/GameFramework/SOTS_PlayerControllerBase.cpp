#include "GameFramework/SOTS_PlayerControllerBase.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystemUtil.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

void ASOTS_PlayerControllerBase::BeginPlay()
{
    Super::BeginPlay();

    if (USOTS_CoreLifecycleSubsystem* Subsystem = SOTS_Core::Private::GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyPlayerControllerBeginPlay(this);
    }
}

void ASOTS_PlayerControllerBase::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (USOTS_CoreLifecycleSubsystem* Subsystem = SOTS_Core::Private::GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyPawnPossessed(this, InPawn);
    }
}
