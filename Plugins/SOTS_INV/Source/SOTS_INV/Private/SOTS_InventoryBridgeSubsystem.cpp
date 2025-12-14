#include "SOTS_InventoryBridgeSubsystem.h"

#include "Interfaces/SOTS_InventoryProviderInterface.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

USOTS_InventoryBridgeSubsystem* USOTS_InventoryBridgeSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (UWorld* World = WorldContextObject->GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<USOTS_InventoryBridgeSubsystem>();
        }
    }

    return nullptr;
}

UObject* USOTS_InventoryBridgeSubsystem::ResolveInventoryProvider(const AActor* Owner) const
{
    if (!Owner)
    {
        return nullptr;
    }

    if (Owner->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
    {
        return const_cast<AActor*>(Owner);
    }

    TInlineComponentArray<UActorComponent*> Components;
    Owner->GetComponents(Components);
    for (UActorComponent* Component : Components)
    {
        if (Component && Component->GetClass()->ImplementsInterface(USOTS_InventoryProviderInterface::StaticClass()))
        {
            return Component;
        }
    }

    return nullptr;
}

int32 USOTS_InventoryBridgeSubsystem::CountItemsOnInvComponent(UInvSP_InventoryComponent* InvComp, FGameplayTag ItemTag) const
{
    if (!InvComp || !ItemTag.IsValid())
    {
        return 0;
    }

    const FName TagName = ItemTag.GetTagName();
    int32 TotalCount = 0;

    for (const FSOTS_SerializedItem& Item : InvComp->GetCarriedItems())
    {
        if (Item.ItemId == TagName)
        {
            TotalCount += Item.Quantity;
        }
    }

    return TotalCount;
}

bool USOTS_InventoryBridgeSubsystem::ConsumeItemsOnInvComponent(UInvSP_InventoryComponent* InvComp, FGameplayTag ItemTag, int32 Count)
{
    if (!InvComp || !ItemTag.IsValid() || Count <= 0)
    {
        return false;
    }

    TArray<FSOTS_SerializedItem> Items = InvComp->GetCarriedItems();
    const FName TagName = ItemTag.GetTagName();

    int32 Remaining = Count;
    for (FSOTS_SerializedItem& Item : Items)
    {
        if (Remaining <= 0)
        {
            break;
        }

        if (Item.ItemId != TagName || Item.Quantity <= 0)
        {
            continue;
        }

        const int32 RemoveAmount = FMath::Min(Item.Quantity, Remaining);
        Item.Quantity -= RemoveAmount;
        Remaining -= RemoveAmount;
    }

    if (Remaining > 0)
    {
        return false;
    }

    Items.RemoveAll([](const FSOTS_SerializedItem& Item)
    {
        return Item.ItemId.IsNone() || Item.Quantity <= 0;
    });

    InvComp->ClearCarriedItems();
    for (const FSOTS_SerializedItem& Item : Items)
    {
        InvComp->AddCarriedItem(Item.ItemId, Item.Quantity);
    }

    return true;
}

UActorComponent* USOTS_InventoryBridgeSubsystem::FindPlayerCarriedInventoryComponent() const
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                return Pawn->FindComponentByClass<UInvSP_InventoryComponent>();
            }
        }
    }
    return nullptr;
}

UActorComponent* USOTS_InventoryBridgeSubsystem::FindPlayerStashInventoryComponent() const
{
    return FindPlayerCarriedInventoryComponent();
}

void USOTS_InventoryBridgeSubsystem::BuildProfileData(FSOTS_InventoryProfileData& OutData) const
{
    OutData.CarriedItems.Reset();
    OutData.StashItems.Reset();
    OutData.QuickSlots.Reset();

    if (UActorComponent* CarriedComp = FindPlayerCarriedInventoryComponent())
    {
        ExtractItemsFromInventoryComponent(CarriedComp, OutData.CarriedItems);
    }

    if (UActorComponent* StashComp = FindPlayerStashInventoryComponent())
    {
        ExtractItemsFromInventoryComponent(StashComp, OutData.StashItems, true);
    }

    ExtractQuickSlots(OutData.QuickSlots);
}

