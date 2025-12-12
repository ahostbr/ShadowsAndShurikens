#include "SOTS_KEM_OmniTraceBridge.h"

#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "OmniTraceBlueprintLibrary.h"
#include "OmniTracePatternPreset.h"
#include "OmniTraceTypes.h"
#include "SOTS_KEM_ExecutionDefinition.h"
#include "SOTS_KEM_ManagerSubsystem.h"
#include "SOTS_OmniTraceKEMPresetLibrary.h"
#include "OmniTraceDebugSubsystem.h"
#include "OmniTraceDebugTypes.h"
#include "Subsystems/WorldSubsystem.h"

#if defined(GroundLeft)
#undef GroundLeft
#endif

#if defined(GroundRight)
#undef GroundRight
#endif

#if defined(CornerLeft)
#undef CornerLeft
#endif

#if defined(CornerRight)
#undef CornerRight
#endif

static bool KEM_TagStringContains(const FGameplayTag& Tag, const TCHAR* Substring)
{
    if (!Substring || !Tag.IsValid())
    {
        return false;
    }
    return Tag.ToString().Contains(Substring);
}

// Helper: compute a ground-relative transform around the target on the X/Y
// plane. LocalOffsetDir is interpreted as multipliers in the target's
// (Forward, Right) basis, e.g. (-1,0,0)=rear, (1,0,0)=front, (0,-1,0)=left.
static bool ComputeGroundRelativeHelperTransform(
    const AActor* TargetActor,
    const FVector& SampleLocation,
    const FVector& LocalOffsetDir,
    const UOmniTracePatternPreset* PatternPreset,
    float TraceDistance,
    FTransform& OutTransform)
{
    if (!TargetActor)
    {
        return false;
    }

    UWorld* World = TargetActor->GetWorld();
    if (!World)
    {
        return false;
    }

    const FTransform TargetXf = TargetActor->GetActorTransform();
    const FVector Forward = TargetXf.GetRotation().GetForwardVector().GetSafeNormal();
    const FVector Right   = TargetXf.GetRotation().GetRightVector().GetSafeNormal();

    FVector DesiredDir = Forward * LocalOffsetDir.X + Right * LocalOffsetDir.Y;
    if (!DesiredDir.Normalize())
    {
        DesiredDir = !Forward.IsNearlyZero() ? Forward : FVector::ForwardVector;
    }

    // Use OmniTrace to search along the desired direction for reasonable
    // ground, then fall back to a simple offset if nothing is hit.
    FOmniTraceRequest Request;
    if (PatternPreset)
    {
        Request = PatternPreset->Request;
    }
    else
    {
        Request.PatternFamily      = EOmniTraceTraceFamily::Forward;
        Request.ForwardPattern     = EOmniTraceForwardPattern::MultiSpread;
        Request.Shape              = EOmniTraceShape::Line;
        Request.TraceChannel       = ECC_Visibility;
        Request.bTraceComplex      = false;
        Request.MaxDistance        = TraceDistance > 0.f ? TraceDistance : 2000.f;
        Request.RayCount           = 11;
        Request.SpreadAngleDegrees = 45.0f;
    }

    Request.OriginActor.Reset();
    Request.OriginComponent.Reset();
    Request.OriginLocationOverride = SampleLocation;
    Request.OriginRotationOverride = DesiredDir.Rotation();
    Request.DebugOptions.bEnableDebug = false;

    if (TraceDistance > 0.f)
    {
        Request.MaxDistance = TraceDistance;
    }

    const FOmniTracePatternResult PatternResult =
        UOmniTraceBlueprintLibrary::OmniTrace_Pattern(World, Request);

    bool    bFoundCandidate = false;
    FVector BestLocation    = SampleLocation;
    float   BestDistSq      = TNumericLimits<float>::Max();

    for (const FOmniTraceSingleRayResult& Ray : PatternResult.Rays)
    {
        if (!Ray.bHit)
        {
            continue;
        }

        const FOmniTraceHitResult& Hit = Ray.Hit;

        // Prefer reasonably flat ground.
        if (Hit.Normal.Z < 0.5f)
        {
            continue;
        }

        const FVector Delta  = Hit.Location - SampleLocation;
        const float   DistSq = Delta.SizeSquared();
        if (DistSq <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const FVector ToHit     = Delta.GetSafeNormal();
        const float   Alignment = FVector::DotProduct(ToHit, DesiredDir);

        // Only accept hits that are at least roughly in the desired direction.
        if (Alignment < 0.25f)
        {
            continue;
        }

        if (!bFoundCandidate || DistSq < BestDistSq)
        {
            bFoundCandidate = true;
            BestDistSq      = DistSq;
            BestLocation    = Hit.Location;
        }
    }

    if (!bFoundCandidate)
    {
        if (PatternResult.bAnyHit && PatternResult.Rays.Num() > 0)
        {
            BestLocation = PatternResult.NearestHit.Location;
        }
        else
        {
            const float FallbackDistance = FMath::Min(TraceDistance * 0.5f, 200.0f);
            BestLocation = SampleLocation + DesiredDir * FallbackDistance;
        }
    }

    // Orientation: for pure rear offsets, preserve the historical behavior
    // of facing along -Forward; otherwise, face back toward the target.
    FVector FacingDir;
    if (LocalOffsetDir.X < 0.f && FMath::IsNearlyZero(LocalOffsetDir.Y))
    {
        FacingDir = -Forward;
    }
    else
    {
        FacingDir = (SampleLocation - BestLocation).GetSafeNormal();
        if (!FacingDir.Normalize())
        {
            FacingDir = -DesiredDir;
        }
    }

    const FRotator HelperRot = FacingDir.Rotation();
    OutTransform = FTransform(HelperRot, BestLocation);

    return true;
}

static bool ComputeVerticalHelperTransform(
    UWorld* World,
    const FVector& TargetLocation,
    bool bAbove,
    float TraceDistance,
    FTransform& OutTransform)
{
    if (!World)
    {
        return false;
    }

    const float StartOffset = bAbove ? 150.f : -150.f;
    const FVector Start = TargetLocation + FVector(0.f, 0.f, StartOffset);
    const FVector End = TargetLocation + FVector(0.f, 0.f, bAbove ? TraceDistance : -TraceDistance);

    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KEM_OmniTraceVertical), true);
    if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
    {
        FVector Facing = (TargetLocation - Hit.Location);
        if (!Facing.Normalize())
        {
            Facing = bAbove ? FVector::DownVector : FVector::UpVector;
        }
        OutTransform = FTransform(Facing.Rotation(), Hit.Location);
        return true;
    }

    const FVector Fallback = End;
    FVector Facing = (TargetLocation - Fallback).GetSafeNormal();
    if (!Facing.Normalize())
    {
        Facing = bAbove ? FVector::DownVector : FVector::UpVector;
    }

    OutTransform = FTransform(Facing.Rotation(), Fallback);
    return true;
}

