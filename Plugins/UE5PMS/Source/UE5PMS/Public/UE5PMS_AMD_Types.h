#pragma once

#include "CoreMinimal.h"
#include "UE5PMS_Types.h"
#include "UE5PMS_AMD_Types.generated.h"

// ---------------------------------------------------------
// AMD (ADLX) stats exposed to Blueprints
// ---------------------------------------------------------
USTRUCT(BlueprintType)
struct FPMS_AMDStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|AMD")
    bool bIsValid = false;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|AMD")
    EPMS_StatusCode Status = EPMS_StatusCode::NotInitialized;

    // Optional textual error, empty when Status == OK
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|AMD")
    FString ErrorMessage;

    // Overall GPU usage percentage
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|AMD")
    int32 GpuUsagePercent = -1;

    // Memory usage in MB
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|AMD")
    int32 VramUsedMB = -1;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|AMD")
    int32 VramTotalMB = -1;

    // Core temperature in °C
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|AMD")
    int32 TemperatureC = -1;
};
