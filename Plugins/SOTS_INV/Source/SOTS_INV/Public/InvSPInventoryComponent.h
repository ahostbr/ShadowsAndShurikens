#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_ProfileTypes.h"
#include "InvSPInventoryComponent.generated.h"

UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_INV_API UInvSP_InventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    const TArray<FSOTS_SerializedItem>& GetCarriedItems() const { return CarriedItems; }
    const TArray<FSOTS_SerializedItem>& GetStashItems() const { return StashItems; }
    const TArray<FSOTS_ItemSlotBinding>& GetQuickSlots() const { return QuickSlots; }

    void ClearCarriedItems();
    void ClearStashItems();
    void ClearQuickSlots();

    void AddCarriedItem(FName ItemId, int32 Quantity);
    void AddStashItem(FName ItemId, int32 Quantity);
    void SetQuickSlot(int32 SlotIndex, FName ItemId);

private:
    UPROPERTY()
    TArray<FSOTS_SerializedItem> CarriedItems;

    UPROPERTY()
    TArray<FSOTS_SerializedItem> StashItems;

    UPROPERTY()
    TArray<FSOTS_ItemSlotBinding> QuickSlots;
};
