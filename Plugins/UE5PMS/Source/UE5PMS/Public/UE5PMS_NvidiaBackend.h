#pragma once

#include "CoreMinimal.h"
#include "UE5PMS_NvidiaTypes.h"

#if PLATFORM_WINDOWS

#define UE5PMS_HAS_NVAPI 1

// NVAPI pulls in Windows headers; wrap them to keep Unreal's namespace clean
#include "Windows/AllowWindowsPlatformTypes.h"
//THIRD_PARTY_INCLUDES_START
#include "nvapi.h"
//THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformTypes.h"

// -------------------------------------------------------------------------
// NVAPI compatibility:
// Some SDK drops expose NV_GPU_DYNAMIC_PSTATES_INFO_EX but *do not* define
// the NVAPI_GPU_UTILIZATION_DOMAIN_* enums that the docs reference.
// Define them here so our code (and any consumers) can rely on them.
// -------------------------------------------------------------------------
#ifndef NVAPI_GPU_UTILIZATION_DOMAIN_GPU
#define NVAPI_GPU_UTILIZATION_DOMAIN_GPU 0   // Graphics core
#endif

#ifndef NVAPI_GPU_UTILIZATION_DOMAIN_FB
#define NVAPI_GPU_UTILIZATION_DOMAIN_FB  1   // Frame buffer (VRAM)
#endif

#ifndef NVAPI_GPU_UTILIZATION_DOMAIN_VID
#define NVAPI_GPU_UTILIZATION_DOMAIN_VID 2   // Video engine
#endif

#ifndef NVAPI_GPU_UTILIZATION_DOMAIN_BUS
#define NVAPI_GPU_UTILIZATION_DOMAIN_BUS 3   // PCIe bus
#endif

#else

#define UE5PMS_HAS_NVAPI 0

#endif // PLATFORM_WINDOWS

class UE5PMS_API FPMS_NvApiBackend
{
public:
	static FPMS_NvApiBackend& Get();

	/** True if NVAPI initialized successfully and at least one NVIDIA GPU exists. */
	bool IsAvailable() const { return bHasGpu; }

	/** Query latest GPU telemetry. Returns false if NVAPI is unavailable or query failed. */
	bool QueryMetrics(FUE5PMS_NvGpuMetrics& OutMetrics);

private:
	FPMS_NvApiBackend();
	~FPMS_NvApiBackend();

	bool Initialize(FString& OutError);
	void Shutdown();

	FString NvStatusToString(NvAPI_Status Status) const;

private:
	bool bInitialized = false;
	bool bHasGpu = false;
	bool bTriedInit = false;

#if UE5PMS_HAS_NVAPI
	NvPhysicalGpuHandle PrimaryGpu = nullptr;
#endif
};
