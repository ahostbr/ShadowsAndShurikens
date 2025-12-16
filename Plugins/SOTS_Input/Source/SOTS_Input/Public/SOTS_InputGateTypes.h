#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_InputGateTypes.generated.h"

USTRUCT(BlueprintType)
struct FSOTS_InputGateRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input")
    FGameplayTag GateTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input")
    bool bRequirePresent = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input")
    bool bAffectsBuffering = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Input")
    bool bAffectsLiveInput = true;
};
