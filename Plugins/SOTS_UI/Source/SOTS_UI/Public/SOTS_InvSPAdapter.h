#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SOTS_InvSPAdapter.generated.h"

class AActor;

/**
 * Adapter seam for InvSP UI integration.
 * Implement in Blueprint to forward these calls to InvSP (e.g., BP_InventoryComponent).
 * No direct InvSP dependencies should be added here.
 */
UCLASS(Blueprintable, BlueprintType)
class SOTS_UI_API USOTS_InvSPAdapter : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void OpenInventory();

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void CloseInventory();

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void RefreshInventoryView();

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void ToggleInventory();

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void SetShortcutMenuVisible(bool bVisible);

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void NotifyPickupItem(FGameplayTag ItemTag, int32 Quantity);

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void NotifyFirstTimePickup(FGameplayTag ItemTag);

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void OpenContainer(AActor* ContainerActor);

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void CloseContainer();
};
