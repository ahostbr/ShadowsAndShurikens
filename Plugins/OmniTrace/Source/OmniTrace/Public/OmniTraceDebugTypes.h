#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_OmniTraceKEMPresetLibrary.h"

#include "OmniTraceDebugTypes.generated.h"

UENUM()
enum class EOmniTraceKEMExecutionFamily : uint8
{
    Unknown,
    GroundRear,
    GroundFront,
    GroundLeft,
    GroundRight,
    CornerLeft,
    CornerRight,
    VerticalAbove,
    VerticalBelow,
    DropPoint,
    PullDown,
    Cinematic,
};

USTRUCT()
struct OMNITRACE_API FSOTS_OmniTraceKEMDebugRecord
{
    GENERATED_BODY()

    UPROPERTY()
    ESOTS_OmniTraceKEMPreset PresetId = ESOTS_OmniTraceKEMPreset::Unknown;

    UPROPERTY()
    FVector InstigatorLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY()
    FGameplayTag PositionTag;

    UPROPERTY()
    EOmniTraceKEMExecutionFamily ExecutionFamily = EOmniTraceKEMExecutionFamily::Unknown;

    UPROPERTY()
    bool bHit = false;

    UPROPERTY()
    FVector HitLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector HitNormal = FVector::UpVector;

    UPROPERTY()
    FTransform FinalSpawnTransform = FTransform::Identity;
};
