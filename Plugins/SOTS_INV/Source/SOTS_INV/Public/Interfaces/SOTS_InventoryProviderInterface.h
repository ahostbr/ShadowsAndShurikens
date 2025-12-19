#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "Internationalization/Text.h"
#include "SOTS_InventoryProviderInterface.generated.h"

class AActor;

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
    bool CanPickup(AActor* Instigator, AActor* PickupActor, FGameplayTag ItemTag, int32 Quantity, FText& OutFailReason) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    bool ExecutePickup(AActor* Instigator, AActor* PickupActor, FGameplayTag ItemTag, int32 Quantity);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    bool AddItemByTag(AActor* Instigator, FGameplayTag ItemTag, int32 Quantity);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    bool RemoveItemByTag(AActor* Instigator, FGameplayTag ItemTag, int32 Quantity);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    int32 GetItemCount(AActor* Instigator, FGameplayTag ItemTag) const;

    // Legacy/provider bridge helpers (Execute_ wrappers expected by bridge).
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    int32 GetItemCountByTag(FGameplayTag ItemTag) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    bool HasItemByTag(FGameplayTag ItemTag, int32 Count) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    bool TryConsumeItemByTag(FGameplayTag ItemTag, int32 Count);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Inventory")
    bool GetEquippedItemTag(UPARAM(ref) FGameplayTag& OutEquippedItemTag) const;
};
