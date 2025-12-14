#include "SOTS_InteractionTrace.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

#if __has_include("OmniTrace.h")
    #include "OmniTrace.h"
    #define SOTS_HAS_OMNITRACE 1
#else
    #define SOTS_HAS_OMNITRACE 0
#endif

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
        bool bDebug
    )
    {
        OutHits.Reset();
        if (!World) return false;

#if SOTS_HAS_OMNITRACE
        // TODO: Replace with the suite's OmniTrace sweep call signature.
        (void)bDebug;
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
        if (bDebug)
        {
            DrawDebugLine(World, Start, End, FColor::Green, false, 0.05f, 0, 0.5f);
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
        bool bDebug
    )
    {
        if (!World) return false;

#if SOTS_HAS_OMNITRACE
        // TODO: Replace with the suite's OmniTrace line trace call.
        (void)bDebug;
#endif

        FCollisionQueryParams Params(SCENE_QUERY_STAT(SOTS_InteractionLOS), false);
        for (const AActor* A : ActorsToIgnore)
        {
            if (A) Params.AddIgnoredActor(A);
        }

        const bool bBlocked = World->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bDebug)
        {
            DrawDebugLine(World, Start, End, FColor::Red, false, 0.05f, 0, 0.5f);
        }
#endif
        return bBlocked;
    }
}
