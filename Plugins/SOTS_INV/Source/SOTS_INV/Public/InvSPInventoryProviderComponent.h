#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/SOTS_InventoryProvider.h"
#include "InvSPInventoryComponent.h"
#include "InvSPInventoryProviderComponent.generated.h"

/**
 * Adapter that wraps InvSP inventory components behind the ISOTS_InventoryProvider seam.
 * Place this on the player/pawn blueprint to expose inventory to the SOTS bridge.
 */
UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_INV_API UInvSPInventoryProviderComponent : public UActorComponent, public ISOTS_InventoryProvider
{
    GENERATED_BODY()

public:
    UInvSPInventoryProviderComponent();

    virtual void BeginPlay() override;

    // ISOTS_InventoryProvider
    virtual bool IsInventoryReady() const override;
    virtual UObject* GetProviderObject() const override;
    virtual bool HasItemByTag(const FGameplayTag& ItemTag, int32 MinQty, int32& OutQuantity) const override;
    virtual bool TryConsumeItemByTag(const FGameplayTag& ItemTag, int32 Quantity, int32& OutConsumed) override;
    virtual bool AddItemByTag(const FGameplayTag& ItemTag, int32 Quantity) override;
    virtual bool RemoveItemByTag(const FGameplayTag& ItemTag, int32 Quantity) override;
    virtual bool GetEquippedItemTag(FGameplayTag& OutItemTag) const override;

private:
    TWeakObjectPtr<UInvSP_InventoryComponent> CachedInvComponent;

    UInvSP_InventoryComponent* ResolveInvComponent() const;
    int32 CountItems(const FGameplayTag& ItemTag) const;
    bool ConsumeItems(const FGameplayTag& ItemTag, int32 Quantity, int32& OutConsumed);
};
