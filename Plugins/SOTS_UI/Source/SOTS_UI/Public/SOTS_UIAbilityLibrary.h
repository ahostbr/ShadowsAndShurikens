#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_UIAbilityLibrary.generated.h"

class USOTS_HUDSubsystem;
class USOTS_NotificationSubsystem;

UCLASS()
class SOTS_UI_API USOTS_UIAbilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Notify the HUD/notification systems about an ability event. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Ability", meta = (WorldContext = "WorldContextObject"))
	static void NotifyAbilityEvent(const UObject* WorldContextObject,
	                               FGameplayTag AbilityTag,
	                               bool bSuccess,
	                               const FText& DetailText);
};
