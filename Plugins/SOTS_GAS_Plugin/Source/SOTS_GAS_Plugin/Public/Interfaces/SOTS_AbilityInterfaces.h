#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "SOTS_AbilityInterfaces.generated.h"

UINTERFACE(BlueprintType)
class SOTS_GAS_PLUGIN_API UBPI_SOTS_InventoryAccess : public UInterface
{
    GENERATED_BODY()
};

class SOTS_GAS_PLUGIN_API IBPI_SOTS_InventoryAccess
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS Inventory")
    int32 GetInventoryItemCountByTags(const FGameplayTagContainer& ItemTags) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS Inventory")
    bool ConsumeInventoryItemsByTags(const FGameplayTagContainer& ItemTags, int32 Count);
};

UINTERFACE(BlueprintType)
class SOTS_GAS_PLUGIN_API UBPI_SOTS_SkillTreeAccess : public UInterface
{
    GENERATED_BODY()
};

class SOTS_GAS_PLUGIN_API IBPI_SOTS_SkillTreeAccess
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS SkillTree")
    bool IsSkillUnlocked(FGameplayTag SkillTag) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS SkillTree")
    int32 GetSkillLevel(FGameplayTag SkillTag) const;
};
