#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InvSPInventoryComponent.h"
#include "Interfaces/SOTS_InventoryProviderInterface.h"
#include "Interfaces/SOTS_InventoryProvider.h"
#include "GameplayTagContainer.h"
#include "SOTS_ProfileSnapshotProvider.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_InventoryTypes.h"
#include "SOTS_InventoryBridgeSubsystem.generated.h"

UCLASS(Config=Game)
class SOTS_INV_API USOTS_InventoryBridgeSubsystem : public UGameInstanceSubsystem, public ISOTS_ProfileSnapshotProvider
{
	GENERATED_BODY()

	friend class USOTS_InventoryFacadeLibrary;

public:
	static USOTS_InventoryBridgeSubsystem* Get(const UObject* WorldContextObject);

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

	bool ShouldLogInventoryOpFailures() const { return bDebugLogInventoryOpFailures; }
	UObject* GetResolvedProviderForFacade(const AActor* Owner) const { return GetResolvedProvider(Owner); }

	void BuildProfileData(FSOTS_InventoryProfileData& OutData) const;
	void ApplyProfileData(const FSOTS_InventoryProfileData& InData);
	void ValidateInventorySnapshot(const FSOTS_InventoryProfileData& InData, TArray<FString>& OutWarnings, TArray<FString>& OutErrors) const;
    virtual void BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot) override;
    virtual void ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot) override;

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

	UFUNCTION(BlueprintCallable, Category="SOTS|Inventory")
	FSOTS_InventoryOpReport HasItemByTag_WithReport(AActor* Owner, FGameplayTag ItemTag, int32 Count = 1) const;

	UFUNCTION(BlueprintCallable, Category="SOTS|Inventory")
	FSOTS_InventoryOpReport TryConsumeItemByTag_WithReport(AActor* Owner, FGameplayTag ItemTag, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category="SOTS|Inventory")
	FSOTS_InventoryOpReport AddItemByTag_WithReport(AActor* Owner, FGameplayTag ItemTag, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category="SOTS|Inventory")
	FSOTS_InventoryOpReport RemoveItemByTag_WithReport(AActor* Owner, FGameplayTag ItemTag, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category="SOTS|Inventory")
	FSOTS_InventoryOpReport GetEquippedItemTag_WithReport(AActor* Owner) const;

	UPROPERTY(BlueprintAssignable, Category="SOTS|Inventory")
	FSOTS_OnInventoryOpCompleted OnInventoryOpCompleted;

	UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|Provider")
	bool TryResolveInventoryProviderNow(AActor* OwnerOverride = nullptr);

	UFUNCTION(BlueprintCallable, Category="SOTS|Inventory|Provider")
	bool IsInventoryProviderReady() const { return bProviderResolved && CachedProvider.IsValid(); }

	// Facade/debug helpers
	UObject* GetResolvedProvider_ForUI(const AActor* Owner) const { return GetResolvedProvider(Owner); }

	int32 GetCarriedItemCountByTags(const FGameplayTagContainer& Tags) const;
	bool ConsumeCarriedItemsByTags(const FGameplayTagContainer& Tags, int32 Count);

protected:
	UPROPERTY()
	TArray<FSOTS_SerializedItem> CachedCarriedItems;

	UPROPERTY()
	TArray<FSOTS_SerializedItem> CachedStashItems;

	UPROPERTY()
	TArray<FSOTS_ItemSlotBinding> CachedQuickSlots;

	UPROPERTY(Config)
	bool bDebugLogProviderResolution = false;

	UPROPERTY(Config)
	bool bWarnOnProviderMissing = false;

	UPROPERTY(Config)
	bool bWarnOnProviderNotReady = false;

	UPROPERTY(Config)
	bool bDebugLogInventoryOpFailures = false;

	UPROPERTY(Config)
	bool bValidateInventorySnapshotOnBuild = false;

	UPROPERTY(Config)
	bool bValidateInventorySnapshotOnApply = false;

	UPROPERTY(Config)
	bool bDebugLogSnapshotValidation = false;

	UPROPERTY(Config)
	bool bEnableDeferredProfileApply = false;

	UPROPERTY(Config)
	int32 DeferredApplyMaxRetries = 20;

	UPROPERTY(Config)
	float DeferredApplyRetryIntervalSeconds = 0.5f;

	UPROPERTY()
	TWeakObjectPtr<UObject> CachedProvider;

	bool bProviderResolved = false;
	mutable double NextProviderWarnTime = 0.0;

	// Deferred profile apply state
	bool bHasPendingSnapshot = false;
	FSOTS_InventoryProfileData PendingSnapshot;
	int32 PendingApplyRetries = 0;
	FTimerHandle DeferredApplyTimerHandle;

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
	bool EnsureProviderResolved(const AActor* Owner) const;
	UObject* GetResolvedProvider(const AActor* Owner) const;
	FSOTS_InventoryOpReport MakeBaseReport(AActor* Owner, const FGameplayTag& ItemTag, int32 RequestedQty) const;
	void LogOpFailureIfNeeded(const FSOTS_InventoryOpReport& Report) const;
	FSOTS_InventoryOpReport Op_HasItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count) const;
	FSOTS_InventoryOpReport Op_TryConsumeItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count);
	FSOTS_InventoryOpReport Op_AddItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count);
	FSOTS_InventoryOpReport Op_RemoveItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count);
	FSOTS_InventoryOpReport Op_GetEquippedItem(AActor* Owner) const;
	int32 CountItemsOnInvComponent(UInvSP_InventoryComponent* InvComp, FGameplayTag ItemTag) const;
	bool ConsumeItemsOnInvComponent(UInvSP_InventoryComponent* InvComp, FGameplayTag ItemTag, int32 Count);
	void TryApplyPendingSnapshot();
};
