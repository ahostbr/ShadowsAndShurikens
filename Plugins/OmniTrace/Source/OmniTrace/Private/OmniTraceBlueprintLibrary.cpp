// Copyright (c) 2025 USP45Master. All Rights Reserved.

#include "OmniTraceBlueprintLibrary.h"

#include "OmniTracePatternPreset.h"
#include "OmniTracePresetLibrary.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

//////////////////////////////////////////////////////////////////////////
// Internal helpers & builtin preset registry
//////////////////////////////////////////////////////////////////////////

namespace
{
    /** Resolve world from a context object. */
    UWorld* GetWorldFromContext(UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return nullptr;
        }

        if (UWorld* World = WorldContextObject->GetWorld())
        {
            return World;
        }

        if (AActor* AsActor = Cast<AActor>(WorldContextObject))
        {
            return AsActor->GetWorld();
        }

        return nullptr;
    }

    /** Compute origin & forward vector from the request (actor/component/overrides). */
    void ResolveOriginAndForward(const FOmniTraceRequest& Request, FVector& OutOrigin, FVector& OutForward)
    {
        // Origin component has priority.
        if (Request.OriginComponent.IsValid())
        {
            OutOrigin  = Request.OriginComponent->GetComponentLocation();
            OutForward = Request.OriginComponent->GetForwardVector();
            return;
        }

        // Then origin actor.
        if (Request.OriginActor.IsValid())
        {
            OutOrigin  = Request.OriginActor->GetActorLocation();
            OutForward = Request.OriginActor->GetActorForwardVector();
            return;
        }

        // Fallback to overrides.
        OutOrigin  = Request.OriginLocationOverride;
        OutForward = Request.OriginRotationOverride.Vector();

        if (OutForward.IsNearlyZero())
        {
            OutForward = FVector::ForwardVector;
        }
    }

    /** Resolve target location from request. */
    FVector ResolveTargetLocation(const FOmniTraceRequest& Request)
    {
        if (Request.TargetActor.IsValid())
        {
            return Request.TargetActor->GetActorLocation();
        }
        return Request.TargetLocationOverride;
    }

    /** Utility: perform a single trace based on shape. */
    bool PerformSingleTrace(
        UWorld* World,
        const FOmniTraceRequest& Request,
        const FVector& Start,
        const FVector& End,
        FHitResult& OutHit)
    {
        if (!World)
        {
            return false;
        }

        FCollisionQueryParams Params(SCENE_QUERY_STAT(OmniTrace), Request.bTraceComplex);
        Params.bReturnPhysicalMaterial = false;

        // Always ignore the origin actor / owner of origin component
        AActor* OriginToIgnore = nullptr;

        if (Request.OriginComponent.IsValid())
        {
            OriginToIgnore = Request.OriginComponent->GetOwner();
        }
        else if (Request.OriginActor.IsValid())
        {
            OriginToIgnore = Request.OriginActor.Get();
        }

        if (IsValid(OriginToIgnore))
        {
            Params.AddIgnoredActor(OriginToIgnore);
        }

        // Also ignore any explicitly provided actors
        for (AActor* IgnoredActor : Request.ActorsToIgnore)
        {
            if (IsValid(IgnoredActor))
            {
                Params.AddIgnoredActor(IgnoredActor);
            }
        }

        const ECollisionChannel Channel = Request.TraceChannel;

        switch (Request.Shape)
        {
        case EOmniTraceShape::Line:
            return World->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);

        case EOmniTraceShape::SphereSweep:
        {
            const float Radius = FMath::Max(Request.ShapeRadius, KINDA_SMALL_NUMBER);
            FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
            return World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, Channel, Shape, Params);
        }

        case EOmniTraceShape::BoxSweep:
        {
            const FVector HalfExtents = Request.BoxHalfExtents.IsNearlyZero()
                ? FVector(30.0f, 30.0f, 60.0f)
                : Request.BoxHalfExtents;
            FCollisionShape Shape = FCollisionShape::MakeBox(HalfExtents);
            return World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, Channel, Shape, Params);
        }

        case EOmniTraceShape::CapsuleSweep:
        {
            const float Radius = FMath::Max(Request.ShapeRadius, KINDA_SMALL_NUMBER);
            const float HalfHeight = FMath::Max(Request.CapsuleHalfHeight, Radius);
            FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
            return World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, Channel, Shape, Params);
        }

        default:
            break;
        }

        return false;
    }
    /** Utility: debug draw for a ray. */
    void DrawDebugForRay(
        UWorld* World,
        const FOmniTraceRequest& Request,
        const FVector& Start,
        const FVector& End,
        bool bHit)
    {
        if (!World || !Request.DebugOptions.bEnableDebug)
        {
            return;
        }

        const FColor Color = Request.DebugOptions.Color.ToFColor(true);
        const float Lifetime  = Request.DebugOptions.Lifetime;
        const float Thickness = Request.DebugOptions.Thickness;

        DrawDebugLine(World, Start, End, Color, false, Lifetime, 0, Thickness);

        if (bHit)
        {
            DrawDebugPoint(World, End, 8.0f, Color, false, Lifetime);
        }
    }

    }

