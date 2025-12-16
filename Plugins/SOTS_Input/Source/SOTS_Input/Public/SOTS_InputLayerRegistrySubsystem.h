#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_InputLayerRegistrySubsystem.generated.h"

class USOTS_InputLayerDataAsset;
class FStreamableHandle;

UCLASS()
class SOTS_INPUT_API USOTS_InputLayerRegistrySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    bool TryGetLayerAsset(FGameplayTag Tag, USOTS_InputLayerDataAsset*& OutLayer) const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Input")
    void RequestLayerAsync(FGameplayTag Tag);

    bool IsAsyncLoadsEnabled() const { return bUseAsyncLoads; }

private:
    UPROPERTY()
    TMap<FGameplayTag, TSoftObjectPtr<USOTS_InputLayerDataAsset>> LayerMap;

    UPROPERTY()
    TMap<FGameplayTag, TObjectPtr<USOTS_InputLayerDataAsset>> LoadedLayers;

    TMap<FGameplayTag, TSharedPtr<FStreamableHandle>> PendingLoads;

    bool bUseAsyncLoads = false;

    void OnLayerLoadedInternal(FGameplayTag Tag, TSoftObjectPtr<USOTS_InputLayerDataAsset> SoftPtr);
};
