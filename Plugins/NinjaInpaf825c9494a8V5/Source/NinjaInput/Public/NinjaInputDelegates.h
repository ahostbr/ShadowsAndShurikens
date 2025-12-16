// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "NinjaInputDelegates.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInputSaveSettingsEventSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInputModeChangedSignature, FGameplayTag, InputMode);

UCLASS(Abstract, Const, Hidden, NotBlueprintable)
class NINJAINPUT_API UNinjaInputDelegates : public UObject
{
	GENERATED_BODY()
};
