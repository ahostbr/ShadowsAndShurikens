#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SOTS_AIPerceptionSubsystem.generated.h"

class USOTS_AIPerceptionComponent;
class AActor;
struct FGameplayTag;
class APlayerController;
class APawn;

DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_AIPerception_OnCoreWorldReady, UWorld*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTS_AIPerception_OnCorePrimaryPlayerReady, APlayerController*, APawn*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_AIPerception_OnCorePreLoadMap, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_AIPerception_OnCorePostLoadMap, UWorld*);

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

    // [BRIDGE11] Optional Core bridge seams (state-only).
    void HandleCoreWorldReady(UWorld* World);
    void HandleCorePrimaryPlayerReady(APlayerController* PC, APawn* Pawn);
    void HandleCorePreLoadMap(const FString& MapName);
    void HandleCorePostLoadMap(UWorld* World);

    FSOTS_AIPerception_OnCoreWorldReady OnCoreWorldReady;
    FSOTS_AIPerception_OnCorePrimaryPlayerReady OnCorePrimaryPlayerReady;
    FSOTS_AIPerception_OnCorePreLoadMap OnCorePreLoadMap;
    FSOTS_AIPerception_OnCorePostLoadMap OnCorePostLoadMap;

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<USOTS_AIPerceptionComponent>> RegisteredComponents;

    UPROPERTY(Transient)
    TWeakObjectPtr<UWorld> LastCoreWorldReady;

    UPROPERTY(Transient)
    TWeakObjectPtr<APlayerController> LastCorePrimaryPC;

    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> LastCorePrimaryPawn;

    UPROPERTY(Transient)
    FString LastCorePreLoadMapName;

    UPROPERTY(Transient)
    TWeakObjectPtr<UWorld> LastCorePostLoadMapWorld;
};

