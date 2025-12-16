// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "InputTriggers.h"
#include "InputTriggerDoubleTap.generated.h"

/**
 * Double Tap: Press/Release/Press.
 * 
 * Input must be actuated then released within TapReleaseTimeThreshold seconds to trigger.
 * Then a second input must be actuated within the TapIntervalThreshold.
 */
UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "Double Tap"))
class UInputTriggerDoubleTap final : public UInputTrigger
{
	
	GENERATED_BODY()

public:
	
	/** Release within this time-frame to trigger a tap. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "Trigger Settings", meta = (ClampMin = "0"))
	float TapReleaseTimeThreshold;

	/** Time limit between taps. */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "Trigger Settings", meta = (ClampMin = "0"))
	float TapIntervalThreshold;

	/** Should global time dilation be applied to the held duration? */
	UPROPERTY(EditAnywhere, Config, BlueprintReadWrite, Category = "Trigger Settings")
	bool bAffectedByTimeDilation = false;
	
	UInputTriggerDoubleTap(const FObjectInitializer& ObjectInitializer);

	// -- Begin Input Trigger implementation
	virtual ETriggerEventsSupported GetSupportedTriggerEvents() const override;
	virtual FString GetDebugState() const override;
	// -- End Input Trigger implementation
	
protected:

	/** How long have we been actuating this trigger? */
	UPROPERTY(BlueprintReadWrite, Category = "Trigger Settings")
	float HeldDuration;
	
	// -- Begin Input Trigger implementation
	virtual ETriggerState UpdateState_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue ModifiedValue, float DeltaTime) override;
	// -- End Input Trigger implementation

	float CalculateHeldDuration(const UEnhancedPlayerInput* const PlayerInput, const float DeltaTime) const;
	
private:

	uint8 Taps = 0;
	float LastTapTime = 0.f;
	
};
