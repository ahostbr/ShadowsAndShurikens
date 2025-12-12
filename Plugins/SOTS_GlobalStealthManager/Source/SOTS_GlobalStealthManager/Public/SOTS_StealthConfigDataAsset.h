#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_StealthConfigDataAsset.generated.h"

/**
 * DataAsset that holds all tunable knobs for the global stealth scoring model.
 * This allows designers to tweak stealth behavior without touching code.
 */
UCLASS(BlueprintType)
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_StealthConfigDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stealth")
    FSOTS_StealthScoringConfig Config;
};

