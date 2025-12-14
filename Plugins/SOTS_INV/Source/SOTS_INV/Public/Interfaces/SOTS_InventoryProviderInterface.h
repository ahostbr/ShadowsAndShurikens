#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "SOTS_InventoryProviderInterface.generated.h"

UINTERFACE(BlueprintType)
class SOTS_INV_API USOTS_InventoryProviderInterface : public UInterface
{
    GENERATED_BODY()
};

class SOTS_INV_API ISOTS_InventoryProviderInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    int32 GetItemCountByTag(FGameplayTag ItemTag) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    bool TryConsumeItemByTag(FGameplayTag ItemTag, int32 Count);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    bool HasItemByTag(FGameplayTag ItemTag, int32 Count) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    bool GetEquippedItemTag(FGameplayTag& OutItemTag) const;
};