//////////////////////////////////////////////////////////////////////////
// Main pattern implementation
//////////////////////////////////////////////////////////////////////////

FOmniTracePatternResult UOmniTraceBlueprintLibrary::OmniTrace_Pattern(
    UObject* WorldContextObject,
    const FOmniTraceRequest& Request)
{
    FOmniTracePatternResult Result;

    UWorld* World = GetWorldFromContext(WorldContextObject);
    if (!World || Request.RayCount <= 0 || Request.MaxDistance <= 0.0f)
    {
        return Result;
    }

    FVector Origin, Forward;
    ResolveOriginAndForward(Request, Origin, Forward);

    const FVector TargetLocation = ResolveTargetLocation(Request);

    const int32 NumRays = FMath::Max(1, Request.RayCount);
    Result.Rays.Reserve(NumRays);

    auto AddRay = [&](int32 RayIndex, const FVector& Dir)
    {
        FVector Start = Origin;
        FVector End   = Origin + Dir.GetSafeNormal() * Request.MaxDistance;

        FHitResult Hit;
        const bool bHit = PerformSingleTrace(World, Request, Start, End, Hit);

        DrawDebugForRay(World, Request, Start, bHit ? Hit.Location : End, bHit);

        FOmniTraceSingleRayResult Ray;
        Ray.RayIndex = RayIndex;
        Ray.Start    = Start;
        Ray.End      = bHit ? Hit.Location : End;
        Ray.bHit     = bHit;
        Ray.Hit      = MakeOmniTraceHitResult(Hit);

        Result.Rays.Add(Ray);
        Result.TotalRays++;

        if (bHit)
        {
            if (!Result.bAnyHit)
            {
                Result.bAnyHit          = true;
                Result.FirstBlockingHit = MakeOmniTraceHitResult(Hit);
                Result.NearestHit       = Result.FirstBlockingHit;
            }
            else
            {
                const float PrevDistSq = FVector::DistSquared(Origin, Result.NearestHit.Location);
                const float NewDistSq  = FVector::DistSquared(Origin, Hit.Location);
                if (NewDistSq < PrevDistSq)
                {
                    Result.NearestHit = MakeOmniTraceHitResult(Hit);
                }
            }
        }
    };

    // Generate directions based on pattern family.
    switch (Request.PatternFamily)
    {
    case EOmniTraceTraceFamily::Forward:
    {
        if (Request.ForwardPattern == EOmniTraceForwardPattern::SingleRay || NumRays == 1)
        {
            AddRay(0, Forward);
        }
        else
        {
            const float HalfAngle = Request.SpreadAngleDegrees * 0.5f;
            for (int32 Index = 0; Index < NumRays; ++Index)
            {
                const float Alpha = (NumRays == 1) ? 0.0f : (float(Index) / float(NumRays - 1));
                const float Angle = FMath::Lerp(-HalfAngle, HalfAngle, Alpha);
                const FRotator Rot(0.0f, Angle, 0.0f);
                const FVector Dir = Rot.RotateVector(Forward);
                AddRay(Index, Dir);
            }
        }
        break;
    }

    case EOmniTraceTraceFamily::Target:
    {
        const FVector ToTarget = (TargetLocation - Origin);
        if (ToTarget.IsNearlyZero())
        {
            AddRay(0, Forward);
            break;
        }

        FVector BasisForward = ToTarget.GetSafeNormal();
        FVector BasisRight, BasisUp;
        BasisForward.FindBestAxisVectors(BasisRight, BasisUp);

        const float ArcAngle = Request.ArcAngleDegrees;
        const float StartAngle = -ArcAngle * 0.5f;

        for (int32 Index = 0; Index < NumRays; ++Index)
        {
            const float Alpha = (NumRays == 1) ? 0.0f : (float(Index) / float(NumRays - 1));
            const float AngleDeg = StartAngle + ArcAngle * Alpha;
            const float AngleRad = FMath::DegreesToRadians(AngleDeg);

            const FVector Dir =
                BasisForward * FMath::Cos(AngleRad) +
                BasisRight   * FMath::Sin(AngleRad);

            AddRay(Index, Dir);
        }
        break;
    }

    case EOmniTraceTraceFamily::Orbit:
    {
        const FVector Center = TargetLocation.IsNearlyZero() ? Origin : TargetLocation;

        const float Radius = Request.OrbitRadius;
        const float StepAngle = 360.0f / float(NumRays);

        for (int32 Index = 0; Index < NumRays; ++Index)
        {
            const float AngleDeg = StepAngle * Index;
            const float AngleRad = FMath::DegreesToRadians(AngleDeg);

            const FVector RingOffset(
                FMath::Cos(AngleRad) * Radius,
                FMath::Sin(AngleRad) * Radius,
                0.0f);

            const FVector RayOrigin = Center + RingOffset;
            const FVector Dir = RingOffset.GetSafeNormal();

            FOmniTraceRequest RayReq = Request;
            RayReq.OriginLocationOverride = RayOrigin;
            RayReq.OriginActor   = nullptr;
            RayReq.OriginComponent = nullptr;

            FVector DummyOrigin, DummyForward;
            ResolveOriginAndForward(RayReq, DummyOrigin, DummyForward);

            // For orbit we fire outward from the ring.
            AddRay(Index, Dir);
        }
        break;
    }

    case EOmniTraceTraceFamily::Radial3D:
    {
        // Simple stratified sphere directions.
        const float PhiStep = PI * (3.0f - FMath::Sqrt(5.0f)); // golden angle approx
        for (int32 Index = 0; Index < NumRays; ++Index)
        {
            const float Y = 1.0f - (2.0f * float(Index) / float(NumRays - 1));
            const float Radius = FMath::Sqrt(FMath::Max(0.0f, 1.0f - Y * Y));
            const float Theta = PhiStep * Index;

            const float X = Radius * FMath::Cos(Theta);
            const float Z = Radius * FMath::Sin(Theta);

            const FVector Dir = FVector(X, Z, Y); // some distribution on a sphere
            AddRay(Index, Dir);
        }
        break;
    }

    default:
        break;
    }

    return Result;
}