static EOmniTraceKEMExecutionFamily ConvertExecutionFamily(ESOTS_KEM_ExecutionFamily Family)
{
#if defined(GroundLeft)
#undef GroundLeft
#endif

#if defined(GroundRight)
#undef GroundRight
#endif

#if defined(CornerLeft)
#undef CornerLeft
#endif

#if defined(CornerRight)
#undef CornerRight
#endif

    switch (Family)
    {
    case ESOTS_KEM_ExecutionFamily::GroundRear:
        return EOmniTraceKEMExecutionFamily::GroundRear;
    case ESOTS_KEM_ExecutionFamily::GroundFront:
        return EOmniTraceKEMExecutionFamily::GroundFront;
    case ESOTS_KEM_ExecutionFamily::GroundLeft:
        return EOmniTraceKEMExecutionFamily::GroundLeft;
    case ESOTS_KEM_ExecutionFamily::GroundRight:
        return EOmniTraceKEMExecutionFamily::GroundRight;
    case ESOTS_KEM_ExecutionFamily::CornerLeft:
        return EOmniTraceKEMExecutionFamily::CornerLeft;
    case ESOTS_KEM_ExecutionFamily::CornerRight:
        return EOmniTraceKEMExecutionFamily::CornerRight;
    case ESOTS_KEM_ExecutionFamily::VerticalAbove:
        return EOmniTraceKEMExecutionFamily::VerticalAbove;
    case ESOTS_KEM_ExecutionFamily::VerticalBelow:
        return EOmniTraceKEMExecutionFamily::VerticalBelow;
    case ESOTS_KEM_ExecutionFamily::Special:
        return EOmniTraceKEMExecutionFamily::Cinematic;
    default:
        return EOmniTraceKEMExecutionFamily::Unknown;
    }
}

