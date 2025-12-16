// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Modifiers/InputModifier_MouseSensitivity.h"

#include "Input/Settings/NinjaInputUserSettings.h"

FInputActionValue UInputModifier_MouseSensitivity::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, const FInputActionValue CurrentValue, const float DeltaTime)
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

	const float ScaleX = UserSettings->GetMouseSensitivityX();
	const float ScaleY = UserSettings->GetMouseSensitivityY();
	
	switch (Type)
	{
		case EInputActionValueType::Axis1D:
		{
			float Value = CurrentValue.Get<float>();
			Value *= ScaleX;
			return FInputActionValue(Value);
		}
		case EInputActionValueType::Axis2D:
		{
			FVector2D Value = CurrentValue.Get<FVector2D>();
			Value.X *= ScaleX;
			Value.Y *= ScaleY;
			return FInputActionValue(Value);
		}
		case EInputActionValueType::Axis3D:
		{
			FVector Value = CurrentValue.Get<FVector>();
			Value.X *= ScaleX;
			Value.Y *= ScaleY;
			return FInputActionValue(Value);
		}
		default: return CurrentValue;
	}
}
