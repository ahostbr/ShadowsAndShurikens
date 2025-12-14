#pragma once

#include "CoreMinimal.h"
#include "UE5PMS_RHI_Types.generated.h"

/**
 * Generic engine-side timing metrics based on RHI / UWorld.
 * No vendor-specific data here – just "how fast is the game running?"
 */
USTRUCT(BlueprintType)
struct UE5PMS_API FUE5PMS_RhiMetrics
{
	GENERATED_BODY()

	/** True if we had a valid world and could calculate timing. */
	UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
	bool bIsValid = false;

	/** If something went wrong (no world, etc), this will contain a description. */
	UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
	FString ErrorMessage;

	/** Current frame’s instantaneous FPS (1 / DeltaSeconds). */
	UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
	float CurrentFPS = -1.0f;

	/** Current frame time in milliseconds (DeltaSeconds * 1000). */
	UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
	float FrameTimeMS = -1.0f;

	/** Optional smoothed FPS (can be filled later if you want to add smoothing). */
	UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
	float SmoothedFPS = -1.0f;

	/** Optional smoothed frame time in ms. */
	UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
	float SmoothedFrameTimeMS = -1.0f;
};
