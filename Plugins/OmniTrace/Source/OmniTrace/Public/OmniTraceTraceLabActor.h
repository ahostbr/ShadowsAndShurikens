// Copyright (c) 2025 USP45Master. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OmniTraceTypes.h"
#include "OmniTraceBlueprintLibrary.h"

#include "OmniTraceTraceLabActor.generated.h"

/**
 * Dev-only helper actor for experimenting with OmniTrace patterns.
 *
 * Drop this into a test level and use the CallInEditor functions in the Details panel
 * to fire different trace patterns and visualize them.
 */
UCLASS()
class AOmniTraceTraceLabActor : public AActor
{
    GENERATED_BODY()

public:
    AOmniTraceTraceLabActor();

#if WITH_EDITOR
    /** Fire a simple forward multi-spread fan from the origin. */
    UFUNCTION(CallInEditor, Category="Trace Lab")
    void RunForwardFanTest();

    /** Fire a target-centred arc pattern from the origin towards TargetActor/Location. */
    UFUNCTION(CallInEditor, Category="Trace Lab")
    void RunTargetArcTest();

    /** Fire an orbit ring of rays around the origin. */
    UFUNCTION(CallInEditor, Category="Trace Lab")
    void RunOrbitRingTest();

    /** Fire a 3D radial burst of rays around the origin. */
    UFUNCTION(CallInEditor, Category="Trace Lab")
    void RunRadialBurstTest();

    /** Build points from PatternConfig and trace along them using PatternTraceTemplate. */
    UFUNCTION(CallInEditor, Category="Trace Lab")
    void RunPatternTraceTest();
#endif // WITH_EDITOR

protected:
    /** If true, this actor's transform is used as the origin for all tests. */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Origin")
    bool bUseActorTransformAsOrigin;

    /** Optional override origin (used when bUseActorTransformAsOrigin == false). */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Origin", meta=(EditCondition="!bUseActorTransformAsOrigin"))
    FTransform OriginOverride;

    /** Default ray count used for most tests. */
    UPROPERTY(EditAnywhere, Category="Trace Lab|General", meta=(ClampMin="1", UIMin="1"))
    int32 DefaultRayCount;

    /** Default max distance used for most tests (world units). */
    UPROPERTY(EditAnywhere, Category="Trace Lab|General", meta=(ClampMin="10.0", UIMin="100.0"))
    float DefaultMaxDistance;

    /** Forward fan request (PatternFamily = Forward). */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Requests")
    FOmniTraceRequest ForwardFanRequest;

    /** Target arc request (PatternFamily = Target). */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Requests")
    FOmniTraceRequest TargetArcRequest;

    /** Orbit ring request (PatternFamily = Orbit). */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Requests")
    FOmniTraceRequest OrbitRequest;

    /** Radial 3D burst request (PatternFamily = Radial3D). */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Requests")
    FOmniTraceRequest RadialBurstRequest;

    /** Pattern config used when calling OmniTrace_TraceAlongPatternConfig. */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Patterns")
    FOmniTracePatternConfig PatternConfig;

    /** Number of points to generate for PatternConfig. */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Patterns", meta=(ClampMin="1", UIMin="4"))
    int32 PatternNumPoints;

    /** Pattern scale (radius / extent in world units). */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Patterns", meta=(ClampMin="10.0", UIMin="50.0"))
    float PatternScale;

    /** Template request used when tracing along a pattern config. */
    UPROPERTY(EditAnywhere, Category="Trace Lab|Patterns")
    FOmniTraceRequest PatternTraceTemplate;

    /** Resolve the origin transform based on settings. */
    FTransform GetOriginTransform() const;

    /** Apply default debug settings (and RayCount / MaxDistance) to a request. */
    void ApplyDebugDefaults(FOmniTraceRequest& Request) const;

    /** Helper to run a single OmniTrace_Pattern() with the provided request. */
    void RunSinglePattern(const FOmniTraceRequest& Request);
};
