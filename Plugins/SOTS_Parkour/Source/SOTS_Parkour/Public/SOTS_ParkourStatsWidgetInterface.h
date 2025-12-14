#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "SOTS_ParkourStatsWidgetInterface.generated.h"

UINTERFACE(BlueprintType)
class SOTS_PARKOUR_API USOTS_ParkourStatsWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Minimal interface for debug widgets that display parkour state/action/style.
 * Mirrors the CGF ParkourStatsWidget interface (Set Parkour State/Action/Climb Style).
 */
class SOTS_PARKOUR_API ISOTS_ParkourStatsWidgetInterface
{
	GENERATED_BODY()

public:
	/** Update the displayed parkour state tag (e.g., Parkour.State.Mantle). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|Parkour|UI")
	void SetParkourStateTag(FGameplayTag StateTag);

	/** Update the displayed parkour action tag (e.g., Parkour.Action.Mantle). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|Parkour|UI")
	void SetParkourActionTag(FGameplayTag ActionTag);

	/** Update the displayed climb style tag (e.g., Parkour.ClimbStyle.Braced). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SOTS|Parkour|UI")
	void SetParkourClimbStyleTag(FGameplayTag ClimbStyleTag);
};
