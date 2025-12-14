#include "UE5PMS_RHI_Backend.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

FPMS_RhiBackend& FPMS_RhiBackend::Get()
{
	static FPMS_RhiBackend Inst;
	return Inst;
}

bool FPMS_RhiBackend::QueryMetrics(const UObject* WorldContextObject, FUE5PMS_RhiMetrics& OutMetrics)
{
	OutMetrics = FUE5PMS_RhiMetrics{};
	OutMetrics.bIsValid = false;

	if (!WorldContextObject)
	{
		OutMetrics.ErrorMessage = TEXT("WorldContextObject is null.");
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		OutMetrics.ErrorMessage = TEXT("WorldContextObject has no valid UWorld.");
		return false;
	}

	const float DeltaSeconds = World->GetDeltaSeconds();

	if (DeltaSeconds <= 0.f)
	{
		OutMetrics.ErrorMessage = TEXT("DeltaSeconds <= 0. Timing not valid this frame.");
		return false;
	}

	const float FPS = 1.0f / DeltaSeconds;
	const float FrameMS = DeltaSeconds * 1000.0f;

	OutMetrics.CurrentFPS = FPS;
	OutMetrics.FrameTimeMS = FrameMS;

	// Simple exponential smoothing – you can tune or move this to BP logic later.
	const float Alpha = 0.1f;

	if (LastSmoothedFPS < 0.f)
	{
		LastSmoothedFPS = FPS;
		LastSmoothedFrameTimeMS = FrameMS;
	}
	else
	{
		LastSmoothedFPS = FMath::Lerp(LastSmoothedFPS, FPS, Alpha);
		LastSmoothedFrameTimeMS = FMath::Lerp(LastSmoothedFrameTimeMS, FrameMS, Alpha);
	}

	OutMetrics.SmoothedFPS = LastSmoothedFPS;
	OutMetrics.SmoothedFrameTimeMS = LastSmoothedFrameTimeMS;

	OutMetrics.bIsValid = true;
	OutMetrics.ErrorMessage.Reset();

	return true;
}