void FSOTS_KEM_OmniTraceBridge::ConfigureForPresetEntry(const FSOTS_OmniTraceKEMPresetEntry* Entry)
{
    PositionConfig = FPositionPatternConfig();

    if (!Entry)
    {
        return;
    }

    UOmniTracePatternPreset* Preset = nullptr;
    if (Entry->PatternPreset.IsValid())
    {
        Preset = Entry->PatternPreset.LoadSynchronous();
    }

    const FName DefaultLabel = Preset ? Preset->PresetId : TEXT("SOTS.KEM.Pattern.Ground.Unknown");
    PositionConfig.PatternLabel = Entry->PatternLabel.IsNone() ? DefaultLabel : Entry->PatternLabel;
    PositionConfig.PatternPreset = Preset;
    PositionConfig.PatternTraceDistance = Entry->TraceDistance > 0.f ? Entry->TraceDistance : 2000.f;
    const FVector LocalDir = Entry->LocalDirection.GetSafeNormal();
    PositionConfig.LocalDirection = LocalDir.IsNearlyZero() ? FVector::ForwardVector : LocalDir;
    PositionConfig.bUseGroundRelative = Entry->bUseGroundRelative;
    PositionConfig.bUseVertical = Entry->bUseVertical;
    PositionConfig.bVerticalAbove = Entry->bVerticalAbove;
    PositionConfig.bVerticalBelow = Entry->bUseVertical ? !Entry->bVerticalAbove : false;
    PositionConfig.VerticalTraceDistance = Entry->VerticalTraceDistance > 0.f ? Entry->VerticalTraceDistance : 900.f;
    PositionConfig.bHasKnownMapping = true;
}

void FSOTS_KEM_OmniTraceBridge::ConfigureForPositionTag(const FGameplayTag& PositionTag)
{
    PositionConfig = FPositionPatternConfig();

    if (!PositionTag.IsValid())
    {
        return;
    }

    const FName TagName = PositionTag.GetTagName();
    auto ActivateGround = [&](const FVector& Direction, const TCHAR* Pattern)
    {
        PositionConfig.bUseGroundRelative = true;
        PositionConfig.LocalDirection = Direction.GetSafeNormal();
        PositionConfig.PatternLabel = Pattern;
        PositionConfig.bHasKnownMapping = true;
    };

    auto ActivateVertical = [&](bool bAbove, const TCHAR* Pattern, float Distance)
    {
        PositionConfig.bUseVertical = true;
        PositionConfig.bVerticalAbove = bAbove;
        PositionConfig.bVerticalBelow = !bAbove;
        PositionConfig.PatternLabel = Pattern;
        PositionConfig.VerticalTraceDistance = Distance;
        PositionConfig.bHasKnownMapping = true;
    };

    auto IsTag = [&](const TCHAR* Candidate)
    {
        return TagName == FName(Candidate);
    };

    if (IsTag(TEXT("SOTS.KEM.Position.Ground.Rear")) || IsTag(TEXT("SOTS.KEM.Position.Rear")))
    {
        ActivateGround(FVector(-1.f, 0.f, 0.f), TEXT("SOTS.KEM.Pattern.Ground.Rear"));
    }
    else if (IsTag(TEXT("SOTS.KEM.Position.Ground.Front")) || IsTag(TEXT("SOTS.KEM.Position.Front")))
    {
        ActivateGround(FVector(1.f, 0.f, 0.f), TEXT("SOTS.KEM.Pattern.Ground.Front"));
    }
    else if (IsTag(TEXT("SOTS.KEM.Position.Ground.Left")) || IsTag(TEXT("SOTS.KEM.Position.Left")))
    {
        ActivateGround(FVector(0.f, -1.f, 0.f), TEXT("SOTS.KEM.Pattern.Ground.Left"));
    }
    else if (IsTag(TEXT("SOTS.KEM.Position.Ground.Right")) || IsTag(TEXT("SOTS.KEM.Position.Right")))
    {
        ActivateGround(FVector(0.f, 1.f, 0.f), TEXT("SOTS.KEM.Pattern.Ground.Right"));
    }
    else if (IsTag(TEXT("SOTS.KEM.Position.Corner.Left")))
    {
        ActivateGround(FVector(-0.8f, -1.f, 0.f), TEXT("SOTS.KEM.Pattern.Corner.Left"));
    }
    else if (IsTag(TEXT("SOTS.KEM.Position.Corner.Right")))
    {
        ActivateGround(FVector(-0.8f, 1.f, 0.f), TEXT("SOTS.KEM.Pattern.Corner.Right"));
    }
    else if (IsTag(TEXT("SOTS.KEM.Position.Corner")))
    {
        ActivateGround(FVector(-0.8f, -0.8f, 0.f), TEXT("SOTS.KEM.Pattern.Corner.Left"));
    }
    else if (IsTag(TEXT("SOTS.KEM.Position.Vertical.Above")) || IsTag(TEXT("SOTS.KEM.Position.Vertical")))
    {
        ActivateVertical(true, TEXT("SOTS.KEM.Pattern.Vertical.Above"), 1000.f);
    }
    else if (IsTag(TEXT("SOTS.KEM.Position.Vertical.Below")))
    {
        ActivateVertical(false, TEXT("SOTS.KEM.Pattern.Vertical.Below"), 1000.f);
    }
}

