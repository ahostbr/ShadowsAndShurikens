// Copyright (c) 2025 USP45Master. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OmniTraceTypes.h"
#include "OmniTracePatternPreset.generated.h"

/**
 * DataAsset wrapper for FOmniTraceRequest plus metadata.
 * Users create presets of this type and pass them to the library functions.
 */
UCLASS(BlueprintType)
class OMNITRACE_API UOmniTracePatternPreset : public UDataAsset
{
    GENERATED_BODY()

public:

    UOmniTracePatternPreset();

    /** Stable internal identifier for this preset (for lookups, debug, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace|Preset")
    FName PresetId;

    /** Display name shown in tools / UIs. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace|Preset")
    FText DisplayName;

    /** Short human description of what this preset does. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace|Preset", meta = (MultiLine = true))
    FText Description;

    /** Loose category tag (Vision, Weapon, Environment, Utility, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace|Preset")
    FName Category;

    /** Default debug color for this preset's lines. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace|Preset")
    FLinearColor FamilyColor;

    /**
     * If true, when applying this preset the FamilyColor will overwrite
     * Request.DebugOptions.Color.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace|Preset")
    bool bOverrideDebugColorFromFamily;

    /** The underlying OmniTrace request this preset represents. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OmniTrace")
    FOmniTraceRequest Request;
};
