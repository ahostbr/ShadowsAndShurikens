#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "SOTS_InputLayerRegistrySettings.generated.h"

class USOTS_InputLayerDataAsset;

USTRUCT(BlueprintType)
struct FSOTS_InputLayerRegistryEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Layer")
    FGameplayTag LayerTag;

    UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Layer")
    TSoftObjectPtr<USOTS_InputLayerDataAsset> LayerAsset;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SOTS Input Layer Registry"))
class SOTS_INPUT_API USOTS_InputLayerRegistrySettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USOTS_InputLayerRegistrySettings();

    UPROPERTY(EditAnywhere, Config, Category = "Layers")
    TArray<FSOTS_InputLayerRegistryEntry> Layers;

    UPROPERTY(EditAnywhere, Config, Category = "Layers")
    bool bLogRegistryLoads;

    UPROPERTY(EditAnywhere, Config, Category = "Layers")
    bool bUseAsyncLoads;
};
