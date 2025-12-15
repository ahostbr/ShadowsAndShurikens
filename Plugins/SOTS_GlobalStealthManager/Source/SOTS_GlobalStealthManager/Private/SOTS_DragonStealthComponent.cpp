#include "SOTS_DragonStealthComponent.h"

#include "SOTS_GlobalStealthManagerSubsystem.h"

USOTS_DragonStealthComponent::USOTS_DragonStealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CachedState = FSOTS_PlayerStealthState();
}

void USOTS_DragonStealthComponent::BeginPlay()
{
    Super::BeginPlay();

    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        GSM->OnStealthStateChanged.AddUObject(this, &USOTS_DragonStealthComponent::HandleStealthStateChanged);
    }
}

void USOTS_DragonStealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        GSM->OnStealthStateChanged.RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}

void USOTS_DragonStealthComponent::HandleStealthStateChanged(const FSOTS_PlayerStealthState& InState)
{
    CachedState = InState;
    LightLevel01 = FMath::Clamp(InState.LightLevel01, 0.0f, 1.0f);
    AIPerceptionLevel01 = FMath::Clamp(InState.AISuspicion01, 0.0f, 1.0f);
    OverallDetection01 = FMath::Clamp(InState.GlobalStealthScore01, 0.0f, 1.0f);
}
