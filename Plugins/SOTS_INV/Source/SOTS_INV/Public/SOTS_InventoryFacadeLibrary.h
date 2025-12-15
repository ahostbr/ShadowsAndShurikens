#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_InventoryTypes.h"
#include "SOTS_InventoryFacadeLibrary.generated.h"

class USOTS_InventoryBridgeSubsystem;

/**
 * Suite-facing helpers for abilities, skill gates, and equipped queries.
 * Wraps the inventory bridge without exposing InvSP internals.
 */
UCLASS()
class SOTS_INV_API USOTS_InventoryFacadeLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|Facade", meta=(WorldContext="WorldContextObject"))
    static bool TryConsumeItemForAbility(const UObject* WorldContextObject,
                                         FGameplayTag ItemTag,
                                         int32 Quantity,
                                         FGameplayTag AbilityTag,
                                         FSOTS_InventoryOpReport& OutReport);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|Facade", meta=(WorldContext="WorldContextObject"))
    static bool HasItemForSkillGate(const UObject* WorldContextObject,
                                    FGameplayTag ItemTag,
                                    int32 MinQuantity,
                                    FGameplayTag SkillTag,
                                    FSOTS_InventoryOpReport& OutReport);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|Facade", meta=(WorldContext="WorldContextObject"))
    static bool GetEquippedItemTag_ForContext(const UObject* WorldContextObject,
                                              FGameplayTag SlotTag,
                                              FGameplayTag& OutEquippedTag,
                                              FSOTS_InventoryOpReport& OutReport);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|UI", meta=(WorldContext="WorldContextObject"))
    static bool OpenInventoryUI_WithReport(const UObject* WorldContextObject, FSOTS_InventoryOpReport& OutReport);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|UI", meta=(WorldContext="WorldContextObject"))
    static bool CloseInventoryUI_WithReport(const UObject* WorldContextObject, FSOTS_InventoryOpReport& OutReport);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|UI", meta=(WorldContext="WorldContextObject"))
    static bool ToggleInventoryUI_WithReport(const UObject* WorldContextObject, FSOTS_InventoryOpReport& OutReport);
};
