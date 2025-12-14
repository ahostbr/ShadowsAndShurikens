#include "UE5PMS_AMD_Backend.h"
#include "UE5PMS_AMD_Types.h"
#include "Misc/ScopeLock.h"
#include "Math/UnrealMathUtility.h"

DEFINE_LOG_CATEGORY_STATIC(LogUE5PMS_AMD, Log, All);

static FCriticalSection G_ADLX_MUTEX;

#if PLATFORM_WINDOWS && UE5PMS_HAS_ADLX

THIRD_PARTY_INCLUDES_START
#include "ADLXHelper/Windows/Cpp/ADLXHelper.h"
#include "Include/IPerformanceMonitoring.h"
THIRD_PARTY_INCLUDES_END

using namespace adlx;

// Global helper instance from ADLX SDK samples
static ADLXHelper G_ADLXHelper;

#endif // PLATFORM_WINDOWS && UE5PMS_HAS_ADLX

FPMS_ADLXBackend& FPMS_ADLXBackend::Get()
{
    static FPMS_ADLXBackend Inst;
    return Inst;
}

FPMS_ADLXBackend::FPMS_ADLXBackend()
{
}

FPMS_ADLXBackend::~FPMS_ADLXBackend()
{
    Shutdown();
}

bool FPMS_ADLXBackend::Initialize(FString& OutError)
{
    OutError.Reset();

#if !(PLATFORM_WINDOWS && UE5PMS_HAS_ADLX)
    OutError = TEXT("ADLX compiled out or unsupported platform.");
    UE_LOG(LogUE5PMS_AMD, Warning, TEXT("%s"), *OutError);
    return false;
#else
    if (bTriedInit)
        return bInitialized && bHasGPU;

    FScopeLock Lock(&G_ADLX_MUTEX);
    if (bTriedInit)
        return bInitialized && bHasGPU;

    bTriedInit   = true;
    bInitialized = false;
    bHasGPU      = false;

    ADLX_RESULT Res = G_ADLXHelper.Initialize();
    if (ADLX_FAILED(Res))
    {
        OutError = TEXT("ADLXHelper.Initialize() failed.");
        UE_LOG(LogUE5PMS_AMD, Error, TEXT("%s (Res=%d)"), *OutError, (int32)Res);
        return false;
    }

    // System services
    IADLXSystem* Sys = G_ADLXHelper.GetSystemServices();
    if (!Sys)
    {
        OutError = TEXT("ADLX GetSystemServices() returned null.");
        UE_LOG(LogUE5PMS_AMD, Error, TEXT("%s"), *OutError);
        G_ADLXHelper.Terminate();
        return false;
    }
    System = Sys;

    // Performance monitoring
    IADLXPerformanceMonitoringServices* Perf = nullptr;
    Res = System->GetPerformanceMonitoringServices(&Perf);
    if (ADLX_FAILED(Res) || !Perf)
    {
        OutError = TEXT("GetPerformanceMonitoringServices() failed.");
        UE_LOG(LogUE5PMS_AMD, Error, TEXT("%s (Res=%d)"), *OutError, (int32)Res);
        G_ADLXHelper.Terminate();
        System = nullptr;
        return false;
    }
    PerfRaw = Perf;

    // GPU list
    IADLXGPUList* GPUs = nullptr;
    Res = System->GetGPUs(&GPUs);
    if (ADLX_FAILED(Res) || !GPUs)
    {
        OutError = TEXT("GetGPUs() failed or returned null.");
        UE_LOG(LogUE5PMS_AMD, Error, TEXT("%s (Res=%d)"), *OutError, (int32)Res);
        G_ADLXHelper.Terminate();
        System  = nullptr;
        PerfRaw = nullptr;
        return false;
    }

    // Take the first GPU as primary
    IADLXGPU* OneGPU = nullptr;
    Res = GPUs->At(GPUs->Begin(), &OneGPU);
    if (ADLX_FAILED(Res) || !OneGPU)
    {
        OutError = TEXT("Failed to get first AMD GPU from GPU list.");
        UE_LOG(LogUE5PMS_AMD, Error, TEXT("%s (Res=%d)"), *OutError, (int32)Res);
        G_ADLXHelper.Terminate();
        System  = nullptr;
        PerfRaw = nullptr;
        return false;
    }
    PrimaryGpuRaw = OneGPU;

    bInitialized = true;
    bHasGPU      = true;

    UE_LOG(LogUE5PMS_AMD, Log, TEXT("ADLX initialized, AMD GPU detected."));
    return true;
#endif
}

