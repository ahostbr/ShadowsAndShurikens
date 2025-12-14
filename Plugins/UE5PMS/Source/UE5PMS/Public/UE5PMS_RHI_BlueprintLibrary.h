#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UE5PMS_RHI_Types.h"
#include "UE5PMS_RHI_BlueprintLibrary.generated.h"

/**
 * Blueprint access for engine/RHI timing.
 * This node does NOT decide anything; it only returns current timing metrics.
 */
UCLASS()
class UE5PMS_API UUE5PMS_RHI_BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Fetch current GPU metrics via Unreal RHI (vendor-agnostic).
	 *
	 * This node makes NO decisions – it simply calls into the RHI backend
	 * and returns whatever it gets.
	 *
	 * Your Blueprint overlay can:
	 * - Check bOutIsValid / Stats.bIsValid
	 * - Check ErrorMessage if invalid
	 * - Use this as a safe fallback when vendor APIs are unavailable.
	 */



	UFUNCTION(
		BlueprintPure,
		Category = "UE5PMS|RHI",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "UE5-PMS Get RHI Stats",
			CompactNodeTitle = "PMS_RHI"
			)
	)










	static FUE5PMS_RhiMetrics GetRHIStats(const UObject* WorldContextObject, bool& bIsValid, FString& ErrorMessage);
};
