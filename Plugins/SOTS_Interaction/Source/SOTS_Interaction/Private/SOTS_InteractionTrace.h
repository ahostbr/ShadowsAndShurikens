#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

class UWorld;
class AActor;

namespace SOTSInteractionTrace
{
    /** Sweep sphere from Start to End. Returns true if any hits. */
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
        bool bDebugDraw = false,
        bool bDebugLog = false
    );

    /** LOS line trace. Returns true if blocked (hit something). */
    bool LineTraceBlocked(
        UWorld* World,
        FHitResult& OutHit,
        const FVector& Start,
        const FVector& End,
        ECollisionChannel Channel,
        const TArray<const AActor*>& ActorsToIgnore,
        bool bForceLegacyTraces,
        bool& bOutUsedOmniTrace,
        bool bDebugDraw = false,
        bool bDebugLog = false
    );
}