void USOTS_InventoryBridgeSubsystem::ApplyProfileData(const FSOTS_InventoryProfileData& InData)
{
    if (UActorComponent* CarriedComp = FindPlayerCarriedInventoryComponent())
    {
        ClearInventoryComponent(CarriedComp);
        ApplyItemsToInventoryComponent(CarriedComp, InData.CarriedItems);
    }

    if (UActorComponent* StashComp = FindPlayerStashInventoryComponent())
    {
        ClearInventoryComponent(StashComp, true);
        ApplyItemsToInventoryComponent(StashComp, InData.StashItems, true);
    }

    ApplyQuickSlots(InData.QuickSlots);

    CachedCarriedItems = InData.CarriedItems;
    CachedStashItems = InData.StashItems;
    CachedQuickSlots = InData.QuickSlots;
}

void USOTS_InventoryBridgeSubsystem::ClearAllInventory()
{
    if (UActorComponent* CarriedComp = FindPlayerCarriedInventoryComponent())
    {
        ClearInventoryComponent(CarriedComp);
        CachedCarriedItems.Reset();
    }

    if (UActorComponent* StashComp = FindPlayerStashInventoryComponent())
    {
        ClearInventoryComponent(StashComp, true);
        CachedStashItems.Reset();
    }

    ApplyQuickSlots({});
    CachedQuickSlots.Reset();
}

void USOTS_InventoryBridgeSubsystem::AddCarriedItemById(FName ItemId, int32 Quantity)
{
    if (ItemId.IsNone() || Quantity <= 0)
    {
        return;
    }

    if (UActorComponent* CarriedComp = FindPlayerCarriedInventoryComponent())
    {
        TArray<FSOTS_SerializedItem> Temp;
        Temp.Add({ ItemId, Quantity });
        ApplyItemsToInventoryComponent(CarriedComp, Temp);
    }

    CachedCarriedItems.Add({ ItemId, Quantity });
}

void USOTS_InventoryBridgeSubsystem::AddStashItemById(FName ItemId, int32 Quantity)
{
    if (ItemId.IsNone() || Quantity <= 0)
    {
        return;
    }

    if (UActorComponent* StashComp = FindPlayerStashInventoryComponent())
    {
        TArray<FSOTS_SerializedItem> Temp;
        Temp.Add({ ItemId, Quantity });
        ApplyItemsToInventoryComponent(StashComp, Temp, true);
    }

    CachedStashItems.Add({ ItemId, Quantity });
}

void USOTS_InventoryBridgeSubsystem::SetQuickSlotItem(int32 SlotIndex, FName ItemId)
{
    if (SlotIndex < 0)
    {
        return;
    }

    TArray<FSOTS_ItemSlotBinding> Bindings;
    ExtractQuickSlots(Bindings);

    bool bFound = false;
    for (FSOTS_ItemSlotBinding& Binding : Bindings)
    {
        if (Binding.SlotIndex == SlotIndex)
        {
            Binding.ItemId = ItemId;
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        Bindings.Add({ SlotIndex, ItemId });
    }

    ApplyQuickSlots(Bindings);
    CachedQuickSlots = Bindings;
}

void USOTS_InventoryBridgeSubsystem::ClearQuickSlots()
{
    ApplyQuickSlots({});
    CachedQuickSlots.Reset();
}

bool USOTS_InventoryBridgeSubsystem::HasItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count) const
{
    if (Count <= 0)
    {
        return true;
    }

    if (!ItemTag.IsValid())
    {
        return false;
    }

    if (UObject* Provider = ResolveInventoryProvider(Owner))
    {
        if (ISOTS_InventoryProviderInterface::Execute_HasItemByTag(Provider, ItemTag, Count))
        {
            return true;
        }

        const int32 ProviderCount = ISOTS_InventoryProviderInterface::Execute_GetItemCountByTag(Provider, ItemTag);
        return ProviderCount >= Count;
    }

    if (Owner)
    {
        if (UInvSP_InventoryComponent* InvComp = Owner->FindComponentByClass<UInvSP_InventoryComponent>())
        {
            return CountItemsOnInvComponent(InvComp, ItemTag) >= Count;
        }
    }

    return false;
}

