#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_OmniTraceKEMPresetLibrary.h"
#include "SOTS_KEM_OmniTraceTuning.generated.h"

USTRUCT(BlueprintType)
struct FSOTS_KEM_OmniTraceTuning
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|OmniTrace")
    ESOTS_OmniTraceKEMPreset PresetId = ESOTS_OmniTraceKEMPreset::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|OmniTrace", meta=(ToolTip="0 = use preset default"))
    float MaxDistanceOverride = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|OmniTrace", meta=(ToolTip="0 = use preset default"))
    float MaxVerticalOffsetOverride = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|OmniTrace", meta=(ToolTip="Allow steeper helper angles than the default Wanders"))
    bool bAllowSteepAngles = false;
};

UCLASS(BlueprintType)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_OmniTraceTuningConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|OmniTrace")
    TArray<FSOTS_KEM_OmniTraceTuning> Entries;
};
