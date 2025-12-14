#pragma once

#include "CoreMinimal.h"
#include "UE5PMS_NvidiaTypes.generated.h"

USTRUCT(BlueprintType)
struct UE5PMS_API FUE5PMS_NvGpuMetrics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bIsValid = false;

	UPROPERTY(BlueprintReadOnly)
	FString ErrorMessage;

	UPROPERTY(BlueprintReadOnly)
	FString GpuName;

	UPROPERTY(BlueprintReadOnly)
	int32 VramTotalMB = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 VramUsedMB = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 GpuUsagePercent = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 MemoryControllerUsagePercent = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 CoreClockMHz = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 MemoryClockMHz = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 TemperatureC = -1;
};
