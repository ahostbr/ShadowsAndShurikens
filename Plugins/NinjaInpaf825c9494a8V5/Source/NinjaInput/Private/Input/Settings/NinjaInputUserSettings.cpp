// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Settings/NinjaInputUserSettings.h"

void UNinjaInputUserSettings::AsyncSaveSettings()
{
	Super::AsyncSaveSettings();
	OnSaveSettingsStarted.Broadcast();
}

void UNinjaInputUserSettings::OnAsyncSaveComplete(const FString& SlotName, const int32 UserIndex, const bool bSuccess)
{
	Super::OnAsyncSaveComplete(SlotName, UserIndex, bSuccess);

	if (bSuccess)
	{
		OnSaveSettingsCompleted.Broadcast();
	}
	else
	{
		OnSaveSettingsFailed.Broadcast();
	}
}

void UNinjaInputUserSettings::SetInvertVerticalAxis(const bool bNewValue)
{
	bInvertVerticalAxis = bNewValue;
	OnSettingsChanged.Broadcast(this);
}

void UNinjaInputUserSettings::SetInvertHorizontalAxis(const bool bNewValue)
{
	bInvertHorizontalAxis = bNewValue;
	OnSettingsChanged.Broadcast(this);	
}

void UNinjaInputUserSettings::SetMouseSensitivityX(const float NewValue)
{
	MouseSensitivityX = FMath::Clamp(NewValue, 0.f, 10.f);
	OnSettingsChanged.Broadcast(this);
}

void UNinjaInputUserSettings::SetMouseSensitivityY(const float NewValue)
{
	MouseSensitivityY = FMath::Clamp(NewValue, 0.f, 10.f);
	OnSettingsChanged.Broadcast(this);
}

void UNinjaInputUserSettings::SetGamepadSensitivity(const float NewValue)
{
	GamepadSensitivity = FMath::Clamp(NewValue, 0.f, 10.f);
	OnSettingsChanged.Broadcast(this);
}
