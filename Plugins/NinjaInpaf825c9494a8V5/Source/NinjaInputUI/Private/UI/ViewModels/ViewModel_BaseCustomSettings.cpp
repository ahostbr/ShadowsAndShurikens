// Ninja Bear Studio Inc., all rights reserved.
#include "UI/ViewModels/ViewModel_BaseCustomSettings.h"

#include "NinjaInputFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/NinjaInputManagerComponent.h"
#include "Input/Settings/NinjaInputUserSettings.h"

void UViewModel_BaseCustomSettings::InitializeViewModel_Implementation(const UUserWidget* UserWidget)
{
	PlayerController = UserWidget->GetOwningPlayer();
}

void UViewModel_BaseCustomSettings::ShutdownViewModel_Implementation()
{
	ResetUserSettings();
	PlayerController = nullptr;
}

void UViewModel_BaseCustomSettings::SaveInputSettings()
{
	if (bIsSavingSettings)
	{
		return;
	}
	
	const UNinjaInputManagerComponent* InputManager = UNinjaInputFunctionLibrary::GetInputManagerComponent(PlayerController);
	if (IsValid(InputManager))
	{
		UserSettings = InputManager->GetInputUserSettings();
		if (IsValid(UserSettings))
		{
			UserSettings->OnSaveSettingsStarted.AddUniqueDynamic(this, &ThisClass::HandleSaveSettingsStarted);
			UserSettings->OnSaveSettingsCompleted.AddUniqueDynamic(this, &ThisClass::OnSaveSettingsFinished);
			UserSettings->OnSaveSettingsFailed.AddUniqueDynamic(this, &ThisClass::OnSaveSettingsFinished);
		}
		
		FGameplayTagContainer FailureReason;
		InputManager->SaveInputSettings();
	}
}

void UViewModel_BaseCustomSettings::HandleSaveSettingsStarted()
{
	SetIsSavingSettings(true);
}

void UViewModel_BaseCustomSettings::OnSaveSettingsFinished()
{
	SetIsSavingSettings(true);
	ResetUserSettings();
}

void UViewModel_BaseCustomSettings::ResetUserSettings()
{
	if (IsValid(UserSettings))
	{
		UserSettings->OnSaveSettingsStarted.RemoveAll(this);
		UserSettings->OnSaveSettingsCompleted.RemoveAll(this);
		UserSettings->OnSaveSettingsFailed.RemoveAll(this);
		UserSettings = nullptr;
	}
}

void UViewModel_BaseCustomSettings::SetIsSavingSettings(const bool bNewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsSavingSettings, bNewValue);
}

UNinjaInputUserSettings* UViewModel_BaseCustomSettings::GetUserSettings() const
{
	const UNinjaInputManagerComponent* InputManager = UNinjaInputFunctionLibrary::GetInputManagerComponent(PlayerController);
	if (IsValid(InputManager))
	{
		return InputManager->GetInputUserSettings();
	}
	return nullptr;
}
