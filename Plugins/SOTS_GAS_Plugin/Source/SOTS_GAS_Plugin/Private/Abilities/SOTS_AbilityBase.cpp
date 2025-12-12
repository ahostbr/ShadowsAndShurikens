#include "Abilities/SOTS_AbilityBase.h"

#include "Components/SOTS_AbilityComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"

void USOTS_AbilityBase::Initialize(UAC_SOTS_Abilitys* InComponent,
                                   const F_SOTS_AbilityDefinition& InDefinition,
                                   const F_SOTS_AbilityHandle& InHandle)
{
    OwningComponent = InComponent;
    OwningActor     = InComponent ? InComponent->GetOwner() : nullptr;
    AbilityTag      = InDefinition.AbilityTag;
    Handle          = InHandle;

    OnAbilityGranted(InDefinition);
}

float USOTS_AbilityBase::GetOwnerLightLevel01() const
{
    if (!OwningActor)
    {
        return 0.0f;
    }

    if (UWorld* World = OwningActor->GetWorld())
    {
        if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(World))
        {
            return GSM->GetStealthState().LightLevel01;
        }
    }

    return 0.0f;
}

float USOTS_AbilityBase::GetOwnerGlobalStealthScore01() const
{
    if (!OwningActor)
    {
        return 0.0f;
    }

    if (UWorld* World = OwningActor->GetWorld())
    {
        if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(World))
        {
            return GSM->GetStealthState().GlobalStealthScore01;
        }
    }

    return 0.0f;
}

ESOTS_StealthTier USOTS_AbilityBase::GetOwnerStealthTier() const
{
    if (!OwningActor)
    {
        return ESOTS_StealthTier::Hidden;
    }

    if (UWorld* World = OwningActor->GetWorld())
    {
        if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(World))
        {
            return GSM->GetStealthState().StealthTier;
        }
    }

    return ESOTS_StealthTier::Hidden;
}

void USOTS_AbilityBase::ApplyStealthModifierToWorld(const FSOTS_StealthModifier& Modifier) const
{
    if (!OwningActor)
    {
        return;
    }

    if (UWorld* World = OwningActor->GetWorld())
    {
        if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(World))
        {
            GSM->AddStealthModifier(Modifier);
        }
    }
}

void USOTS_AbilityBase::RemoveStealthModifierFromWorld(FName SourceId) const
{
    if (SourceId.IsNone() || !OwningActor)
    {
        return;
    }

    if (UWorld* World = OwningActor->GetWorld())
    {
        if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(World))
        {
            GSM->RemoveStealthModifierBySource(SourceId);
        }
    }
}
