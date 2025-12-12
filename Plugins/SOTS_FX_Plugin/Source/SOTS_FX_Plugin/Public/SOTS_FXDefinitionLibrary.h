#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTSFXTypes.h"
#include "SOTS_FXDefinitionLibrary.generated.h"

/**
 * Simple library of tag-driven FX definitions.
 * The global FX manager searches these libraries to resolve
 * incoming FX tags into concrete Niagara / Sound assets.
 */
UCLASS(BlueprintType)
class SOTS_FX_PLUGIN_API USOTS_FXDefinitionLibrary : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|FX")
    TArray<FSOTS_FXDefinition> Definitions;
};