void FPMS_ADLXBackend::Shutdown()
{
#if PLATFORM_WINDOWS && UE5PMS_HAS_ADLX
    FScopeLock Lock(&G_ADLX_MUTEX);

    if (bInitialized)
    {
        UE_LOG(LogUE5PMS_AMD, Log, TEXT("Shutting down ADLX backend."));
        G_ADLXHelper.Terminate();
    }

    System        = nullptr;
    PerfRaw       = nullptr;
    PrimaryGpuRaw = nullptr;
#endif

    bInitialized = false;
    bHasGPU      = false;
    bTriedInit   = false;
}

bool FPMS_ADLXBackend::QueryMetrics(FPMS_AMDStats& OutStats, FString& OutError)
{
    OutError.Reset();
    OutStats          = FPMS_AMDStats{};
    OutStats.bIsValid = false;
    OutStats.ErrorMessage.Empty();

#if !(PLATFORM_WINDOWS && UE5PMS_HAS_ADLX)
    OutError = TEXT("ADLX is not enabled in this build.");
    OutStats.ErrorMessage = OutError;
    UE_LOG(LogUE5PMS_AMD, Warning, TEXT("%s"), *OutError);
    return false;
#else
    if (!bInitialized)
    {
        if (!Initialize(OutError))
        {
            OutStats.ErrorMessage = OutError;
            return false;
        }
    }

    if (!bHasGPU || !System || !PerfRaw || !PrimaryGpuRaw)
    {
        OutError = TEXT("ADLX backend has no valid AMD GPU handle.");
        OutStats.ErrorMessage = OutError;
        UE_LOG(LogUE5PMS_AMD, Error, TEXT("%s"), *OutError);
        return false;
    }

    IADLXPerformanceMonitoringServices* Perf = PerfRaw;
    IADLXGPU* Gpu = PrimaryGpuRaw;

    IADLXGPUMetricsSupport* MetricsSupport = nullptr;
    ADLX_RESULT Res = Perf->GetSupportedGPUMetrics(Gpu, &MetricsSupport);
    if (ADLX_FAILED(Res) || !MetricsSupport)
    {
        OutError = TEXT("GetSupportedGPUMetrics() failed.");
        OutStats.ErrorMessage = OutError;
        UE_LOG(LogUE5PMS_AMD, Error, TEXT("%s (Res=%d)"), *OutError, (int32)Res);
        return false;
    }

    IADLXGPUMetrics* Metrics = nullptr;
    Res = Perf->GetCurrentGPUMetrics(Gpu, &Metrics);
    if (ADLX_FAILED(Res) || !Metrics)
    {
        OutError = TEXT("GetCurrentGPUMetrics() failed.");
        OutStats.ErrorMessage = OutError;
        UE_LOG(LogUE5PMS_AMD, Error, TEXT("%s (Res=%d)"), *OutError, (int32)Res);
        return false;
    }

    bool bAnySuccess = false;

    // GPU Temperature (°C)
    {
        adlx_bool bSupported = false;
        Res = MetricsSupport->IsSupportedGPUTemperature(&bSupported);
        if (ADLX_SUCCEEDED(Res) && bSupported)
        {
            adlx_double Temp = 0.0;
            Res = Metrics->GPUTemperature(&Temp);
            if (ADLX_SUCCEEDED(Res))
            {
                OutStats.TemperatureC = (int32)FMath::RoundToInt((float)Temp);
                bAnySuccess = true;
            }
        }
    }

    // GPU usage %
    {
        adlx_bool bSupported = false;
        Res = MetricsSupport->IsSupportedGPUUsage(&bSupported);
        if (ADLX_SUCCEEDED(Res) && bSupported)
        {
            adlx_double Usage = 0.0;
            Res = Metrics->GPUUsage(&Usage);
            if (ADLX_SUCCEEDED(Res))
            {
                OutStats.GpuUsagePercent = (int32)FMath::RoundToInt((float)Usage);
                bAnySuccess = true;
            }
        }
    }

    // GPU VRAM (MB) - "usage" not total, because ADLX exposes usage here.
    {
        adlx_bool bSupported = false;
        Res = MetricsSupport->IsSupportedGPUVRAM(&bSupported);
        if (ADLX_SUCCEEDED(Res) && bSupported)
        {
            adlx_int VramMB = 0;
            Res = Metrics->GPUVRAM(&VramMB);
            if (ADLX_SUCCEEDED(Res))
            {
                OutStats.VramUsedMB = (int32)VramMB;
                bAnySuccess = true;
            }
        }
    }

    if (!bAnySuccess)
    {
        OutStats.bIsValid      = false;
        OutStats.Status        = EPMS_StatusCode::UnknownError;
        OutError               = TEXT("ADLX metrics not available for this GPU.");
        OutStats.ErrorMessage  = OutError;
        UE_LOG(LogUE5PMS_AMD, Warning, TEXT("%s"), *OutError);
        return false;
    }

    OutStats.bIsValid     = true;
    OutStats.Status       = EPMS_StatusCode::OK;
    OutStats.ErrorMessage = TEXT("");
    return true;
#endif
}