void UOmniTraceBlueprintLibrary::OmniTrace_Pattern_Async(
    UObject* WorldContextObject,
    FLatentActionInfo LatentInfo,
    FOmniTracePatternResult& OutResult,
    const FOmniTraceRequest& Request)
{
    // For now, just perform synchronously and hand back the result.
    OutResult = OmniTrace_Pattern(WorldContextObject, Request);
}

FOmniTracePatternResult UOmniTraceBlueprintLibrary::OmniTrace_PatternFromPreset(
    UObject* WorldContextObject,
    UOmniTracePatternPreset* Preset,
    AActor* OriginActorOverride,
    USceneComponent* OriginComponentOverride,
    AActor* TargetActorOverride)
{
    FOmniTracePatternResult Result;

    if (!WorldContextObject || !Preset)
    {
        return Result;
    }

    FOmniTraceRequest Request = Preset->Request;

    if (Preset->bOverrideDebugColorFromFamily)
    {
        Request.DebugOptions.Color = Preset->FamilyColor;
    }

    if (OriginActorOverride)
    {
        Request.OriginActor = OriginActorOverride;
    }

    if (OriginComponentOverride)
    {
        Request.OriginComponent = OriginComponentOverride;
    }

    if (TargetActorOverride)
    {
        Request.TargetActor = TargetActorOverride;
    }

    return OmniTrace_Pattern(WorldContextObject, Request);
}

