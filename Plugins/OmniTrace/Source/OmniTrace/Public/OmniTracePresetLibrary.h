// Copyright (c) 2025 USP45Master. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OmniTracePresetLibrary.generated.h"

class UOmniTracePatternPreset;

/** One entry in a pattern library. */
USTRUCT(BlueprintType)
struct FOmniTracePatternLibraryEntry
{
    GENERATED_BODY()

    /** Id used for lookup. If left None, the Preset's PresetId will be used. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace|Library")
    FName PresetId;

    /** The preset asset associated with this entry. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace|Library")
    TObjectPtr<UOmniTracePatternPreset> Preset;
};

/**
 * DataAsset that groups multiple UOmniTracePatternPreset assets.
 * Used for lookups by id and for building UI lists.
 */
UCLASS(BlueprintType)
class UOmniTracePatternLibrary : public UDataAsset
{
    GENERATED_BODY()

public:

    /** List of presets available in this library. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace|Library")
    TArray<FOmniTracePatternLibraryEntry> Entries;

    /** Simple linear search by id. Returns nullptr if not found. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OmniTrace|Library")
    UOmniTracePatternPreset* FindPresetById(FName InPresetId) const;
};
