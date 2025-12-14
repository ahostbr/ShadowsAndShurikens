#pragma once

#include "CoreMinimal.h"

struct FPMS_AMDStats;

namespace adlx
{
    class IADLXSystem;
    class IADLXPerformanceMonitoringServices;
    class IADLXGPU;
}

// Thin ADLX-based backend for AMD metrics
class FPMS_ADLXBackend
{
public:
    static FPMS_ADLXBackend& Get();

    bool Initialize(FString& OutError);
    void Shutdown();

    // Fills OutStats and returns true if at least one metric was successfully read
    bool QueryMetrics(FPMS_AMDStats& OutStats, FString& OutError);

private:
    FPMS_ADLXBackend();
    ~FPMS_ADLXBackend();

    FPMS_ADLXBackend(const FPMS_ADLXBackend&) = delete;
    FPMS_ADLXBackend& operator=(const FPMS_ADLXBackend&) = delete;

    bool bTriedInit   = false;
    bool bInitialized = false;
    bool bHasGPU      = false;

    adlx::IADLXSystem*                       System        = nullptr;
    adlx::IADLXPerformanceMonitoringServices* PerfRaw      = nullptr;
    adlx::IADLXGPU*                          PrimaryGpuRaw = nullptr;
};
