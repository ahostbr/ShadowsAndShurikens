#include "OmniTraceDebugSubsystem.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

void UOmniTraceDebugSubsystem::SetLastKEMTrace(const FSOTS_OmniTraceKEMDebugRecord& InRecord)
{
    LastKEMTrace = InRecord;
}

void UOmniTraceDebugSubsystem::OmniTrace_DrawLastKEMTrace()
{
#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
    return;
#endif

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogOmniTrace, Warning, TEXT("OmniTrace_DrawLastKEMTrace: no valid world."));
        return;
    }

    const FSOTS_OmniTraceKEMDebugRecord& Rec = LastKEMTrace;
    const float Lifetime = 5.f;

    DrawDebugSphere(World, Rec.InstigatorLocation, 10.f, 8, FColor::Green, false, Lifetime, 0, 1.5f);
    DrawDebugSphere(World, Rec.TargetLocation, 10.f, 8, FColor::Blue, false, Lifetime, 0, 1.5f);

    if (Rec.bHit)
    {
        DrawDebugSphere(World, Rec.HitLocation, 12.f, 8, FColor::Yellow, false, Lifetime, 0, 1.5f);
        DrawDebugDirectionalArrow(World, Rec.HitLocation,
            Rec.HitLocation + Rec.HitNormal * 50.f,
            8.f, FColor::Cyan, false, Lifetime, 0, 1.5f);
    }

    DrawDebugCoordinateSystem(World, Rec.FinalSpawnTransform.GetLocation(), Rec.FinalSpawnTransform.GetRotation().Rotator(),
        25.f, false, Lifetime, 0, 1.5f);

    UE_LOG(LogOmniTrace, Log, TEXT("OmniTrace_DrawLastKEMTrace: Preset=%d Family=%d Hit=%s"),
        static_cast<int32>(Rec.PresetId), static_cast<int32>(Rec.ExecutionFamily), Rec.bHit ? TEXT("true") : TEXT("false"));
}
