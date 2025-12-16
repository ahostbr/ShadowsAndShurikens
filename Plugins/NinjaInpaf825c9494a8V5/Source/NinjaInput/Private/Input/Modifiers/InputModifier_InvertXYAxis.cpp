// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Modifiers/InputModifier_InvertXYAxis.h"

#include "EnhancedPlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "Input/Settings/NinjaInputUserSettings.h"

FInputActionValue UInputModifier_InvertXYAxis::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, const FInputActionValue CurrentValue, const float DeltaTime)
{
	const UNinjaInputUserSettings* UserSettings = GetOrCacheSettings(PlayerInput);
	if (!IsValid(UserSettings))
	{
		return CurrentValue;
	}

	FVector NewValue = CurrentValue.Get<FVector>();
	
	if (UserSettings->ShouldInvertHorizontalAxis())
	{
		NewValue.X *= -1.0f;
	}
	if (UserSettings->ShouldInvertVerticalAxis())
	{
		NewValue.Y *= -1.0f;
	}

	return NewValue;
}
