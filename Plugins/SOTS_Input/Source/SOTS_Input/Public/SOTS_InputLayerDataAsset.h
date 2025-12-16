#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SOTS_InputLayerDataAsset.generated.h"

class UInputMappingContext;
class USOTS_InputHandler;

UENUM(BlueprintType)
enum class ESOTS_InputLayerConsumePolicy : uint8
{
    None,
    ConsumeHandled,
    ConsumeAllMatches
};

// STABLE API: used by SOTS_UI and gameplay systems
UCLASS(BlueprintType)
class SOTS_INPUT_API USOTS_InputLayerDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
    FGameplayTag LayerTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
    TArray<TObjectPtr<UInputMappingContext>> MappingContexts;

    UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Layer")
    TArray<TObjectPtr<USOTS_InputHandler>> HandlerTemplates;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
    bool bBlocksLowerPriorityLayers = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
    ESOTS_InputLayerConsumePolicy ConsumePolicy = ESOTS_InputLayerConsumePolicy::ConsumeHandled;

    void GetAllBindings(TMap<const UInputAction*, TSet<ETriggerEvent>>& Out) const;
};
