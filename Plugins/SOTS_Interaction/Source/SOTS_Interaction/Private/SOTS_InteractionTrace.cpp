#include "SOTS_InteractionTrace.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "DrawDebugHelpers.h"
#include "SOTS_InteractionLog.h"

#if __has_include("OmniTrace.h")
    #include "OmniTrace.h"
    #include "OmniTraceBlueprintLibrary.h"
    #include "OmniTraceTypes.h"
    #define SOTS_HAS_OMNITRACE 1
#else
    #define SOTS_HAS_OMNITRACE 0
#endif

namespace
{
    FHitResult MakeHitFromOmni(const FOmniTraceHitResult& OmniHit, const FVector& TraceStart, const FVector& TraceEnd)
    {
        FHitResult Hit;
        Hit.TraceStart = TraceStart;
        Hit.TraceEnd = TraceEnd;
        Hit.Location = OmniHit.Location;
        Hit.ImpactPoint = OmniHit.ImpactPoint;
        Hit.Normal = OmniHit.Normal;
        Hit.Distance = OmniHit.Distance;
        Hit.bBlockingHit = OmniHit.bBlockingHit;

        const double TraceLen = (TraceEnd - TraceStart).Size();
        Hit.Time = (TraceLen > KINDA_SMALL_NUMBER) ? FMath::Clamp(static_cast<float>(OmniHit.Distance / TraceLen), 0.f, 1.f) : 0.f;

        Hit.HitObjectHandle = FActorInstanceHandle(OmniHit.HitActor.Get());
        Hit.Component = OmniHit.HitComponent.Get();
        return Hit;
    }
}

namespace SOTSInteractionTrace
{
    bool SphereSweepMulti(
        UWorld* World,
        TArray<FHitResult>& OutHits,
        const FVector& Start,
        const FVector& End,
        float Radius,
        ECollisionChannel Channel,
        const TArray<const AActor*>& ActorsToIgnore,
        bool bForceLegacyTraces,
        bool& bOutUsedOmniTrace,
        bool bDebugDraw,
        bool bDebugLog
    )
    {
        OutHits.Reset();
        if (!World) return false;

        bOutUsedOmniTrace = false;

#if SOTS_HAS_OMNITRACE
        if (!bForceLegacyTraces)
        {
            FOmniTraceRequest Request;
            Request.PatternFamily = EOmniTraceTraceFamily::Forward;
            Request.ForwardPattern = EOmniTraceForwardPattern::SingleRay;
            Request.Shape = EOmniTraceShape::SphereSweep;
            Request.ShapeRadius = Radius;
            Request.TraceChannel = Channel;
            Request.MaxDistance = (End - Start).Size();
            Request.RayCount = 1;
            Request.SpreadAngleDegrees = 0.f;
            Request.OriginLocationOverride = Start;
            Request.OriginRotationOverride = (End - Start).Rotation();
            Request.TargetLocationOverride = End;
            Request.DebugOptions.bEnableDebug = bDebugDraw;
            Request.DebugOptions.Color = FLinearColor::Green;
            Request.DebugOptions.Lifetime = 0.05f;
            Request.DebugOptions.Thickness = 1.5f;

            for (const AActor* A : ActorsToIgnore)
            {
                if (A)
                {
                    Request.ActorsToIgnore.Add(const_cast<AActor*>(A));
                }
            }

            const FOmniTracePatternResult Result = UOmniTraceBlueprintLibrary::OmniTrace_Pattern(World, Request);

            if (Result.Rays.Num() > 0)
            {
                for (const FOmniTraceSingleRayResult& Ray : Result.Rays)
                {
                    if (!Ray.bHit)
                    {
                        continue;
                    }

                    OutHits.Add(MakeHitFromOmni(Ray.Hit, Ray.Start, Ray.End));
                }
            }

            if (OutHits.Num() > 1)
            {
                OutHits.Sort([](const FHitResult& A, const FHitResult& B)
                {
                    return A.Distance < B.Distance;
                });
            }

            bOutUsedOmniTrace = true;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bDebugLog)
            {
                const FString FirstName = OutHits.Num() > 0 && OutHits[0].GetActor() ? OutHits[0].GetActor()->GetName() : TEXT("None");
                UE_LOG(LogSOTSInteraction, Verbose, TEXT("[Trace] OmniTrace sweep hits=%d first=%s"), OutHits.Num(), *FirstName);
            }
#endif
            return OutHits.Num() > 0;
        }
#endif

