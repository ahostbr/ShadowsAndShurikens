#pragma once

#include "CoreMinimal.h"
#include "UE5PMS_RHI_Types.h"

/**
 * FPMS_RhiBackend
 *
 * Simple timing backend using UWorld DeltaSeconds.
 * No vendor APIs, no decisions – just timing.
 */
class UE5PMS_API FPMS_RhiBackend
{
public:
	static FPMS_RhiBackend& Get();

	/**
	 * Query timing metrics from the given world context.
	 *
	 * - OutMetrics.bIsValid will be true on success.
	 * - OutMetrics.ErrorMessage will be filled on failure.
	 */
	bool QueryMetrics(const UObject* WorldContextObject, FUE5PMS_RhiMetrics& OutMetrics);

private:
	FPMS_RhiBackend() = default;
	~FPMS_RhiBackend() = default;

	FPMS_RhiBackend(const FPMS_RhiBackend&) = delete;
	FPMS_RhiBackend& operator=(const FPMS_RhiBackend&) = delete;

private:
	// Simple running average – you can adjust this alpha or move smoothing to BP if you prefer.
	float LastSmoothedFPS = -1.0f;
	float LastSmoothedFrameTimeMS = -1.0f;
};
