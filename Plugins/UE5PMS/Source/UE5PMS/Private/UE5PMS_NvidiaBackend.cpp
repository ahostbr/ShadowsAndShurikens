#include "UE5PMS_NvidiaBackend.h"
#include "Misc/ScopeLock.h"

//Nvidia Logging
//DEFINE_LOG_CATEGORY_STATIC(LogUE5PMS_NV, Log, All);


static FCriticalSection G_NVAPI_MUTEX;

FPMS_NvApiBackend& FPMS_NvApiBackend::Get()
{
	static FPMS_NvApiBackend Inst;
	return Inst;
}

FPMS_NvApiBackend::FPMS_NvApiBackend()
{
}

FPMS_NvApiBackend::~FPMS_NvApiBackend()
{
	Shutdown();
}

FString FPMS_NvApiBackend::NvStatusToString(NvAPI_Status Status) const
{
#if UE5PMS_HAS_NVAPI
	NvAPI_ShortString NVStr = {};
	NvAPI_GetErrorMessage(Status, NVStr);
	return UTF8_TO_TCHAR(NVStr);
#else
	return TEXT("NVAPI unavailable");
#endif
}

bool FPMS_NvApiBackend::Initialize(FString& OutError)
{
	//UE_LOG(LogUE5PMS_NV, Warning, TEXT("FPMS_NvApiBackend::Initialize (UE5PMS_HAS_NVAPI=%d)"), UE5PMS_HAS_NVAPI);

#if !(PLATFORM_WINDOWS && UE5PMS_HAS_NVAPI)
	OutError = TEXT("NVAPI compiled out at build time (platform or macro disabled).");
	UE_LOG(LogUE5PMS_NV, Error, TEXT("%s"), *OutError);
	return false;
#else
	if (bTriedInit)
		return bInitialized && bHasGpu;

	FScopeLock Lock(&G_NVAPI_MUTEX);
	if (bTriedInit)
		return bInitialized && bHasGpu;

	bTriedInit = true;
	bInitialized = false;
	bHasGpu = false;

	NvAPI_Status Status = NvAPI_Initialize();
	if (Status != NVAPI_OK)
	{
		OutError = NvStatusToString(Status);
		if (OutError.IsEmpty())
		{
			OutError = TEXT("NvAPI_Initialize failed with unknown error.");
		}
	//	UE_LOG(LogUE5PMS_NV, Error, TEXT("%s"), *OutError);
		return false;
	}

	// Enumerate GPUs
	NvPhysicalGpuHandle Handles[NVAPI_MAX_PHYSICAL_GPUS] = {};
	NvU32 GpuCount = 0;
	Status = NvAPI_EnumPhysicalGPUs(Handles, &GpuCount);
	if (Status != NVAPI_OK || GpuCount == 0)
	{
		OutError = NvStatusToString(Status);
		if (OutError.IsEmpty())
		{
			OutError = TEXT("NvAPI_EnumPhysicalGPUs failed or found no GPUs.");
		}
	//	UE_LOG(LogUE5PMS_NV, Error, TEXT("%s"), *OutError);
		NvAPI_Unload();
		return false;
	}

	PrimaryGpu = Handles[0];   // assume first GPU
	bHasGpu = true;
	bInitialized = true;

//	UE_LOG(LogUE5PMS_NV, Warning, TEXT("NVAPI initialized, %u GPU(s) detected"), GpuCount);
	OutError.Reset();
	return true;
#endif
}

void FPMS_NvApiBackend::Shutdown()
{
#if UE5PMS_HAS_NVAPI
	if (bInitialized)
	{
		NvAPI_Unload();
	}
#endif
	bInitialized = false;
	bHasGpu = false;
}