int32 USOTS_InventoryBridgeSubsystem::GetItemCountByTag(AActor* Owner, FGameplayTag ItemTag) const
{
    if (!ItemTag.IsValid())
    {
        return 0;
    }

    if (UObject* Provider = ResolveInventoryProvider(Owner))
    {
        return ISOTS_InventoryProviderInterface::Execute_GetItemCountByTag(Provider, ItemTag);
    }

    if (Owner)
    {
        if (UInvSP_InventoryComponent* InvComp = Owner->FindComponentByClass<UInvSP_InventoryComponent>())
        {
            return CountItemsOnInvComponent(InvComp, ItemTag);
        }
    }

    return 0;
}

bool USOTS_InventoryBridgeSubsystem::TryConsumeItemByTag(AActor* Owner, FGameplayTag ItemTag, int32 Count)
{
    if (Count <= 0)
    {
        return false;
    }

    if (!ItemTag.IsValid())
    {
        return false;
    }

    if (UObject* Provider = ResolveInventoryProvider(Owner))
    {
        return ISOTS_InventoryProviderInterface::Execute_TryConsumeItemByTag(Provider, ItemTag, Count);
    }

    if (Owner)
    {
        if (UInvSP_InventoryComponent* InvComp = Owner->FindComponentByClass<UInvSP_InventoryComponent>())
        {
            return ConsumeItemsOnInvComponent(InvComp, ItemTag, Count);
        }
    }

    return false;
}

bool USOTS_InventoryBridgeSubsystem::GetEquippedItemTag(AActor* Owner, FGameplayTag& OutItemTag) const
{
    if (UObject* Provider = ResolveInventoryProvider(Owner))
    {
        return ISOTS_InventoryProviderInterface::Execute_GetEquippedItemTag(Provider, OutItemTag);
    }

    return false;
}

void USOTS_InventoryBridgeSubsystem::ExtractItemsFromInventoryComponent(UActorComponent* Comp, TArray<FSOTS_SerializedItem>& OutItems, bool bStash) const
{
    OutItems.Reset();

    if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
    {
        if (bStash)
        {
            OutItems.Append(InvComp->GetStashItems());
        }
        else
        {
            OutItems.Append(InvComp->GetCarriedItems());
        }
    }
}

void USOTS_InventoryBridgeSubsystem::ClearInventoryComponent(UActorComponent* Comp, bool bStash) const
{
    if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
    {
        if (bStash)
        {
            InvComp->ClearStashItems();
        }
        else
        {
            InvComp->ClearCarriedItems();
        }
    }
}

void USOTS_InventoryBridgeSubsystem::ApplyItemsToInventoryComponent(UActorComponent* Comp, const TArray<FSOTS_SerializedItem>& Items, bool bStash) const
{
    if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
    {
        if (bStash)
        {
            InvComp->ClearStashItems();
        }
        else
        {
            InvComp->ClearCarriedItems();
        }

        for (const FSOTS_SerializedItem& Item : Items)
        {
            if (Item.ItemId.IsNone() || Item.Quantity <= 0)
            {
                continue;
            }

            if (bStash)
            {
                InvComp->AddStashItem(Item.ItemId, Item.Quantity);
            }
            else
            {
                InvComp->AddCarriedItem(Item.ItemId, Item.Quantity);
            }
        }
    }
}

void USOTS_InventoryBridgeSubsystem::ExtractQuickSlots(TArray<FSOTS_ItemSlotBinding>& OutBindings) const
{
    OutBindings.Reset();

    if (UActorComponent* Comp = FindPlayerCarriedInventoryComponent())
    {
        if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
        {
            OutBindings.Append(InvComp->GetQuickSlots());
        }
    }
}

