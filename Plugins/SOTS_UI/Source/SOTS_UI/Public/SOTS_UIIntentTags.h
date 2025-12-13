#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

namespace SOTS_UIIntentTags
{
	static FORCEINLINE FGameplayTag GetQuitGameTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Action.QuitGame")), false);
	}

	static FORCEINLINE FGameplayTag GetReturnToMainMenuTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Action.ReturnToMainMenu")), false);
	}

	static FORCEINLINE FGameplayTag GetOpenSettingsTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Action.OpenSettings")), false);
	}

	static FORCEINLINE FGameplayTag GetOpenProfilesTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Action.OpenProfiles")), false);
	}

	static FORCEINLINE FGameplayTag GetCloseTopModalTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Action.CloseTopModal")), false);
	}

	static FORCEINLINE FGameplayTag GetConfirmDialogTag()
	{
		// Default modal dialog widget id. Replace via registry/config as needed.
		return FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Modals.DialogPrompt")), false);
	}

	static FORCEINLINE FGameplayTag GetConfirmDialogFallbackTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.CGF.Modal.DialogPrompt")), false);
	}
}
