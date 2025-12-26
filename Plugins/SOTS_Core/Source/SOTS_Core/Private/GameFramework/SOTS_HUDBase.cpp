#include "GameFramework/SOTS_HUDBase.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystemUtil.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

void ASOTS_HUDBase::BeginPlay()
{
    Super::BeginPlay();

    if (USOTS_CoreLifecycleSubsystem* Subsystem = SOTS_Core::Private::GetLifecycleSubsystem(this))
    {
        Subsystem->NotifyHUDReady(this);
    }
}
