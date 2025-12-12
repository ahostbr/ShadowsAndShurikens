#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InvSPInventoryComponent.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_InventoryBridgeSubsystem.generated.h"

UCLASS()
class SOTS_INV_API USOTS_InventoryBridgeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static USOTS_InventoryBridgeSubsystem* Get(const UObject* WorldContextObject);

	void BuildProfileData(FSOTS_InventoryProfileData& OutData) const;
	void ApplyProfileData(const FSOTS_InventoryProfileData& InData);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Inventory")
	void ClearAllInventory();

	UFUNCTION(BlueprintCallable, Category = "SOTS|Inventory")
	void AddCarriedItemById(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Inventory")
	void AddStashItemById(FName ItemId, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Inventory")
	void SetQuickSlotItem(int32 SlotIndex, FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Inventory")
	void ClearQuickSlots();

	int32 GetCarriedItemCountByTags(const FGameplayTagContainer& Tags) const;
	bool ConsumeCarriedItemsByTags(const FGameplayTagContainer& Tags, int32 Count);

protected:
	UPROPERTY()
	TArray<FSOTS_SerializedItem> CachedCarriedItems;

	UPROPERTY()
	TArray<FSOTS_SerializedItem> CachedStashItems;

	UPROPERTY()
	TArray<FSOTS_ItemSlotBinding> CachedQuickSlots;

	UActorComponent* FindPlayerCarriedInventoryComponent() const;
	UActorComponent* FindPlayerStashInventoryComponent() const;

	void ExtractItemsFromInventoryComponent(UActorComponent* Comp, TArray<FSOTS_SerializedItem>& OutItems, bool bStash = false) const;
	void ClearInventoryComponent(UActorComponent* Comp, bool bStash = false) const;
    void ApplyItemsToInventoryComponent(UActorComponent* Comp, const TArray<FSOTS_SerializedItem>& Items, bool bStash = false) const;
    void ExtractQuickSlots(TArray<FSOTS_ItemSlotBinding>& OutBindings) const;
    void ApplyQuickSlots(const TArray<FSOTS_ItemSlotBinding>& InBindings) const;

    int32 GetItemCountByTags(const FGameplayTagContainer& Tags, bool bStash = false) const;
	bool ConsumeItemsByTagsInternal(const FGameplayTagContainer& Tags, int32 Count, bool bStash = false);
};
