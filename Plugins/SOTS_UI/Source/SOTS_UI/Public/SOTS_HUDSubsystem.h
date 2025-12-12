#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_HUDSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnHudFloatChanged, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnHudTextChanged, const FString&, NewText);

UCLASS()
class SOTS_UI_API USOTS_HUDSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static USOTS_HUDSubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SOTS|HUD")
	void SetHealthPercent(float InPercent);

	UFUNCTION(BlueprintPure, Category = "SOTS|HUD")
	float GetHealthPercent() const { return HealthPercent; }

	UFUNCTION(BlueprintCallable, Category = "SOTS|HUD")
	void SetDetectionLevel(float InLevel);

	UFUNCTION(BlueprintPure, Category = "SOTS|HUD")
	float GetDetectionLevel() const { return DetectionLevel; }

	UFUNCTION(BlueprintCallable, Category = "SOTS|HUD")
	void SetObjectiveText(const FString& InText);

	UFUNCTION(BlueprintPure, Category = "SOTS|HUD")
	const FString& GetObjectiveText() const { return ObjectiveText; }

	UPROPERTY(BlueprintAssignable, Category = "SOTS|HUD")
	FSOTS_OnHudFloatChanged OnHealthPercentChanged;

	UPROPERTY(BlueprintAssignable, Category = "SOTS|HUD")
	FSOTS_OnHudFloatChanged OnDetectionLevelChanged;

	UPROPERTY(BlueprintAssignable, Category = "SOTS|HUD")
	FSOTS_OnHudTextChanged OnObjectiveTextChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|HUD", meta = (AllowPrivateAccess = "true"))
	float HealthPercent = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|HUD", meta = (AllowPrivateAccess = "true"))
	float DetectionLevel = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|HUD", meta = (AllowPrivateAccess = "true"))
	FString ObjectiveText;
};
