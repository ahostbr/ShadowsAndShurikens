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
    static void SOTS_ReportNoise(
        UObject* WorldContextObject,
        AActor* Instigator,
        FVector Location,
        float Loudness,
        FGameplayTag NoiseTag);
};

