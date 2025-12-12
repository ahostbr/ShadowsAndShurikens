// Copyright (c) 2025 USP45Master. All Rights Reserved.

#include "OmniTraceTraceLabActor.h"

#include "Engine/World.h"

AOmniTraceTraceLabActor::AOmniTraceTraceLabActor()
{
    PrimaryActorTick.bCanEverTick = false;

    bUseActorTransformAsOrigin = true;
    OriginOverride = FTransform::Identity;

    DefaultRayCount     = 11;
    DefaultMaxDistance  = 2000.0f;
    PatternNumPoints    = 32;
    PatternScale        = 600.0f;

    // Forward fan defaults.
    ForwardFanRequest.PatternFamily       = EOmniTraceTraceFamily::Forward;
    ForwardFanRequest.ForwardPattern      = EOmniTraceForwardPattern::MultiSpread;
    ForwardFanRequest.Shape               = EOmniTraceShape::Line;
    ForwardFanRequest.RayCount            = DefaultRayCount;
    ForwardFanRequest.MaxDistance         = DefaultMaxDistance;
    ForwardFanRequest.SpreadAngleDegrees  = 60.0f;
    ForwardFanRequest.DebugOptions.bEnableDebug = true;
    ForwardFanRequest.DebugOptions.Color        = FLinearColor::Green;
    ForwardFanRequest.DebugOptions.Lifetime     = 2.5f;
    ForwardFanRequest.DebugOptions.Thickness    = 1.5f;

    // Target arc – copy forward and adjust family / angle.
    TargetArcRequest = ForwardFanRequest;
    TargetArcRequest.PatternFamily      = EOmniTraceTraceFamily::Target;
    TargetArcRequest.ArcAngleDegrees    = 90.0f;

    // Orbit ring – copy forward and adjust family / radius / ray count.
    OrbitRequest = ForwardFanRequest;
    OrbitRequest.PatternFamily          = EOmniTraceTraceFamily::Orbit;
    OrbitRequest.OrbitRadius            = 500.0f;
    OrbitRequest.RayCount               = 24;

    // Radial 3D burst – copy forward and adjust family / ray count.
    RadialBurstRequest = ForwardFanRequest;
    RadialBurstRequest.PatternFamily    = EOmniTraceTraceFamily::Radial3D;
    RadialBurstRequest.RayCount         = 64;

    // Pattern config defaults to a simple line; designer can switch in Details.
    PatternConfig.PatternFamily         = EOmniTracePatternFamily::Line;

    // Pattern trace template starts as a copy of forward fan.
    PatternTraceTemplate = ForwardFanRequest;
}

FTransform AOmniTraceTraceLabActor::GetOriginTransform() const
{
    if (bUseActorTransformAsOrigin)
    {
        return GetActorTransform();
    }

    return OriginOverride;
}

void AOmniTraceTraceLabActor::ApplyDebugDefaults(FOmniTraceRequest& Request) const
{
    if (Request.RayCount <= 0)
    {
        Request.RayCount = DefaultRayCount;
    }

    if (Request.MaxDistance <= 0.0f)
    {
        Request.MaxDistance = DefaultMaxDistance;
    }

    Request.DebugOptions.bEnableDebug = true;

    if (Request.DebugOptions.Lifetime <= 0.0f)
    {
        Request.DebugOptions.Lifetime = 2.5f;
    }

    if (Request.DebugOptions.Thickness <= 0.0f)
    {
        Request.DebugOptions.Thickness = 1.5f;
    }
}

void AOmniTraceTraceLabActor::RunSinglePattern(const FOmniTraceRequest& InRequest)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FOmniTraceRequest Request = InRequest;
    ApplyDebugDefaults(Request);

    // Force the origin to this actor / override, and clear origin actor/component.
    const FTransform Origin = GetOriginTransform();
    Request.OriginActor          = nullptr;
    Request.OriginComponent      = nullptr;
    Request.OriginLocationOverride = Origin.GetLocation();
    Request.OriginRotationOverride = Origin.Rotator();

    // We ignore TargetActor here; you can still use TargetLocationOverride manually.
    FOmniTracePatternResult Result = UOmniTraceBlueprintLibrary::OmniTrace_Pattern(this, Request);
    (void)Result; // Debug is driven by Request.DebugOptions.
}

#if WITH_EDITOR

void AOmniTraceTraceLabActor::RunForwardFanTest()
{
    RunSinglePattern(ForwardFanRequest);
}

void AOmniTraceTraceLabActor::RunTargetArcTest()
{
    RunSinglePattern(TargetArcRequest);
}

void AOmniTraceTraceLabActor::RunOrbitRingTest()
{
    RunSinglePattern(OrbitRequest);
}

void AOmniTraceTraceLabActor::RunRadialBurstTest()
{
    RunSinglePattern(RadialBurstRequest);
}

void AOmniTraceTraceLabActor::RunPatternTraceTest()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FTransform Origin = GetOriginTransform();

    FOmniTraceRequest Request = PatternTraceTemplate;
    ApplyDebugDefaults(Request);

    FOmniTracePatternResult Result;
    UOmniTraceBlueprintLibrary::OmniTrace_TraceAlongPatternConfig(
        this,
        Origin,
        PatternConfig,
        PatternNumPoints,
        PatternScale,
        Request,
        Result);

    (void)Result;
}

#endif // WITH_EDITOR
