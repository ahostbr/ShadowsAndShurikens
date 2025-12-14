#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UE5PMS_AMD_Types.h"
#include "UE5PMS_AMD_BlueprintLibrary.generated.h"
// Source/UE5PMS/Public/UE5PMS_AMD_BlueprintLibrary.h

UCLASS()
class UE5PMS_API UUE5PMS_AMD_BlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Fetch current AMD GPU metrics via ADLX.
     *
     * This node makes NO decisions – it simply calls into the ADLX backend,
     * and returns whatever it gets.
     *
     * Your Blueprint overlay can:
     * - Check bOutIsValid / Stats.bIsValid
     * - Check ErrorMessage if invalid
     * - Decide to fall back to NVAPI / RHI / etc.
     */
   //
   // 
    UFUNCTION(
        BlueprintPure,
        Category = "UE5PMS|AMD",
        meta = (
            DisplayName = "UE5PMS Get AMD (ADLX) Stats",
            CompactNodeTitle = "PMS_AMD",
            Keywords = "UE5PMS AMD ADLX performance stats telemetry"
            )
    )
    static FPMS_AMDStats GetAMDStats(bool& bOutIsValid, FString& OutErrorMessage);
};
