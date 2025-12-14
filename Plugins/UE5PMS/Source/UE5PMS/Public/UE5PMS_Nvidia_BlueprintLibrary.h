#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UE5PMS_NvidiaTypes.h"
#include "UE5PMS_Nvidia_BlueprintLibrary.generated.h"

UCLASS()
class UE5PMS_API UUE5PMS_Nvidia_BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Fetch current NVIDIA GPU metrics via NVAPI.
	 *
	 * This node makes NO decisions – it simply calls into the NVAPI backend
	 * and returns whatever it gets.
	 *
	 * Your Blueprint overlay can:
	 * - Check bOutIsValid / Stats.bIsValid
	 * - Check ErrorMessage if invalid
	 * - Decide to fall back to ADLX / RHI / etc.
	 */

	UFUNCTION(
		BlueprintPure,
		Category = "UE5PMS|NVIDIA", 
		meta = (
		DisplayName = "UE5-PMS Get Nvidia Stats",
		CompactNodeTitle = "PMS_NvD")
	)
	static FUE5PMS_NvGpuMetrics GetNvidiaStats(bool& bIsValid, FString& ErrorMessage);
	
};
