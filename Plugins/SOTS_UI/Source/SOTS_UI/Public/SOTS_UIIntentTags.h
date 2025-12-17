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

	static FORCEINLINE FGameplayTag GetInteractionShowTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Intent.Interaction.Show")), false);
	}

	static FORCEINLINE FGameplayTag GetInteractionUpdateTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Intent.Interaction.Update")), false);
	}

	static FORCEINLINE FGameplayTag GetInteractionHideTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Intent.Interaction.Hide")), false);
	}

	static FORCEINLINE FGameplayTag GetInteractionMarkerAddTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Intent.Interaction.Marker.AddOrUpdate")), false);
	}

	static FORCEINLINE FGameplayTag GetInteractionMarkerRemoveTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Intent.Interaction.Marker.Remove")), false);
	}

	static FORCEINLINE FGameplayTag GetLoadingBeginTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Intent.Loading.Begin")), false);
	}

	static FORCEINLINE FGameplayTag GetLoadingEndTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.UI.Intent.Loading.End")), false);
	}
}
