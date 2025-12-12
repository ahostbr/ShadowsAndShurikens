#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SOTS_ProHUDAdapter.generated.h"

UCLASS(Blueprintable, BlueprintType)
class SOTS_UI_API USOTS_ProHUDAdapter : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|ProHUD")
	void EnsureHUDCreated();

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|ProHUD")
	void PushNotification(const FString& Message, float DurationSeconds, FGameplayTag CategoryTag);

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|ProHUD")
	void AddOrUpdateWorldMarker(const FGuid& Id, AActor* Target, const FVector& Location, bool bIsActor, FGameplayTag CategoryTag, bool bClampToEdges);

	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|UI|ProHUD")
	void RemoveWorldMarker(const FGuid& Id);
};
