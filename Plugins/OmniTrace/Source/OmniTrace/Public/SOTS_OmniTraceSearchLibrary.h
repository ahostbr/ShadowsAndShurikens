#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_OmniTraceSearchLibrary.generated.h"

USTRUCT(BlueprintType)
struct FSOTS_SearchPatternResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="OmniTrace")
    TArray<FVector> Waypoints;
};

/**
 * SOTS-specific helper for requesting simple search patterns from OmniTrace.
 * Phase 1 implementation is intentionally minimal: a basic circle pattern.
 */
UCLASS()
class OMNITRACE_API USOTS_OmniTraceSearchLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="SOTS|OmniTrace", meta=(WorldContext="WorldContextObject"))
    static FSOTS_SearchPatternResult SOTS_RequestSearchPattern(
        UObject* WorldContextObject,
        FVector Origin,
        float Radius,
        int32 NumPoints,
        FGameplayTag PatternTag);
};