void UOmniTraceBlueprintLibrary::OmniTrace_PatternFromPreset_Async(
    UObject* WorldContextObject,
    FLatentActionInfo LatentInfo,
    FOmniTracePatternResult& OutResult,
    UOmniTracePatternPreset* Preset,
    AActor* OriginActorOverride,
    USceneComponent* OriginComponentOverride,
    AActor* TargetActorOverride)
{
    if (!WorldContextObject || !Preset)
    {
        return;
    }

    FOmniTraceRequest Request = Preset->Request;

    if (Preset->bOverrideDebugColorFromFamily)
    {
        Request.DebugOptions.Color = Preset->FamilyColor;
    }

    if (OriginActorOverride)
    {
        Request.OriginActor = OriginActorOverride;
    }

    if (OriginComponentOverride)
    {
        Request.OriginComponent = OriginComponentOverride;
    }

    if (TargetActorOverride)
    {
        Request.TargetActor = TargetActorOverride;
    }

    OmniTrace_Pattern_Async(WorldContextObject, LatentInfo, OutResult, Request);
}

FOmniTracePatternResult UOmniTraceBlueprintLibrary::OmniTrace_PatternFromLibrary(
    UObject* WorldContextObject,
    UOmniTracePatternLibrary* Library,
    FName PresetId,
    AActor* OriginActorOverride,
    USceneComponent* OriginComponentOverride,
    AActor* TargetActorOverride)
{
    FOmniTracePatternResult Result;

    if (!WorldContextObject || !Library)
    {
        return Result;
    }

    UOmniTracePatternPreset* Preset = Library->FindPresetById(PresetId);
    if (!Preset)
    {
        return Result;
    }

    return OmniTrace_PatternFromPreset(
        WorldContextObject,
        Preset,
        OriginActorOverride,
        OriginComponentOverride,
        TargetActorOverride);
}

void UOmniTraceBlueprintLibrary::OmniTrace_PatternFromLibrary_Async(
    UObject* WorldContextObject,
    FLatentActionInfo LatentInfo,
    FOmniTracePatternResult& OutResult,
    UOmniTracePatternLibrary* Library,
    FName PresetId,
    AActor* OriginActorOverride,
    USceneComponent* OriginComponentOverride,
    AActor* TargetActorOverride)
{
    if (!WorldContextObject || !Library)
    {
        return;
    }

    UOmniTracePatternPreset* Preset = Library->FindPresetById(PresetId);
    if (!Preset)
    {
        return;
    }

    OmniTrace_PatternFromPreset_Async(
        WorldContextObject,
        LatentInfo,
        OutResult,
        Preset,
        OriginActorOverride,
        OriginComponentOverride,
        TargetActorOverride);
}

void UOmniTraceBlueprintLibrary::OmniTrace_GetLibraryPresetInfos(
    UOmniTracePatternLibrary* Library,
    TArray<FOmniTracePresetInfo>& OutInfos)
{
    OutInfos.Reset();

    if (!Library)
    {
        return;
    }

    for (const FOmniTracePatternLibraryEntry& Entry : Library->Entries)
    {
        if (!Entry.Preset)
        {
            continue;
        }

        FOmniTracePresetInfo Info;

        const FName EffectiveId = Entry.PresetId.IsNone()
            ? Entry.Preset->PresetId
            : Entry.PresetId;

        Info.PresetId    = EffectiveId;
        Info.DisplayName = Entry.Preset->DisplayName;
        Info.Description = Entry.Preset->Description;
        Info.Category    = Entry.Preset->Category;
        Info.FamilyColor = Entry.Preset->FamilyColor;

        OutInfos.Add(Info);
    }
}

