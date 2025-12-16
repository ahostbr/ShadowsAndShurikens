// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputModifier.h"
#include "InputModifier_GamepadSensitivity.generated.h"

/**
 * Applies the gamepad sensitivity configured in the custom settings.
 */
UCLASS(NotBlueprintable, MinimalAPI, meta = (DisplayName = "Gamepad Sensitivity"))
class UInputModifier_GamepadSensitivity : public UNinjaInputModifier
{
	
	GENERATED_BODY()

protected:

	// -- Begin InputModifier implementation
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;
	// -- End InputModifier implementation
	
};
