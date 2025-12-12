// Copyright (c) 2025 USP45Master. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OmniTracePatternPreset.h"
#include "SOTS_OmniTraceKEMPresetLibrary.generated.h"

UENUM(BlueprintType)
enum class ESOTS_OmniTraceKEMPreset : uint8
{
    Unknown         UMETA(DisplayName="Unknown"),
    GroundRear      UMETA(DisplayName="Ground Rear"),
    GroundFront     UMETA(DisplayName="Ground Front"),
    GroundLeft      UMETA(DisplayName="Ground Left"),
    GroundRight     UMETA(DisplayName="Ground Right"),
    CornerLeft      UMETA(DisplayName="Corner Left"),
    CornerRight     UMETA(DisplayName="Corner Right"),
    VerticalAbove   UMETA(DisplayName="Vertical Above"),
    VerticalBelow   UMETA(DisplayName="Vertical Below")
};

USTRUCT(BlueprintType)
struct FSOTS_OmniTraceKEMPresetEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    ESOTS_OmniTraceKEMPreset PresetId = ESOTS_OmniTraceKEMPreset::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    FVector LocalDirection = FVector::ForwardVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    bool bUseGroundRelative = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    bool bUseVertical = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM", meta=(EditCondition="bUseVertical"))
    bool bVerticalAbove = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM", meta=(EditCondition="bUseVertical"))
    float VerticalTraceDistance = 900.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    float TraceDistance = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    FName PatternLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    TSoftObjectPtr<UOmniTracePatternPreset> PatternPreset;
};

UCLASS(BlueprintType)
class OMNITRACE_API USOTS_OmniTraceKEMPresetLibrary : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    TArray<FSOTS_OmniTraceKEMPresetEntry> Entries;

    const FSOTS_OmniTraceKEMPresetEntry* FindEntry(ESOTS_OmniTraceKEMPreset Preset) const;
    UOmniTracePatternPreset* FindPreset(ESOTS_OmniTraceKEMPreset Preset) const;
};
