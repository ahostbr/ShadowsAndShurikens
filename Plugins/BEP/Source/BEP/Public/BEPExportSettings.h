#pragma once

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "BEPExportSettings.generated.h"

/** Default BEP export root under the current project directory. */
BEP_API FString GetDefaultBEPExportRoot();

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

    /** Disk output root; if relative or empty, it is placed under <ProjectDir>/BEP_EXPORTS. */
    UPROPERTY()
    FString OutputRootPath;

    /** Output format choice */
    UPROPERTY()
    EBEPExportOutputFormat OutputFormat = EBEPExportOutputFormat::Json;

    /** Organize Blueprint flow exports under Blueprint/package folders (recommended). */
    UPROPERTY()
    bool bOrganizeBlueprintFlowsByPackagePath = true;

    /** Place each export run under a timestamped subfolder for easier diffing. */
    UPROPERTY()
    bool bUseTimestampRunFolder = true;

    /** Preserve the legacy flat BlueprintFlows layout (disables organize/timestamp). */
    UPROPERTY()
    bool bUseLegacyFlatBlueprintFlowsLayout = false;

    /** Comma- or newline-separated class names / substrings to exclude */
    UPROPERTY()
    FString ExcludedClassPatterns;

    /** Safety cap to avoid loading too many assets at once. */
    UPROPERTY()
    int32 MaxAssetsPerRun = 500;

    FBEPExportSettings()
    {
        OutputRootPath = GetDefaultBEPExportRoot();
    }
};
