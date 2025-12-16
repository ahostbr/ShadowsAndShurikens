#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SOTS_AIPerceptionSubsystem.generated.h"

class USOTS_AIPerceptionComponent;
class AActor;
struct FGameplayTag;

/**
 * World-level coordinator for SOTS AI perception.
 * Owns the set of all active perception components and broadcasts
 * noise events to them.
 */
UCLASS()
class SOTS_AIPERCEPTION_API USOTS_AIPerceptionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void RegisterPerceptionComponent(USOTS_AIPerceptionComponent* Comp);
    void UnregisterPerceptionComponent(USOTS_AIPerceptionComponent* Comp);

    // C++ entrypoint for noises (BP should use the Library wrapper).
    bool TryReportNoise(AActor* Instigator, const FVector& Location, float Loudness, FGameplayTag NoiseTag);
    void ReportNoise(AActor* Instigator, const FVector& Location, float Loudness, FGameplayTag NoiseTag);

    // C++ entrypoint for damage stimuli (BP should use the Library wrapper).
    bool TryReportDamageStimulus(AActor* VictimActor, AActor* InstigatorActor, float DamageAmount, FGameplayTag DamageTag, const FVector& Location, bool bHasLocation);

    // Optional query helpers.
    TArray<USOTS_AIPerceptionComponent*> GetAlertedAI() const;
    bool IsAnyoneAlerted() const;

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<USOTS_AIPerceptionComponent>> RegisteredComponents;
};