void UOmniTraceBlueprintLibrary::OmniTrace_BuildPathFromPatternConfig(
    const FTransform& Origin,
    const FOmniTracePatternConfig& Config,
    int32 NumPoints,
    float Scale,
    TArray<FVector>& OutPoints)
{
    OutPoints.Reset();

    NumPoints = FMath::Max(1, NumPoints);
    if (Scale <= 0.0f)
    {
        Scale = 1.0f;
    }

    const FVector LocalForward = FVector::ForwardVector;
    const FVector LocalRight   = FVector::RightVector;
    const FVector LocalUp      = FVector::UpVector;

    auto AddLocalPoint = [&](const FVector& Local)
    {
        OutPoints.Add(Origin.TransformPosition(Local));
    };

    switch (Config.PatternFamily)
    {
    case EOmniTracePatternFamily::Line:
    {
        const float TotalLength = Scale;
        const float Step = (NumPoints > 1) ? (TotalLength / float(NumPoints - 1)) : 0.0f;

        for (int32 Index = 0; Index < NumPoints; ++Index)
        {
            const float Dist = Step * float(Index);
            AddLocalPoint(LocalForward * Dist);
        }
        break;
    }

    case EOmniTracePatternFamily::Arc:
    {
        // Simple 180° arc in local X/Y plane.
        const float Radius = Scale;
        const float StartAngleDeg = -90.0f;
        const float EndAngleDeg   =  90.0f;

        for (int32 Index = 0; Index < NumPoints; ++Index)
        {
            const float Alpha = (NumPoints > 1) ? (float(Index) / float(NumPoints - 1)) : 0.0f;
            const float AngleDeg = FMath::Lerp(StartAngleDeg, EndAngleDeg, Alpha);
            const float AngleRad = FMath::DegreesToRadians(AngleDeg);

            const float X = FMath::Cos(AngleRad) * Radius;
            const float Y = FMath::Sin(AngleRad) * Radius;
            AddLocalPoint(LocalForward * X + LocalRight * Y);
        }
        break;
    }

    case EOmniTracePatternFamily::Orbit:
    {
        // Full 360° ring around origin in X/Y plane.
        const float Radius = Scale;
        const float StepAngleDeg = 360.0f / float(NumPoints);

        for (int32 Index = 0; Index < NumPoints; ++Index)
        {
            const float AngleDeg = StepAngleDeg * float(Index);
            const float AngleRad = FMath::DegreesToRadians(AngleDeg);

            const float X = FMath::Cos(AngleRad) * Radius;
            const float Y = FMath::Sin(AngleRad) * Radius;
            AddLocalPoint(LocalForward * X + LocalRight * Y);
        }
        break;
    }

    case EOmniTracePatternFamily::Grid:
    {
        // Simple square grid centered around origin in X/Y.
        const int32 Side = FMath::CeilToInt(FMath::Sqrt(float(NumPoints)));
        if (Side <= 0)
        {
            AddLocalPoint(FVector::ZeroVector);
            break;
        }

        const float Half = 0.5f * float(Side - 1);
        const float Step = (Side > 1) ? (Scale / float(Side - 1)) : 0.0f;

        int32 Count = 0;
        for (int32 Y = 0; Y < Side && Count < NumPoints; ++Y)
        {
            for (int32 X = 0; X < Side && Count < NumPoints; ++X)
            {
                const float OffsetX = (float(X) - Half) * Step;
                const float OffsetY = (float(Y) - Half) * Step;
                AddLocalPoint(LocalForward * OffsetX + LocalRight * OffsetY);
                ++Count;
            }
        }
        break;
    }

    case EOmniTracePatternFamily::NoiseWalk:
    {
        // Simple random walk in X/Y around origin.
        FVector Current = FVector::ZeroVector;
        const float Step = Scale / float(NumPoints);

        AddLocalPoint(Current);

        for (int32 Index = 1; Index < NumPoints; ++Index)
        {
            const float AngleRad = FMath::FRand() * 2.0f * PI;
            const FVector Delta =
                LocalForward * (FMath::Cos(AngleRad) * Step) +
                LocalRight   * (FMath::Sin(AngleRad) * Step);

            Current += Delta;
            AddLocalPoint(Current);
        }
        break;
    }

    case EOmniTracePatternFamily::Radial:
    {
        // Simple radial burst from origin.
        const float Radius = Scale;
        for (int32 Index = 0; Index < NumPoints; ++Index)
        {
            const float AngleDeg = (360.0f * float(Index)) / float(NumPoints);
            const float AngleRad = FMath::DegreesToRadians(AngleDeg);

            const float X = FMath::Cos(AngleRad) * Radius;
            const float Y = FMath::Sin(AngleRad) * Radius;
            AddLocalPoint(LocalForward * X + LocalRight * Y);
        }
        break;
    }

    case EOmniTracePatternFamily::Scatter:
    {
        // Scatter in a disc of radius Scale.
        for (int32 Index = 0; Index < NumPoints; ++Index)
        {
            const float AngleRad = FMath::FRand() * 2.0f * PI;
            const float Radius = Scale * FMath::Sqrt(FMath::FRand()); // uniform area

            const float X = FMath::Cos(AngleRad) * Radius;
            const float Y = FMath::Sin(AngleRad) * Radius;
            AddLocalPoint(LocalForward * X + LocalRight * Y);
        }
        break;
    }

    case EOmniTracePatternFamily::Spline:
    {
        // For now: treat as a simple line; spline-aware helpers can be added later.
        const float TotalLength = Scale;
        const float Step = (NumPoints > 1) ? (TotalLength / float(NumPoints - 1)) : 0.0f;

        for (int32 Index = 0; Index < NumPoints; ++Index)
        {
            const float Dist = Step * float(Index);
            AddLocalPoint(LocalForward * Dist);
        }
        break;
    }

    case EOmniTracePatternFamily::Volume:
    {
        // Fill a box volume of extent Scale in each axis.
        for (int32 Index = 0; Index < NumPoints; ++Index)
        {
            const float X = FMath::FRandRange(-Scale, Scale);
            const float Y = FMath::FRandRange(-Scale, Scale);
            const float Z = FMath::FRandRange(-Scale, Scale);

            AddLocalPoint(LocalForward * X + LocalRight * Y + LocalUp * Z);
        }
        break;
    }

    default:
        // Fallback: single point at origin.
        AddLocalPoint(FVector::ZeroVector);
        break;
    }
}
void UOmniTraceBlueprintLibrary::OmniTrace_TraceAlongPatternConfig(
    UObject* WorldContextObject,
    const FTransform& Origin,
    const FOmniTracePatternConfig& Config,
    int32 NumPoints,
    float Scale,
    const FOmniTraceRequest& TraceTemplate,
    FOmniTracePatternResult& OutResult)
{
    OutResult = FOmniTracePatternResult();

    UWorld* World = GetWorldFromContext(WorldContextObject);
    if (!World)
    {
        return;
    }

    // Build world-space points from the pattern config.
    TArray<FVector> Points;
    OmniTrace_BuildPathFromPatternConfig(Origin, Config, NumPoints, Scale, Points);

    if (Points.Num() == 0)
    {
        return;
    }

    const FVector OriginLocation = Origin.GetLocation();

    // Use the template as-is for trace parameters (channel, shape, debug, ignores...).
    const FOmniTraceRequest Request = TraceTemplate;

    OutResult.Rays.Reset();
    OutResult.TotalRays = 0;
    OutResult.bAnyHit   = false;
    OutResult.FirstBlockingHit = FOmniTraceHitResult();
    OutResult.NearestHit       = FOmniTraceHitResult();

    const int32 NumRays = Points.Num();
    OutResult.Rays.Reserve(NumRays);

    for (int32 Index = 0; Index < NumRays; ++Index)
    {
        const FVector& Target = Points[Index];

        FVector Start = OriginLocation;
        FVector End   = Target;

        // Respect MaxDistance if it is set to a positive value.
        if (Request.MaxDistance > 0.0f)
        {
            const FVector Dir   = (Target - OriginLocation);
            const float   Dist  = Dir.Size();
            if (Dist > Request.MaxDistance)
            {
                const FVector ClampedDir = Dir.GetSafeNormal();
                End = OriginLocation + ClampedDir * Request.MaxDistance;
            }
        }

        FHitResult Hit;
        const bool bHit = PerformSingleTrace(World, Request, Start, End, Hit);

        DrawDebugForRay(World, Request, Start, bHit ? Hit.Location : End, bHit);

        FOmniTraceSingleRayResult Ray;
        Ray.RayIndex = Index;
        Ray.Start    = Start;
        Ray.End      = bHit ? Hit.Location : End;
        Ray.bHit     = bHit;
        Ray.Hit      = MakeOmniTraceHitResult(Hit);

        OutResult.Rays.Add(Ray);
        OutResult.TotalRays++;

        if (bHit)
        {
            if (!OutResult.bAnyHit)
            {
                OutResult.bAnyHit          = true;
                OutResult.FirstBlockingHit = MakeOmniTraceHitResult(Hit);
                OutResult.NearestHit       = OutResult.FirstBlockingHit;
            }
            else
            {
                const float PrevDistSq = FVector::DistSquared(OriginLocation, OutResult.NearestHit.Location);
                const float NewDistSq  = FVector::DistSquared(OriginLocation, Hit.Location);
                if (NewDistSq < PrevDistSq)
                {
                    OutResult.NearestHit = MakeOmniTraceHitResult(Hit);
                }
            }
        }
    }
}
