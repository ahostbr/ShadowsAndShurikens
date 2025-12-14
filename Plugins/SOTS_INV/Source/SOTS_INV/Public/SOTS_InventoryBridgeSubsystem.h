#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InvSPInventoryComponent.h"
#include "Interfaces/SOTS_InventoryProviderInterface.h"
#include "GameplayTagContainer.h"
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

	UFUNCTION(BlueprintCallable, Category = "SOTS|Inventory")
	bool HasItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count = 1) const;

	UFUNCTION(BlueprintCallable, Category = "SOTS|Inventory")
	int32 GetItemCountByTag(AActor* Owner, FGameplayTag ItemTag) const;

	UFUNCTION(BlueprintCallable, Category = "SOTS|Inventory")
	bool TryConsumeItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Inventory")
	bool GetEquippedItemTag(AActor* Owner, FGameplayTag& OutItemTag) const;

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

	UObject* ResolveInventoryProvider(const AActor* Owner) const;
	int32 CountItemsOnInvComponent(UInvSP_InventoryComponent* InvComp, FGameplayTag ItemTag) const;
	bool ConsumeItemsOnInvComponent(UInvSP_InventoryComponent* InvComp, FGameplayTag ItemTag, int32 Count);
};