int32 USOTS_InventoryBridgeSubsystem::GetItemCountByTags(const FGameplayTagContainer& Tags, bool bStash) const
{
    if (Tags.IsEmpty())
    {
        return 0;
    }

    TArray<FSOTS_SerializedItem> Items;
    if (UActorComponent* Comp = bStash ? FindPlayerStashInventoryComponent() : FindPlayerCarriedInventoryComponent())
    {
        ExtractItemsFromInventoryComponent(Comp, Items, bStash);
    }
    else
    {
        Items = bStash ? CachedStashItems : CachedCarriedItems;
    }

    int32 TotalCount = 0;
    for (const FGameplayTag& Tag : Tags)
    {
        if (!Tag.IsValid())
        {
            continue;
        }

        const FName TagName = Tag.GetTagName();

        for (const FSOTS_SerializedItem& Item : Items)
        {
            if (Item.ItemId == TagName)
            {
                TotalCount += Item.Quantity;
            }
        }
    }

    return TotalCount;
}

void USOTS_InventoryBridgeSubsystem::ApplyQuickSlots(const TArray<FSOTS_ItemSlotBinding>& InBindings) const
{
    if (UActorComponent* Comp = FindPlayerCarriedInventoryComponent())
    {
        if (UInvSP_InventoryComponent* InvComp = Cast<UInvSP_InventoryComponent>(Comp))
        {
            InvComp->ClearQuickSlots();
            for (const FSOTS_ItemSlotBinding& Binding : InBindings)
            {
                InvComp->SetQuickSlot(Binding.SlotIndex, Binding.ItemId);
            }
        }
    }
}

int32 USOTS_InventoryBridgeSubsystem::GetCarriedItemCountByTags(const FGameplayTagContainer& Tags) const
{
    return GetItemCountByTags(Tags, false);
}

bool USOTS_InventoryBridgeSubsystem::ConsumeCarriedItemsByTags(const FGameplayTagContainer& Tags, int32 Count)
{
    return ConsumeItemsByTagsInternal(Tags, Count, false);
}

bool USOTS_InventoryBridgeSubsystem::ConsumeItemsByTagsInternal(const FGameplayTagContainer& Tags, int32 Count, bool bStash)
{
    if (Count <= 0 || Tags.IsEmpty())
    {
        return false;
    }

    const int32 Available = GetItemCountByTags(Tags, bStash);
    if (Available < Count)
    {
        return false;
    }

    TArray<FSOTS_SerializedItem> Items;
    if (UActorComponent* Comp = bStash ? FindPlayerStashInventoryComponent() : FindPlayerCarriedInventoryComponent())
    {
        ExtractItemsFromInventoryComponent(Comp, Items, bStash);
    }
    else
    {
        Items = bStash ? CachedStashItems : CachedCarriedItems;
    }

    int32 Remaining = Count;
    for (const FGameplayTag& Tag : Tags)
    {
        if (Remaining <= 0)
        {
            break;
        }

        if (!Tag.IsValid())
        {
            continue;
        }

        const FName TagName = Tag.GetTagName();
        for (FSOTS_SerializedItem& Item : Items)
        {
            if (Item.ItemId != TagName || Item.Quantity <= 0)
            {
                continue;
            }

            const int32 RemoveAmount = FMath::Min(Item.Quantity, Remaining);
            Item.Quantity -= RemoveAmount;
            Remaining -= RemoveAmount;

            if (Remaining <= 0)
            {
                break;
            }
        }
    }

    Items.RemoveAll([](const FSOTS_SerializedItem& Item)
    {
        return Item.ItemId.IsNone() || Item.Quantity <= 0;
    });

    if (UActorComponent* Comp = bStash ? FindPlayerStashInventoryComponent() : FindPlayerCarriedInventoryComponent())
    {
        ApplyItemsToInventoryComponent(Comp, Items, bStash);
    }

    if (bStash)
    {
        CachedStashItems = Items;
    }
    else
    {
        CachedCarriedItems = Items;
    }

    return true;
}