        FCollisionQueryParams Params(SCENE_QUERY_STAT(SOTS_InteractionSweep), false);
        for (const AActor* A : ActorsToIgnore)
        {
            if (A) Params.AddIgnoredActor(A);
        }

        const bool bHit = World->SweepMultiByChannel(
            OutHits, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeSphere(Radius), Params
        );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bDebugDraw)
        {
            DrawDebugLine(World, Start, End, FColor::Green, false, 0.05f, 0, 0.5f);
        }

        if (bDebugLog)
        {
            const FString FirstName = bHit && OutHits.Num() > 0 && OutHits[0].GetActor() ? OutHits[0].GetActor()->GetName() : TEXT("None");
            UE_LOG(LogSOTSInteraction, Verbose, TEXT("[Trace] Legacy sweep hits=%d first=%s"), OutHits.Num(), *FirstName);
        }
#endif
        return bHit;
    }

    bool LineTraceBlocked(
        UWorld* World,
        FHitResult& OutHit,
        const FVector& Start,
        const FVector& End,
        ECollisionChannel Channel,
        const TArray<const AActor*>& ActorsToIgnore,
        bool bForceLegacyTraces,
        bool& bOutUsedOmniTrace,
        bool bDebugDraw,
        bool bDebugLog
    )
    {
        if (!World) return false;

        bOutUsedOmniTrace = false;

#if SOTS_HAS_OMNITRACE
        if (!bForceLegacyTraces)
        {
            FOmniTraceRequest Request;
            Request.PatternFamily = EOmniTraceTraceFamily::Forward;
            Request.ForwardPattern = EOmniTraceForwardPattern::SingleRay;
            Request.Shape = EOmniTraceShape::Line;
            Request.TraceChannel = Channel;
            Request.MaxDistance = (End - Start).Size();
            Request.RayCount = 1;
            Request.SpreadAngleDegrees = 0.f;
            Request.OriginLocationOverride = Start;
            Request.OriginRotationOverride = (End - Start).Rotation();
            Request.TargetLocationOverride = End;
            Request.DebugOptions.bEnableDebug = bDebugDraw;
            Request.DebugOptions.Color = FLinearColor::Red;
            Request.DebugOptions.Lifetime = 0.05f;
            Request.DebugOptions.Thickness = 1.5f;

            for (const AActor* A : ActorsToIgnore)
            {
                if (A)
                {
                    Request.ActorsToIgnore.Add(const_cast<AActor*>(A));
                }
            }

            const FOmniTracePatternResult Result = UOmniTraceBlueprintLibrary::OmniTrace_Pattern(World, Request);

            if (Result.Rays.Num() > 0 && Result.Rays[0].bHit)
            {
                OutHit = MakeHitFromOmni(Result.Rays[0].Hit, Result.Rays[0].Start, Result.Rays[0].End);
                bOutUsedOmniTrace = true;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
                if (bDebugLog)
                {
                    const FString HitName = OutHit.GetActor() ? OutHit.GetActor()->GetName() : TEXT("None");
                    UE_LOG(LogSOTSInteraction, Verbose, TEXT("[Trace] OmniTrace LOS hit=%s"), *HitName);
                }
#endif
                return OutHit.bBlockingHit;
            }

            bOutUsedOmniTrace = true;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (bDebugLog)
            {
                UE_LOG(LogSOTSInteraction, Verbose, TEXT("[Trace] OmniTrace LOS no hit"));
            }
#endif
            return false;
        }
#endif

        FCollisionQueryParams Params(SCENE_QUERY_STAT(SOTS_InteractionLOS), false);
        for (const AActor* A : ActorsToIgnore)
        {
            if (A) Params.AddIgnoredActor(A);
        }

        const bool bBlocked = World->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bDebugDraw)
        {
            DrawDebugLine(World, Start, End, FColor::Red, false, 0.05f, 0, 0.5f);
        }

        if (bDebugLog)
        {
            const FString HitName = bBlocked && OutHit.GetActor() ? OutHit.GetActor()->GetName() : TEXT("None");
            UE_LOG(LogSOTSInteraction, Verbose, TEXT("[Trace] Legacy LOS blocked=%s"), bBlocked ? *HitName : TEXT("None"));
        }
#endif
        return bBlocked;
    }
}