bool FSOTS_KEM_OmniTraceBridge::ComputeWarpForSpawnExecution(
    const FSOTS_KEM_SpawnActorConfig& SpawnConfig,
    const USOTS_KEM_ExecutionDefinition* Definition,
    const FSOTS_ExecutionContext& Context,
    FTransform& InOutSpawnTransform,
    FSOTS_KEM_OmniTraceWarpResult& OutResult)
{
    OutResult = FSOTS_KEM_OmniTraceWarpResult();

    if (!SpawnConfig.bUseOmniTraceForWarp)
    {
        return false;
    }

    if (!Context.Target.IsValid())
    {
        return false;
    }

    AActor* TargetActor = Context.Target.Get();
    if (!TargetActor)
    {
        return false;
    }

    UWorld* World = TargetActor->GetWorld();
    if (!World)
    {
        return false;
    }

    FString PatternLabel = SpawnConfig.OmniTracePatternTag.ToString();
    const FVector TargetLocation = Context.TargetLocation;
    FTransform HelperXf;
    bool bFoundHelper = false;

    if (PositionConfig.bHasKnownMapping)
    {
        PatternLabel = PositionConfig.PatternLabel.ToString();
        if (PositionConfig.bUseVertical)
        {
            bFoundHelper = ComputeVerticalHelperTransform(World, TargetLocation, PositionConfig.bVerticalAbove, PositionConfig.VerticalTraceDistance, HelperXf);
        }
        else if (PositionConfig.bUseGroundRelative)
        {
            bFoundHelper = ComputeGroundRelativeHelperTransform(
                TargetActor,
                TargetLocation,
                PositionConfig.LocalDirection,
                PositionConfig.PatternPreset,
                PositionConfig.PatternTraceDistance,
                HelperXf);
        }
    }

    const TCHAR* LegacyPatternLabel = TEXT("Ground.Unknown.Single");
    FVector LegacyLocalDir = FVector::ZeroVector;

    if (!bFoundHelper)
    {
        const bool bIsGroundRearSingle =
            KEM_TagStringContains(SpawnConfig.OmniTracePatternTag, TEXT("Ground.Rear.Single")) ||
            KEM_TagStringContains(SpawnConfig.OmniTracePatternTag, TEXT("Rear.Single"));

        const bool bIsGroundFrontSingle =
            KEM_TagStringContains(SpawnConfig.OmniTracePatternTag, TEXT("Ground.Front.Single")) ||
            KEM_TagStringContains(SpawnConfig.OmniTracePatternTag, TEXT("Front.Single"));

        const bool bIsGroundLeftSingle =
            KEM_TagStringContains(SpawnConfig.OmniTracePatternTag, TEXT("Ground.Left.Single")) ||
            KEM_TagStringContains(SpawnConfig.OmniTracePatternTag, TEXT("Left.Single"));

        const bool bIsGroundRightSingle =
            KEM_TagStringContains(SpawnConfig.OmniTracePatternTag, TEXT("Ground.Right.Single")) ||
            KEM_TagStringContains(SpawnConfig.OmniTracePatternTag, TEXT("Right.Single"));

        if (bIsGroundRearSingle)
        {
            LegacyLocalDir = FVector(-1.f, 0.f, 0.f);
            LegacyPatternLabel = TEXT("Ground.Rear.Single");
        }
        else if (bIsGroundFrontSingle)
        {
            LegacyLocalDir = FVector(1.f, 0.f, 0.f);
            LegacyPatternLabel = TEXT("Ground.Front.Single");
        }
        else if (bIsGroundLeftSingle)
        {
            LegacyLocalDir = FVector(0.f, -1.f, 0.f);
            LegacyPatternLabel = TEXT("Ground.Left.Single");
        }
        else if (bIsGroundRightSingle)
        {
            LegacyLocalDir = FVector(0.f, 1.f, 0.f);
            LegacyPatternLabel = TEXT("Ground.Right.Single");
        }

        if (LegacyLocalDir.IsNearlyZero())
        {
            return false;
        }

        bFoundHelper = ComputeGroundRelativeHelperTransform(TargetActor, TargetLocation, LegacyLocalDir, nullptr, 2000.0f, HelperXf);

        if (!bFoundHelper)
        {
            UE_LOG(LogSOTS_KEM, Warning,
                TEXT("[KEM][OmniTrace] %s: failed to compute ground-relative helper transform for pattern tag %s"),
                LegacyPatternLabel,
                *SpawnConfig.OmniTracePatternTag.ToString());
            return false;
        }

        PatternLabel = LegacyPatternLabel;
    }

    if (!bFoundHelper)
    {
        return false;
    }

    InOutSpawnTransform            = HelperXf;
    OutResult.bHasHelperSpawn      = true;
    OutResult.HelperSpawnTransform = HelperXf;

    FName RuntimeTargetName = NAME_None;
    if (Definition && Definition->WarpPoints.Num() > 0)
    {
        RuntimeTargetName = Definition->WarpPoints[0].WarpTargetName;
    }

    OutResult.WarpTargets.Empty();
    if (RuntimeTargetName != NAME_None)
    {
        FSOTS_KEM_WarpRuntimeTarget RuntimeTarget;
        RuntimeTarget.TargetName      = RuntimeTargetName;
        RuntimeTarget.TargetTransform = HelperXf;
        OutResult.WarpTargets.Add(RuntimeTarget);
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (World)
    {
        const float LifeTime = 5.0f;
        const float Radius   = 15.0f;
        const FVector Loc    = HelperXf.GetLocation();
        const FVector Dir    = HelperXf.GetRotation().GetForwardVector();

        DrawDebugSphere(World, Loc, Radius, 12, FColor::Purple, false, LifeTime, 0, 1.5f);
        DrawDebugDirectionalArrow(
            World,
            Loc,
            Loc + Dir * 75.0f,
            10.0f,
            FColor::Cyan,
            false,
            LifeTime,
            0,
            1.5f);
    }
#endif

    if (IsRunningDedicatedServer())
    {
        return true;
    }

    if (World)
    {
        UE_LOG(LogSOTS_KEM, Verbose,
            TEXT("[KEM][OmniTrace] %s: WarpName=%s HelperLoc=%s NumTargets=%d"),
            *PatternLabel,
            *RuntimeTargetName.ToString(),
            *HelperXf.GetLocation().ToString(),
            OutResult.WarpTargets.Num());
    }

    FSOTS_OmniTraceKEMDebugRecord DebugRecord;
    DebugRecord.PresetId = SpawnConfig.OmniTracePreset;
    DebugRecord.PositionTag = Definition ? Definition->PositionTag : SpawnConfig.OmniTracePatternTag;
    DebugRecord.ExecutionFamily = Definition
        ? ConvertExecutionFamily(Definition->ExecutionFamily)
        : EOmniTraceKEMExecutionFamily::Unknown;
    DebugRecord.InstigatorLocation = Context.InstigatorLocation;
    DebugRecord.TargetLocation = Context.TargetLocation;
    DebugRecord.bHit = OutResult.bHasHelperSpawn;
    DebugRecord.HitLocation = OutResult.HelperSpawnTransform.GetLocation();
    DebugRecord.HitNormal = OutResult.HelperSpawnTransform.GetRotation().GetForwardVector();
    DebugRecord.FinalSpawnTransform = OutResult.HelperSpawnTransform;

    UWorld* TraceWorld = TargetActor->GetWorld();
    if (!TraceWorld && Context.Instigator.IsValid())
    {
        TraceWorld = Context.Instigator->GetWorld();
    }

    if (TraceWorld)
    {
        if (UOmniTraceDebugSubsystem* DebugSubsystem = TraceWorld->GetSubsystem<UOmniTraceDebugSubsystem>())
        {
            DebugSubsystem->SetLastKEMTrace(DebugRecord);
        }
    }

    return true;
}
