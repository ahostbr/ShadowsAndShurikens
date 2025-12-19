#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_InventoryTypes.h"
#include "SOTS_InventoryFacadeLibrary.generated.h"

class USOTS_InventoryBridgeSubsystem;
class AActor;

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

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|UI", meta=(WorldContext="WorldContextObject"))
    static void RequestOpenInventoryUI(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|UI", meta=(WorldContext="WorldContextObject"))
    static void RequestCloseInventoryUI(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|UI", meta=(WorldContext="WorldContextObject"))
    static void RequestToggleInventoryUI(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|UI", meta=(WorldContext="WorldContextObject"))
    static void RequestRefreshInventoryUI(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|UI", meta=(WorldContext="WorldContextObject"))
    static void RequestSetShortcutMenuVisible(const UObject* WorldContextObject, bool bVisible);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|Identity")
    static FName ItemIdFromTag(FGameplayTag ItemTag);

    UFUNCTION(BlueprintPure, Category="SOTS|Inventory|Provider", meta=(WorldContext="WorldContextObject"))
    static UObject* GetInventoryProviderFromWorldContext(const UObject* WorldContextObject, AActor* Instigator);

    UFUNCTION(BlueprintPure, Category="SOTS|Inventory|Provider")
    static UObject* ResolveInventoryProviderFromControllerFirst(AActor* Instigator);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|Provider", meta=(WorldContext="WorldContextObject", DisplayName="Request Pickup (From Request)"))
    static bool RequestPickup_FromRequest(const UObject* WorldContextObject,
                                          const FSOTS_InvPickupRequest& Request,
                                          FSOTS_InvPickupResult& OutResult);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|Provider", meta=(WorldContext="WorldContextObject"))
    static bool RequestPickupSimple(const UObject* WorldContextObject,
                                    AActor* Instigator,
                                    AActor* PickupActor,
                                    FGameplayTag ItemTag,
                                    int32 Quantity,
                                    bool& bSuccess);

    UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|Provider", meta=(WorldContext="WorldContextObject"))
    static bool RequestPickup(const UObject* WorldContextObject,
                              AActor* Instigator,
                              AActor* PickupActor,
                              FGameplayTag ItemTag,
                              int32 Quantity,
                              bool& bSuccess);
};
