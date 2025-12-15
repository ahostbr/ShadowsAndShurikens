#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "SOTS_InventoryProvider.generated.h"

UINTERFACE(MinimalAPI)
class USOTS_InventoryProvider : public UInterface
{
    GENERATED_BODY()
};

class ISOTS_InventoryProvider
{
    GENERATED_BODY()

public:
    /** Returns true when underlying inventory systems are ready to answer queries. */
    virtual bool IsInventoryReady() const = 0;

    /** Optional: direct provider object (component/actor) for diagnostics. */
    virtual UObject* GetProviderObject() const = 0;

    /** Query by tag; OutQuantity returns available count (0 if missing). */
    virtual bool HasItemByTag(const FGameplayTag& ItemTag, int32 MinQty, int32& OutQuantity) const = 0;

    /** Attempt to consume items; OutConsumed contains how many were removed. */
    virtual bool TryConsumeItemByTag(const FGameplayTag& ItemTag, int32 Quantity, int32& OutConsumed) = 0;

    /** Add items by tag. */
    virtual bool AddItemByTag(const FGameplayTag& ItemTag, int32 Quantity) = 0;

    /** Remove items by tag (non-failing if missing). */
    virtual bool RemoveItemByTag(const FGameplayTag& ItemTag, int32 Quantity) = 0;

    /** Optional slot query; returns true if equipped slot tag is available. */
    virtual bool GetEquippedItemTag(FGameplayTag& OutItemTag) const = 0;

    /** Optional UI hooks. */
    virtual bool OpenInventoryUI() { return false; }
    virtual bool CloseInventoryUI() { return false; }
    virtual bool ToggleInventoryUI() { return false; }
};
