// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Modifiers/NinjaInputModifier.h"

#include "EnhancedPlayerInput.h"
#include "NinjaInputFunctionLibrary.h"
#include "Components/NinjaInputManagerComponent.h"
#include "GameFramework/PlayerController.h"
#include "Input/Settings/NinjaInputUserSettings.h"
#include "UObject/UObjectIterator.h"

const UNinjaInputUserSettings* UNinjaInputModifier::GetOrCacheSettings(const UEnhancedPlayerInput* PlayerInput)
{
	if (!IsValid(Settings))
	{
		const APlayerController* PlayerController = Cast<APlayerController>(PlayerInput->GetOuter());
		if (IsValid(PlayerController))
		{
			const UNinjaInputManagerComponent* InputManager = UNinjaInputFunctionLibrary::GetInputManagerComponent(PlayerController);
			if (IsValid(InputManager))
			{
				Settings = InputManager->GetInputUserSettings();
			}
		}
	}

	return Settings;
}
