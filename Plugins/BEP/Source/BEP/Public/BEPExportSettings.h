#pragma once

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "BEPExportSettings.generated.h"

UENUM()
enum class EBEPExportOutputFormat : uint8
{
    Text  UMETA(DisplayName = "Text"),
    Json  UMETA(DisplayName = "JSON"),
    Csv   UMETA(DisplayName = "CSV"),
    All   UMETA(DisplayName = "All")
};

USTRUCT()
struct FBEPExportSettings
{
    GENERATED_BODY()

    /** Root content path, e.g. /Game or /Game/SOTS */
    UPROPERTY()
    FString RootPath = TEXT("/Game");

    /** Disk output root, e.g. C:/BEP_EXPORTS/SOTS or <ProjectSavedDir>/BEPExport */
    UPROPERTY()
    FString OutputRootPath;

    /** Output format choice */
    UPROPERTY()
    EBEPExportOutputFormat OutputFormat = EBEPExportOutputFormat::Json;

    /** Comma- or newline-separated class names / substrings to exclude */
    UPROPERTY()
    FString ExcludedClassPatterns;

    /** Safety cap to avoid loading too many assets at once. */
    UPROPERTY()
    int32 MaxAssetsPerRun = 500;

    FBEPExportSettings()
    {
        OutputRootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("BEPExport"));
    }
};
