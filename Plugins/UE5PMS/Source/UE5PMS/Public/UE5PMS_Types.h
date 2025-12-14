#pragma once

#include "CoreMinimal.h"
#include "UE5PMS_Types.generated.h"

// ---------------------------------------------------------
// Shared status enum used by all vendors
// ---------------------------------------------------------
UENUM(BlueprintType)
enum class EPMS_StatusCode : uint8
{
    OK                      UMETA(DisplayName = "OK"),
    NotInitialized          UMETA(DisplayName = "Not Initialized"),
    NoCompatibleDevice      UMETA(DisplayName = "No Compatible Device"),
    APINotFound             UMETA(DisplayName = "API Not Found / DLL Missing"),
    APICallFailed           UMETA(DisplayName = "API Call Failed"),
    UnsupportedPlatform     UMETA(DisplayName = "Unsupported Platform"),
    UnknownError            UMETA(DisplayName = "Unknown Error")
};

// ---------------------------------------------------------
// RHI generic stats
// ---------------------------------------------------------
USTRUCT(BlueprintType)
struct FPMS_RHIStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
    bool bIsValid = false;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
    EPMS_StatusCode Status = EPMS_StatusCode::NotInitialized;

    // Name of the adapter as seen by RHI
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
    FString AdapterName;

    // Memory in MB
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
    int32 DedicatedVideoMemoryMB = -1;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
    int32 DedicatedSystemMemoryMB = -1;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|RHI")
    int32 SharedSystemMemoryMB = -1;
};

// ---------------------------------------------------------
// NVIDIA (NVAPI) stats
// ---------------------------------------------------------
USTRUCT(BlueprintType)
struct FPMS_NvidiaStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    bool bIsValid = false;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    EPMS_StatusCode Status = EPMS_StatusCode::NotInitialized;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    FString GpuName;

    // VRAM in MB
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    int32 VramTotalMB = -1;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    int32 VramUsedMB = -1;

    // Utilization in %
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    int32 GpuUsagePercent = -1;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    int32 MemoryControllerUsagePercent = -1;

    // Clocks in MHz
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    int32 CoreClockMHz = -1;

    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    int32 MemoryClockMHz = -1;

    // Temperature in °C
    UPROPERTY(BlueprintReadOnly, Category = "UE5PMS|NVIDIA")
    int32 TemperatureC = -1;
};

// NOTE: AMD types live in UE5PMS_AMD_Types.h
