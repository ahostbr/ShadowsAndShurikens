#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_AIPerceptionLibrary.generated.h"

UCLASS()
class SOTS_AIPERCEPTION_API USOTS_AIPerceptionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="SOTS|Perception", meta=(WorldContext="WorldContextObject"))
    static bool SOTS_TryReportNoise(
        UObject* WorldContextObject,
        AActor* Instigator,
        FVector Location,
        float Loudness,
        FGameplayTag NoiseTag,
        bool bLogIfFailed = false);

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception", meta=(WorldContext="WorldContextObject", DeprecatedFunction, DeprecationMessage="Use SOTS_TryReportNoise for explicit success/failure"))
    static void SOTS_ReportNoise(
        UObject* WorldContextObject,
        AActor* Instigator,
        FVector Location,
        float Loudness,
        FGameplayTag NoiseTag);

    UFUNCTION(BlueprintCallable, Category="SOTS|AIPerception", meta=(WorldContext="WorldContextObject"))
    static bool SOTS_TryReportDamageStimulus(
        UObject* WorldContextObject,
        AActor* VictimActor,
        AActor* InstigatorActor,
        float DamageAmount,
        FGameplayTag DamageTag,
        FVector Location,
        bool bHasLocation,
        bool bLogIfFailed = false);
};

