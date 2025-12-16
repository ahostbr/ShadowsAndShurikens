// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Modifiers/InputModifier_GamepadSensitivity.h"

#include "Input/Settings/NinjaInputUserSettings.h"

FInputActionValue UInputModifier_GamepadSensitivity::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, const FInputActionValue CurrentValue, const float DeltaTime)
{
	const EInputActionValueType Type = CurrentValue.GetValueType();
	if (Type == EInputActionValueType::Boolean)
	{
		return CurrentValue;
	}
	
	const UNinjaInputUserSettings* UserSettings = GetOrCacheSettings(PlayerInput);
	if (!IsValid(UserSettings))
	{
		return CurrentValue;
	}

	const float Scale = UserSettings->GetGamepadSensitivity();
	return CurrentValue * Scale;
}
