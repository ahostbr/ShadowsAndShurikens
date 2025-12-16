// Ninja Bear Studio Inc., all rights reserved.
#include "UI/ViewModels/ViewModel_KeyMappingEntry.h"

#include "CommonInputBaseTypes.h"
#include "CommonInputTypeEnum.h"
#include "NinjaInputFunctionLibrary.h"
#include "Components/NinjaInputManagerComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Input/Settings/NinjaInputPlayerMappableKeySettings.h"

void UViewModel_KeyMappingEntry::ShutdownViewModel_Implementation()
{
	PlayerController = nullptr;
}

void UViewModel_KeyMappingEntry::SetPlayerController(APlayerController* OwningPlayer)
{
	PlayerController = OwningPlayer;
}

void UViewModel_KeyMappingEntry::SetMapping(const FPlayerKeyMapping& Mapping, const UNinjaInputPlayerMappableKeySettings* Settings)
{
	if (Mapping.IsValid())
	{
		SetMappingName(Mapping.GetMappingName());
		SetDisplayName(Mapping.GetDisplayName());
		SetDisplayCategory(Mapping.GetDisplayCategory());
		SetCurrentKey(Mapping.GetCurrentKey());
		SetDefaultKey(Mapping.GetDefaultKey());
		SetIsCustomized(Mapping.IsCustomized());

		if (IsValid(Settings))
		{
			const FText& NewTooltip = Settings->GetTooltipText(); 
			SetTooltip(NewTooltip);
		}

		UpdateIconBrushFromCurrentKey();
	}
}

void UViewModel_KeyMappingEntry::UpdateIconBrushFromCurrentKey()
{
	if (CurrentKey.IsValid())
	{
		ECommonInputType Dummy;
		FName OutDefaultGamepadName;
		FCommonInputBase::GetCurrentPlatformDefaults(Dummy, OutDefaultGamepadName);

		ECommonInputType KeyInputType = ECommonInputType::MouseAndKeyboard;
		if (CurrentKey.IsGamepadKey())
		{
			KeyInputType = ECommonInputType::Gamepad;
		}
		else if (CurrentKey.IsTouch())
		{
			KeyInputType = ECommonInputType::Touch;
		}

		FSlateBrush InputBrush;
		if (UCommonInputPlatformSettings::Get()->TryGetInputBrush(InputBrush, TArray<FKey> { CurrentKey }, KeyInputType, OutDefaultGamepadName))
		{
			return SetIconBrush(InputBrush);
		}
	}
}

void UViewModel_KeyMappingEntry::SetMappingName(const FName& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(MappingName, NewValue);
}

void UViewModel_KeyMappingEntry::SetDisplayName(const FText& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, NewValue);	
}

void UViewModel_KeyMappingEntry::SetDisplayCategory(const FText& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(DisplayCategory, NewValue);	
}

void UViewModel_KeyMappingEntry::SetTooltip(const FText& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(Tooltip, NewValue);
}

void UViewModel_KeyMappingEntry::SetCurrentKey(const FKey& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentKey, NewValue);
}

void UViewModel_KeyMappingEntry::SetDefaultKey(const FKey& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(DefaultKey, NewValue);	
}

void UViewModel_KeyMappingEntry::SetIconBrush(const FSlateBrush& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(IconBrush, NewValue);
}

void UViewModel_KeyMappingEntry::SetIsCustomized(const bool bNewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsCustomized, bNewValue);
}

void UViewModel_KeyMappingEntry::RebindKey(const FMapPlayerKeyArgs& NewKeyArgs) const
{
	UNinjaInputManagerComponent* InputManager = UNinjaInputFunctionLibrary::GetInputManagerComponent(PlayerController);
	if (IsValid(InputManager))
	{
		FGameplayTagContainer FailureReason;
		InputManager->RemapKey(NewKeyArgs, FailureReason);
	}
}

void UViewModel_KeyMappingEntry::RebindKeyToFirstSlot(const FKey& Key) const
{
	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingName;
	Args.NewKey = Key;
	Args.Slot = EPlayerMappableKeySlot::First;
	RebindKey(Args);
}

void UViewModel_KeyMappingEntry::ResetKeyBinding() const
{
	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingName;
	Args.NewKey = DefaultKey;
	Args.Slot = EPlayerMappableKeySlot::First;
	RebindKey(Args);
}
