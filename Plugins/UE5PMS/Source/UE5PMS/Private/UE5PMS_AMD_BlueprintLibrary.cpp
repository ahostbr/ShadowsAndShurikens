// Source/UE5PMS/Private/UE5PMS_AMD_BlueprintLibrary.cpp

#include "UE5PMS_AMD_BlueprintLibrary.h"
#include "UE5PMS_AMD_Backend.h"
#include "UE5PMS_Types.h"
FPMS_AMDStats UUE5PMS_AMD_BlueprintLibrary::GetAMDStats(bool& bOutIsValid, FString& OutErrorMessage)
{
    FPMS_AMDStats Stats;
    FString Error;

    FPMS_ADLXBackend& Backend = FPMS_ADLXBackend::Get();
    bool bOk = Backend.QueryMetrics(Stats, Error);

    bOutIsValid = bOk && Stats.bIsValid;
    OutErrorMessage = Error;

    // If backend says it's invalid but didn't set the struct's error, propagate

    if (!bOutIsValid && Stats.ErrorMessage.IsEmpty() && !Error.IsEmpty())
    {
        Stats.ErrorMessage = Error;
    }

    return Stats;
}