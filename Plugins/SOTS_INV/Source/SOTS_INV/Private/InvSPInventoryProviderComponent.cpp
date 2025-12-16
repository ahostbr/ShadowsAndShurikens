#include "InvSPInventoryProviderComponent.h"

UInvSPInventoryProviderComponent::UInvSPInventoryProviderComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInvSPInventoryProviderComponent::BeginPlay()
{
    Super::BeginPlay();
    CachedInvComponent = ResolveInvComponent();
}

bool UInvSPInventoryProviderComponent::IsInventoryReady() const
{
    return ResolveInvComponent() != nullptr;
}

UObject* UInvSPInventoryProviderComponent::GetProviderObject() const
{
    return ResolveInvComponent();
}

bool UInvSPInventoryProviderComponent::HasItemByTag(const FGameplayTag& ItemTag, int32 MinQty, int32& OutQuantity) const
{
    OutQuantity = 0;
    if (!ItemTag.IsValid() || MinQty <= 0)
    {
        return false;
    }

    OutQuantity = CountItems(ItemTag);
    return OutQuantity >= MinQty;
}

bool UInvSPInventoryProviderComponent::TryConsumeItemByTag(const FGameplayTag& ItemTag, int32 Quantity, int32& OutConsumed)
{
    OutConsumed = 0;
    if (!ItemTag.IsValid() || Quantity <= 0)
    {
        return false;
    }

    return ConsumeItems(ItemTag, Quantity, OutConsumed);
}

bool UInvSPInventoryProviderComponent::AddItemByTag(const FGameplayTag& ItemTag, int32 Quantity)
{
    if (!ItemTag.IsValid() || Quantity <= 0)
    {
        return false;
    }

    if (UInvSP_InventoryComponent* InvComp = ResolveInvComponent())
    {
        InvComp->AddCarriedItem(ItemTag.GetTagName(), Quantity);
        return true;
    }
    return false;
}

bool UInvSPInventoryProviderComponent::RemoveItemByTag(const FGameplayTag& ItemTag, int32 Quantity)
{
    int32 Consumed = 0;
    return TryConsumeItemByTag(ItemTag, Quantity, Consumed);
}

bool UInvSPInventoryProviderComponent::GetEquippedItemTag(FGameplayTag& OutItemTag) const
{
    OutItemTag = FGameplayTag();
    // InvSP reference does not expose an equipped slot accessor in current bridge; return false.
    return false;
}

UInvSP_InventoryComponent* UInvSPInventoryProviderComponent::ResolveInvComponent() const
{
    if (CachedInvComponent.IsValid())
    {
        return CachedInvComponent.Get();
    }

    if (AActor* Owner = GetOwner())
    {
        return Owner->FindComponentByClass<UInvSP_InventoryComponent>();
    }

    return nullptr;
}

int32 UInvSPInventoryProviderComponent::CountItems(const FGameplayTag& ItemTag) const
{
    int32 TotalCount = 0;
    if (UInvSP_InventoryComponent* InvComp = ResolveInvComponent())
    {
        const FName TagName = ItemTag.GetTagName();
        for (const FSOTS_SerializedItem& Item : InvComp->GetCarriedItems())
        {
            if (Item.ItemId == TagName)
            {
                TotalCount += Item.Quantity;
            }
        }
    }
    return TotalCount;
}

bool UInvSPInventoryProviderComponent::ConsumeItems(const FGameplayTag& ItemTag, int32 Quantity, int32& OutConsumed)
{
    OutConsumed = 0;
    UInvSP_InventoryComponent* InvComp = ResolveInvComponent();
    if (!InvComp)
    {
        return false;
    }

    TArray<FSOTS_SerializedItem> Items = InvComp->GetCarriedItems();
    const FName TagName = ItemTag.GetTagName();
    int32 Remaining = Quantity;

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
        OutConsumed += RemoveAmount;
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
