#include "InvSPInventoryComponent.h"

#include "Math/UnrealMathUtility.h"

void UInvSP_InventoryComponent::ClearCarriedItems()
{
    CarriedItems.Reset();
}

void UInvSP_InventoryComponent::ClearStashItems()
{
    StashItems.Reset();
}

void UInvSP_InventoryComponent::ClearQuickSlots()
{
    QuickSlots.Reset();
}

void UInvSP_InventoryComponent::AddCarriedItem(FName ItemId, int32 Quantity)
{
    if (ItemId.IsNone() || Quantity <= 0)
    {
        return;
    }

    for (FSOTS_SerializedItem& Entry : CarriedItems)
    {
        if (Entry.ItemId == ItemId)
        {
            Entry.Quantity += Quantity;
            return;
        }
    }

    CarriedItems.Add({ ItemId, Quantity });
}

void UInvSP_InventoryComponent::AddStashItem(FName ItemId, int32 Quantity)
{
    if (ItemId.IsNone() || Quantity <= 0)
    {
        return;
    }

    for (FSOTS_SerializedItem& Entry : StashItems)
    {
        if (Entry.ItemId == ItemId)
        {
            Entry.Quantity += Quantity;
            return;
        }
    }

    StashItems.Add({ ItemId, Quantity });
}

void UInvSP_InventoryComponent::SetQuickSlot(int32 SlotIndex, FName ItemId)
{
    if (SlotIndex < 0)
    {
        return;
    }

    for (FSOTS_ItemSlotBinding& Slot : QuickSlots)
    {
        if (Slot.SlotIndex == SlotIndex)
        {
            Slot.ItemId = ItemId;
            return;
        }
    }

    QuickSlots.Add({ SlotIndex, ItemId });
}
