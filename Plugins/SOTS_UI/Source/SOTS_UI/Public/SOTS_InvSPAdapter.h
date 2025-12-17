#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SOTS_InvSPAdapter.generated.h"

class AActor;

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
	void OpenContainer(AActor* ContainerActor);

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|InvSP")
	void CloseContainer();
};
