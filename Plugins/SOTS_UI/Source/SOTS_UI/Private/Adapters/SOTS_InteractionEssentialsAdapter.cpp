#include "Adapters/SOTS_InteractionEssentialsAdapter.h"

#include "GameplayTagContainer.h"
#include "SOTS_UIRouterSubsystem.h"
#include "StructUtils/InstancedStruct.h"

namespace
{
	static const FName InteractionViewportName(TEXT("SAS.UI.HUD.InteractionViewport"));
}

void USOTS_InteractionEssentialsAdapter::EnsureViewportCreated_Implementation()
{
	RequestViewportWidget();
}

void USOTS_InteractionEssentialsAdapter::ShowPrompt_Implementation(const F_SOTS_InteractionPromptView& View)
{
	// Blueprint should translate View into the InteractionEssentials viewport.
	RequestViewportWidget();
}

void USOTS_InteractionEssentialsAdapter::UpdatePrompt_Implementation(const F_SOTS_InteractionPromptView& View)
{
	RequestViewportWidget();
}

void USOTS_InteractionEssentialsAdapter::HidePrompt_Implementation()
{
}

void USOTS_InteractionEssentialsAdapter::AddOrUpdateMarker_Implementation(const F_SOTS_InteractionMarkerView& Marker)
{
	RequestViewportWidget();
}

void USOTS_InteractionEssentialsAdapter::RemoveMarker_Implementation(const FGuid& MarkerId)
{
}

void USOTS_InteractionEssentialsAdapter::ClearAllMarkers_Implementation()
{
}

bool USOTS_InteractionEssentialsAdapter::RequestViewportWidget()
{
	USOTS_UIRouterSubsystem* Router = USOTS_UIRouterSubsystem::Get(this);
	if (!Router)
	{
		return false;
	}

	const FGameplayTag WidgetId = GetViewportWidgetId();
	if (!WidgetId.IsValid())
	{
		return false;
	}

	return Router->PushWidgetById(WidgetId, FInstancedStruct());
}

FGameplayTag USOTS_InteractionEssentialsAdapter::GetViewportWidgetId()
{
	return FGameplayTag::RequestGameplayTag(InteractionViewportName, false);
}
