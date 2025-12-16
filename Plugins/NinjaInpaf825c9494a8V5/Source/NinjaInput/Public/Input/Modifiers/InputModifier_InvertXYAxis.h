// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "NinjaInputModifier.h"
#include "InputModifier_InvertXYAxis.generated.h"

/**
 * Inverts XY axis, based on the settings provided by the custom Ninja Input class.
 * Each axis is inverted separately, based on the current values in the input settings.
 */
UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "Invert XY Axis"))
class UInputModifier_InvertXYAxis : public UNinjaInputModifier
{
	
	GENERATED_BODY()

protected:

	// -- Begin InputModifier implementation
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;
	// -- End InputModifier implementation
	
};
