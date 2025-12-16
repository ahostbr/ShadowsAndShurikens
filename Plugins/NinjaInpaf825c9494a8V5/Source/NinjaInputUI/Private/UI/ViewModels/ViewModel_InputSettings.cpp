// Ninja Bear Studio Inc., all rights reserved.
#include "UI/ViewModels/ViewModel_InputSettings.h"

#include "Input/Settings/NinjaInputUserSettings.h"

void UViewModel_InputSettings::InitializeViewModel_Implementation(const UUserWidget* UserWidget)
{
	Super::InitializeViewModel_Implementation(UserWidget);

	const UNinjaInputUserSettings* Settings = GetUserSettings();
	if (IsValid(Settings))
	{
		bInvertVerticalAxis = Settings->ShouldInvertVerticalAxis();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bInvertVerticalAxis);

		bInvertHorizontalAxis = Settings->ShouldInvertHorizontalAxis();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bInvertHorizontalAxis);

		MouseSensitivityX = Settings->GetMouseSensitivityX();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MouseSensitivityX);

		MouseSensitivityY = Settings->GetMouseSensitivityY();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MouseSensitivityY);

		GamepadSensitivity = Settings->GetGamepadSensitivity();
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GamepadSensitivity);
	}
}

void UViewModel_InputSettings::SetInvertVerticalAxis(const bool bNewValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bInvertVerticalAxis, bNewValue))
	{
		UNinjaInputUserSettings* Settings = GetUserSettings();
		if (IsValid(Settings))
		{
			Settings->SetInvertVerticalAxis(bNewValue);
		}	
	}
}

void UViewModel_InputSettings::SetInvertHorizontalAxis(const bool bNewValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(bInvertHorizontalAxis, bNewValue))
	{
		UNinjaInputUserSettings* Settings = GetUserSettings();
		if (IsValid(Settings))
		{
			Settings->SetInvertHorizontalAxis(bNewValue);
		}
	}
}

void UViewModel_InputSettings::SetMouseSensitivityX(const float NewValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MouseSensitivityX, NewValue))
	{
		UNinjaInputUserSettings* Settings = GetUserSettings();
		if (IsValid(Settings))
		{
			Settings->SetMouseSensitivityX(NewValue);
		}
	}
}

void UViewModel_InputSettings::SetMouseSensitivityY(const float NewValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MouseSensitivityY, NewValue))
	{
		UNinjaInputUserSettings* Settings = GetUserSettings();
		if (IsValid(Settings))
		{
			Settings->SetMouseSensitivityY(NewValue);
		}
	}
}

void UViewModel_InputSettings::SetGamepadSensitivity(const float NewValue)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(GamepadSensitivity, NewValue))
	{
		UNinjaInputUserSettings* Settings = GetUserSettings();
		if (IsValid(Settings))
		{
			Settings->SetGamepadSensitivity(NewValue);
		}
	}
}
