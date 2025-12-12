// Copyright (c) 2025 USP45Master. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/LatentActionManager.h"
#include "OmniTraceTypes.h"
#include "OmniTraceBlueprintLibrary.generated.h"

class UOmniTracePatternPreset;
class UOmniTracePatternLibrary;

/**
 * Main Blueprint API surface for OmniTrace.
 *
 * This library operates on FOmniTraceRequest / FOmniTracePatternResult and
 * provides helpers to work with presets, libraries, and builtin presets.
 */
UCLASS()
class OMNITRACE_API UOmniTraceBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Core pattern function – fires traces according to the request. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject"))
    static FOmniTracePatternResult OmniTrace_Pattern(
        UObject* WorldContextObject,
        const FOmniTraceRequest& Request);

    /** Latent version of OmniTrace_Pattern. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject", Latent, LatentInfo="LatentInfo"))
    static void OmniTrace_Pattern_Async(
        UObject* WorldContextObject,
        FLatentActionInfo LatentInfo,
        FOmniTracePatternResult& OutResult,
        const FOmniTraceRequest& Request);

    /** Run a pattern from a UOmniTracePatternPreset asset. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject"))
    static FOmniTracePatternResult OmniTrace_PatternFromPreset(
        UObject* WorldContextObject,
        UOmniTracePatternPreset* Preset,
        AActor* OriginActorOverride,
        USceneComponent* OriginComponentOverride,
        AActor* TargetActorOverride);

    /** Latent version of OmniTrace_PatternFromPreset. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject", Latent, LatentInfo="LatentInfo"))
    static void OmniTrace_PatternFromPreset_Async(
        UObject* WorldContextObject,
        FLatentActionInfo LatentInfo,
        FOmniTracePatternResult& OutResult,
        UOmniTracePatternPreset* Preset,
        AActor* OriginActorOverride,
        USceneComponent* OriginComponentOverride,
        AActor* TargetActorOverride);

    /** Run a pattern from a pattern library by preset id. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject"))
    static FOmniTracePatternResult OmniTrace_PatternFromLibrary(
        UObject* WorldContextObject,
        UOmniTracePatternLibrary* Library,
        FName PresetId,
        AActor* OriginActorOverride,
        USceneComponent* OriginComponentOverride,
        AActor* TargetActorOverride);

    /** Latent version of OmniTrace_PatternFromLibrary. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject", Latent, LatentInfo="LatentInfo"))
    static void OmniTrace_PatternFromLibrary_Async(
        UObject* WorldContextObject,
        FLatentActionInfo LatentInfo,
        FOmniTracePatternResult& OutResult,
        UOmniTracePatternLibrary* Library,
        FName PresetId,
        AActor* OriginActorOverride,
        USceneComponent* OriginComponentOverride,
        AActor* TargetActorOverride);

    /** Get lightweight preset infos from a pattern library. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace")
    static void OmniTrace_GetLibraryPresetInfos(
        UOmniTracePatternLibrary* Library,
        TArray<FOmniTracePresetInfo>& OutInfos);

    /** Run a pattern from a builtin preset id. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject"))
    static FOmniTracePatternResult OmniTrace_PatternFromBuiltinPreset(
        UObject* WorldContextObject,
        FName PresetId,
        AActor* OriginActorOverride,
        USceneComponent* OriginComponentOverride,
        AActor* TargetActorOverride);

    /** Latent version of OmniTrace_PatternFromBuiltinPreset. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject", Latent, LatentInfo="LatentInfo"))
    static void OmniTrace_PatternFromBuiltinPreset_Async(
        UObject* WorldContextObject,
        FLatentActionInfo LatentInfo,
        FOmniTracePatternResult& OutResult,
        FName PresetId,
        AActor* OriginActorOverride,
        USceneComponent* OriginComponentOverride,
        AActor* TargetActorOverride);


    /** Build a builtin preset id from a structured key. */
    UFUNCTION(BlueprintPure, Category="OmniTrace|Presets")
    static FName OmniTrace_MakeBuiltinPresetId(const FOmniTraceBuiltinPresetKey& Key);

    /** Run a pattern using a structured builtin preset key. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject"))
    static FOmniTracePatternResult OmniTrace_PatternFromBuiltinPresetKey(
        UObject* WorldContextObject,
        const FOmniTraceBuiltinPresetKey& Key,
        AActor* OriginActorOverride,
        USceneComponent* OriginComponentOverride,
        AActor* TargetActorOverride);

    /** Latent version of OmniTrace_PatternFromBuiltinPresetKey. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace", meta=(WorldContext="WorldContextObject", Latent, LatentInfo="LatentInfo"))
    static void OmniTrace_PatternFromBuiltinPresetKey_Async(
        UObject* WorldContextObject,
        FLatentActionInfo LatentInfo,
        FOmniTracePatternResult& OutResult,
        const FOmniTraceBuiltinPresetKey& Key,
        AActor* OriginActorOverride,
        USceneComponent* OriginComponentOverride,
        AActor* TargetActorOverride);


    /** Generate an array of local pattern points from a high-level pattern config.
     *  Origin defines the local frame (location + rotation). Points are returned in world space.
     */
    UFUNCTION(BlueprintCallable, Category="OmniTrace|Patterns")
    static void OmniTrace_BuildPathFromPatternConfig(
        const FTransform& Origin,
        const FOmniTracePatternConfig& Config,
        int32 NumPoints,
        float Scale,
        TArray<FVector>& OutPoints);

    /**
     * Convenience helper: build points from a pattern config and trace along them using a
     * standard OmniTrace request template.
     */
    UFUNCTION(BlueprintCallable, Category="OmniTrace|Patterns", meta=(WorldContext="WorldContextObject"))
    static void OmniTrace_TraceAlongPatternConfig(
        UObject* WorldContextObject,
        const FTransform& Origin,
        const FOmniTracePatternConfig& Config,
        int32 NumPoints,
        float Scale,
        const FOmniTraceRequest& TraceTemplate,
        FOmniTracePatternResult& OutResult);

    /** Get infos for all builtin presets. */
    UFUNCTION(BlueprintCallable, Category="OmniTrace")
    static void OmniTrace_GetBuiltinPresetInfos(
        TArray<FOmniTracePresetInfo>& OutInfos);

    };