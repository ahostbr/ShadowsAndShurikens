#include "UE5PMS_RHI_BlueprintLibrary.h"
#include "UE5PMS_RHI_Backend.h"

FUE5PMS_RhiMetrics UUE5PMS_RHI_BlueprintLibrary::GetRHIStats(const UObject* WorldContextObject, bool& bIsValid, FString& ErrorMessage)
{
	FUE5PMS_RhiMetrics Metrics;

	bool bOK = FPMS_RhiBackend::Get().QueryMetrics(WorldContextObject, Metrics);

	bIsValid = bOK && Metrics.bIsValid;
	ErrorMessage = Metrics.ErrorMessage;

	return Metrics;
}
