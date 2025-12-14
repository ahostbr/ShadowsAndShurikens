#pragma once

#include "CoreMinimal.h"
#include "SOTS_InteractionViewTypes.h"
#include "UObject/Object.h"
#include "SOTS_InteractionEssentialsAdapter.generated.h"

class USOTS_UIRouterSubsystem;

/**
 * Adapter that bridges SOTS interaction intents into InteractionEssentials widgets.
 * Blueprint implementations are expected to translate the view structs into the concrete widget calls.
 */
UCLASS(Blueprintable, BlueprintType)
class SOTS_UI_API USOTS_InteractionEssentialsAdapter : public UObject
{
	GENERATED_BODY()

public:
	// Ensure the interaction viewport widget is created (via widget registry tag) and ready to receive updates.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|UI|Interaction")
	void EnsureViewportCreated();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|UI|Interaction")
	void ShowPrompt(const F_SOTS_InteractionPromptView& View);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|UI|Interaction")
	void UpdatePrompt(const F_SOTS_InteractionPromptView& View);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|UI|Interaction")
	void HidePrompt();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|UI|Interaction")
	void AddOrUpdateMarker(const F_SOTS_InteractionMarkerView& Marker);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|UI|Interaction")
	void RemoveMarker(const FGuid& MarkerId);

	// Optional helper for full clears.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|UI|Interaction")
	void ClearAllMarkers();

protected:
	// Helper for implementations: push the viewport widget by registry ID. Uses SOTS_UIRouterSubsystem.
	UFUNCTION(BlueprintCallable, Category = "SOTS|UI|Interaction")
	bool RequestViewportWidget();

private:
	// The registry ID for the InteractionEssentials viewport.
	static FGameplayTag GetViewportWidgetId();
};