bool FPMS_NvApiBackend::QueryMetrics(FUE5PMS_NvGpuMetrics& OutMetrics)
{
	OutMetrics = FUE5PMS_NvGpuMetrics{};
	OutMetrics.bIsValid = false;

	FString InitError;
	if (!Initialize(InitError))
	{
		OutMetrics.ErrorMessage = InitError;
		return false;
	}

#if !UE5PMS_HAS_NVAPI
	OutMetrics.ErrorMessage = TEXT("NVAPI unavailable.");
	return false;
#else

	//------------------------------------------------------------
	// GPU Name
	//------------------------------------------------------------
	{
		NvAPI_ShortString Name = {};
		NvAPI_Status S = NvAPI_GPU_GetFullName(PrimaryGpu, Name);
		if (S == NVAPI_OK)
		{
			OutMetrics.GpuName = UTF8_TO_TCHAR(Name);
		}
	}

	//------------------------------------------------------------
	// Memory Info (dedicated VRAM total / used)
	// Uses NvAPI_GPU_GetMemoryInfoEx + NV_GPU_MEMORY_INFO_EX
	//------------------------------------------------------------
	{
		NV_GPU_MEMORY_INFO_EX Mem = {};
		Mem.version = NV_GPU_MEMORY_INFO_EX_VER;

		NvAPI_Status S = NvAPI_GPU_GetMemoryInfoEx(PrimaryGpu, &Mem);
		if (S == NVAPI_OK)
		{
			// NV_GPU_MEMORY_INFO_EX reports values in BYTES.
			const double BytesToMB = 1.0 / (1024.0 * 1024.0);

			const double TotalMB = Mem.dedicatedVideoMemory * BytesToMB;
			const double AvailableMB = Mem.curAvailableDedicatedVideoMemory * BytesToMB;

			OutMetrics.VramTotalMB = static_cast<int32>(TotalMB + 0.5);

			if (OutMetrics.VramTotalMB > 0)
			{
				const double UsedMB = TotalMB - AvailableMB;
				OutMetrics.VramUsedMB = static_cast<int32>(UsedMB + 0.5);
			}
		}
		else
		{
			// Not fatal for the whole query – we just report the error.
			OutMetrics.ErrorMessage = FString::Printf(
				TEXT("NvAPI_GPU_GetMemoryInfoEx failed: %s"),
				*NvStatusToString(S)
			);
		}
	}

	//------------------------------------------------------------
	// Utilization (GPU + Memory Controller)
	//------------------------------------------------------------
	{
		NV_GPU_DYNAMIC_PSTATES_INFO_EX Info = {};
		Info.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;

		NvAPI_Status S = NvAPI_GPU_GetDynamicPstatesInfoEx(PrimaryGpu, &Info);
		if (S == NVAPI_OK)
		{
			OutMetrics.GpuUsagePercent =
				Info.utilization[NVAPI_GPU_UTILIZATION_DOMAIN_GPU].percentage;

			OutMetrics.MemoryControllerUsagePercent =
				Info.utilization[NVAPI_GPU_UTILIZATION_DOMAIN_FB].percentage;
		}
	}

	//------------------------------------------------------------
	// Clocks (core + memory)
	//------------------------------------------------------------
	{
		NV_GPU_CLOCK_FREQUENCIES Clocks = {};
		Clocks.version = NV_GPU_CLOCK_FREQUENCIES_VER;

		NvAPI_Status S = NvAPI_GPU_GetAllClockFrequencies(PrimaryGpu, &Clocks);
		if (S == NVAPI_OK)
		{
			if (Clocks.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].bIsPresent)
			{
				OutMetrics.CoreClockMHz =
					Clocks.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency / 1000;
			}

			if (Clocks.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].bIsPresent)
			{
				OutMetrics.MemoryClockMHz =
					Clocks.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].frequency / 1000;
			}
		}
	}

	//------------------------------------------------------------
	// Temperature
	//------------------------------------------------------------
	{
		NV_GPU_THERMAL_SETTINGS Thermal = {};
		Thermal.version = NV_GPU_THERMAL_SETTINGS_VER;

		NvAPI_Status S = NvAPI_GPU_GetThermalSettings(
			PrimaryGpu,
			NVAPI_THERMAL_TARGET_GPU,
			&Thermal
		);

		if (S == NVAPI_OK && Thermal.count > 0)
		{
			OutMetrics.TemperatureC = Thermal.sensor[0].currentTemp;
		}
	}

	OutMetrics.bIsValid = true;
	return true;

#endif // UE5PMS_HAS_NVAPI
}
