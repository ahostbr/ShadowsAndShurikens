// Ninja Bear Studio Inc., all rights reserved.
#include "UI/ViewModels/ViewModel_KeyMappingCategory.h"

#include "NinjaInputTags.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Input/Settings/NinjaInputPlayerMappableKeySettings.h"
#include "UI/ViewModels/ViewModel_KeyMappingEntry.h"
#include "UObject/UObjectIterator.h"

void UViewModel_KeyMappingCategory::ShutdownViewModel_Implementation()
{
	for (UViewModel_KeyMappingEntry* Binding : Bindings)
	{
		Binding->ShutdownViewModel();
	}

	Bindings.Empty();
	KeyboardAndMouseBindings.Empty();
	GamepadBindings.Empty();	
	OriginalMappings.Empty();
	PlayerController = nullptr;
}

void UViewModel_KeyMappingCategory::SetPlayerController(APlayerController* OwningPlayer)
{
	PlayerController = OwningPlayer;
}

void UViewModel_KeyMappingCategory::SetDisplayCategory(const FText& NewValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(DisplayCategory, NewValue);
}

void UViewModel_KeyMappingCategory::SetMappings(const TArray<FPlayerKeyMapping>& Mappings)
{
	OriginalMappings = Mappings;

	KeyboardAndMouseBindings.Empty();
	GamepadBindings.Empty();
	
	Bindings.Empty();
	Bindings.Reserve(OriginalMappings.Num());

	TMap<FName, const UNinjaInputPlayerMappableKeySettings*> SettingsMap;
	for (TObjectIterator<UNinjaInputPlayerMappableKeySettings> Itr; Itr; ++Itr)
	{
		if (IsValid(*Itr))
		{
			SettingsMap.Add(Itr->GetMappingName(), *Itr);
		}
	}
	
	for (const FPlayerKeyMapping& Mapping : Mappings)
	{
		FName MappingName = Mapping.GetMappingName();
		if (SettingsMap.Contains(MappingName))
		{
			const UNinjaInputPlayerMappableKeySettings* Settings = SettingsMap[MappingName];
			UViewModel_KeyMappingEntry* Entry = NewObject<UViewModel_KeyMappingEntry>(GetOuter());
			Entry->SetPlayerController(PlayerController);
			Entry->SetMapping(Mapping, Settings);
			Bindings.Add(Entry);

			if (Settings->GetInputFilter().MatchesTagExact(Tag_Input_Mode_KeyboardAndMouse))
			{
				KeyboardAndMouseBindings.Add(Entry);
			}
			else if (Settings->GetInputFilter().MatchesTagExact(Tag_Input_Mode_Gamepad))
			{
				GamepadBindings.Add(Entry);
			}
		}
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetBindings);
}

TArray<UViewModel_KeyMappingEntry*> UViewModel_KeyMappingCategory::GetBindings() const
{
	return Bindings;
}

TArray<UViewModel_KeyMappingEntry*> UViewModel_KeyMappingCategory::GetKeyboardAndMouseBindings() const
{
	return KeyboardAndMouseBindings;
}

TArray<UViewModel_KeyMappingEntry*> UViewModel_KeyMappingCategory::GetGamepadBindings() const
{
	return GamepadBindings;
}