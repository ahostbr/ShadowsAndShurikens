// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "NinjaInputModifier.generated.h"

class UNinjaInputUserSettings;

/**
 * Base modifier class that supports the custom input settings.
 */
UCLASS(Abstract, NotBlueprintable)
class NINJAINPUT_API UNinjaInputModifier : public UInputModifier
{
	
	GENERATED_BODY()

protected:

	/**
	 * Provides the custom user settings class. It will retrieve and cache the class if needed.
	 */
	const UNinjaInputUserSettings* GetOrCacheSettings(const UEnhancedPlayerInput* PlayerInput);

private:

	/** Settings providing relevant data. */
	UPROPERTY()
	TObjectPtr<const UNinjaInputUserSettings> Settings;
	
};
