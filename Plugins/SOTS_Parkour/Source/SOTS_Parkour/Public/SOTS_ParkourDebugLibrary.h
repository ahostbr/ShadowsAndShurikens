#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourDebugLibrary.generated.h"

class UObject;
class USOTS_ParkourActionSet;

/**
 * Debug helpers for SOTS Parkour.
 *
 * This library gives us a central place to:
 *  - Log FSOTS_ParkourResult in a readable way.
 *  - Draw debug capsules/lines/points for FSOTS_ParkourCapsuleTraceSettings.
 *
 * It’s intended purely for parity/QA and can be called from both C++
 * and Blueprint during bring-up. Shipping builds can disable calls to
 * these functions if needed.
 */
UCLASS()
class SOTS_PARKOUR_API USOTS_ParkourDebugLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Draw a debug representation of a capsule trace given its computed
	 * FSOTS_ParkourCapsuleTraceSettings.
	 *
	 * - Draws the start capsule.
	 * - If End != Start, draws the end capsule and a line between them.
	 * - If bHit && Hit.IsValidBlockingHit(), draws the impact point.
	 */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug", meta = (WorldContext = "WorldContextObject"))
	static void DrawParkourCapsuleTrace(
		UObject* WorldContextObject,
		const FSOTS_ParkourCapsuleTraceSettings& Settings,
		bool bHit,
		const FHitResult& Hit,
		float Duration = 1.0f
	);

	/**
	 * Log a concise summary of a parkour result to the output log.
	 *
	 * Useful when comparing CGF BP behavior to the C++ path:
	 *  - Action / ResultType
	 *  - ClimbStyle
	 *  - HeightDelta
	 *  - WorldLocation / TargetLocation
	 */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug", meta = (WorldContext = "WorldContextObject"))
	static void LogParkourResult(
		UObject* WorldContextObject,
		const FSOTS_ParkourResult& Result,
		const FString& ContextLabel
	);

	/**
	 * Import parkour action definitions from a JSON file into the given ActionSet.
	 *
	 * Intended for editor-time use only. FilePath can be absolute or
	 * relative to ProjectContentDir (e.g. "SOTS/Config/UEFNParkourActionVariables.json").
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "SOTS|Parkour|Config")
	static bool ImportParkourActionsFromJson(USOTS_ParkourActionSet* TargetAsset, const FString& FilePath);
};
