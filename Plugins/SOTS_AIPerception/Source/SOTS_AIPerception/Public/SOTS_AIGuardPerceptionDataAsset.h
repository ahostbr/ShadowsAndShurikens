#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_AIPerceptionTypes.h"
#include "SOTS_AIGuardPerceptionDataAsset.generated.h"

/**
 * Per-guard perception tuning asset. Controls how fast suspicion
 * rises/falls and which tags are applied when states change.
 */
UCLASS(BlueprintType)
class SOTS_AIPERCEPTION_API USOTS_AIGuardPerceptionDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI")
    FSOTS_AIGuardPerceptionConfig Config;
};

