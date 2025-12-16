#include "SOTS_AIPerceptionSubsystem.h"

#include "SOTS_AIPerceptionComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/World.h"

void USOTS_AIPerceptionSubsystem::RegisterPerceptionComponent(USOTS_AIPerceptionComponent* Comp)
{
    if (!Comp)
    {
        return;
    }

    RegisteredComponents.AddUnique(Comp);
}

void USOTS_AIPerceptionSubsystem::UnregisterPerceptionComponent(USOTS_AIPerceptionComponent* Comp)
{
    if (!Comp)
    {
        return;
    }

    RegisteredComponents.RemoveAll([Comp](const TWeakObjectPtr<USOTS_AIPerceptionComponent>& Ptr)
    {
        return Ptr.Get() == Comp;
    });
}

bool USOTS_AIPerceptionSubsystem::TryReportNoise(AActor* Instigator, const FVector& Location, float Loudness, FGameplayTag NoiseTag)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const float ClampedLoudness = FMath::Clamp(Loudness, 0.0f, 1.0f);

    bool bDelivered = false;

    for (const TWeakObjectPtr<USOTS_AIPerceptionComponent>& CompPtr : RegisteredComponents)
    {
        USOTS_AIPerceptionComponent* Comp = CompPtr.Get();
        if (!Comp)
        {
            continue;
        }

        // Let the component decide how to interpret the noise based on
        // its configuration and distance.
        Comp->HandleReportedNoise(Location, ClampedLoudness, Instigator, NoiseTag);
        bDelivered = true;
    }

    return bDelivered;
}

void USOTS_AIPerceptionSubsystem::ReportNoise(AActor* Instigator, const FVector& Location, float Loudness, FGameplayTag NoiseTag)
{
    TryReportNoise(Instigator, Location, Loudness, NoiseTag);
}

bool USOTS_AIPerceptionSubsystem::TryReportDamageStimulus(AActor* VictimActor, AActor* InstigatorActor, float DamageAmount, FGameplayTag DamageTag, const FVector& Location, bool bHasLocation)
{
    if (!VictimActor)
    {
        return false;
    }

    USOTS_AIPerceptionComponent* VictimComp = VictimActor->FindComponentByClass<USOTS_AIPerceptionComponent>();

    if (!VictimComp)
    {
        for (const TWeakObjectPtr<USOTS_AIPerceptionComponent>& CompPtr : RegisteredComponents)
        {
            if (USOTS_AIPerceptionComponent* Comp = CompPtr.Get())
            {
                if (Comp->GetOwner() == VictimActor)
                {
                    VictimComp = Comp;
                    break;
                }
            }
        }
    }

    if (!VictimComp)
    {
        return false;
    }

    VictimComp->ApplyDamageStimulus(InstigatorActor, DamageAmount, DamageTag, Location, bHasLocation);
    return true;
}

TArray<USOTS_AIPerceptionComponent*> USOTS_AIPerceptionSubsystem::GetAlertedAI() const
{
    TArray<USOTS_AIPerceptionComponent*> Result;

    for (const TWeakObjectPtr<USOTS_AIPerceptionComponent>& CompPtr : RegisteredComponents)
    {
        if (USOTS_AIPerceptionComponent* Comp = CompPtr.Get())
        {
            if (Comp->GetCurrentPerceptionState() == ESOTS_PerceptionState::Alerted)
            {
                Result.Add(Comp);
            }
        }
    }

    return Result;
}

bool USOTS_AIPerceptionSubsystem::IsAnyoneAlerted() const
{
    for (const TWeakObjectPtr<USOTS_AIPerceptionComponent>& CompPtr : RegisteredComponents)
    {
        if (USOTS_AIPerceptionComponent* Comp = CompPtr.Get())
        {
            if (Comp->GetCurrentPerceptionState() == ESOTS_PerceptionState::Alerted)
            {
                return true;
            }
        }
    }

    return false;
}

