#include "SOTS_AIStealthBridgeComponent.h"

#include "SOTS_GlobalStealthManagerSubsystem.h"

USOTS_AIStealthBridgeComponent::USOTS_AIStealthBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    Suspicion01 = 0.0f;
}

void USOTS_AIStealthBridgeComponent::SetSuspicion01(float In01)
{
    Suspicion01 = FMath::Clamp(In01, 0.0f, 1.0f);

    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        GSM->SetAISuspicion(Suspicion01);
    }
}

