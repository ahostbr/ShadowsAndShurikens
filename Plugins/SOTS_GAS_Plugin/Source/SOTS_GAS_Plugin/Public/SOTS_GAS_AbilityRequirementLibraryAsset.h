#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/SOTS_AbilityTypes.h"
#include "SOTS_GAS_AbilityRequirementLibraryAsset.generated.h"

/**
 * Optional library asset that maps ability tags to authored requirement
 * descriptions. Abilities can look up their tag here and then evaluate
 * the resulting FSOTS_AbilityRequirements struct.
 */
UCLASS(BlueprintType)
class SOTS_GAS_PLUGIN_API USOTS_AbilityRequirementLibraryAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    TArray<FSOTS_AbilityRequirements> AbilityRequirements;
};

