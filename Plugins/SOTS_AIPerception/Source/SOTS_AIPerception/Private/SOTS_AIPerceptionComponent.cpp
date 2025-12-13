#include "SOTS_AIPerceptionComponent.h"

#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "SOTS_AIPerceptionConfig.h"
#include "SOTS_AIPerceptionSubsystem.h"
#include "SOTS_AIPerceptionTypes.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_TagAccessHelpers.h"
#include "SOTS_GlobalStealthTypes.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_AIPerceptionTelemetry, Log, Verbose);

namespace SOTS_AIPerception_BBKeys
{
    static const FName TargetActor(TEXT("BB_SOTS_TargetActor"));
    static const FName HasLOSToTarget(TEXT("BB_SOTS_HasLOSToTarget"));
    static const FName LastKnownTargetLocation(TEXT("BB_SOTS_LastKnownTargetLocation"));
    static const FName Awareness(TEXT("BB_SOTS_Awareness"));
    static const FName PerceptionState(TEXT("BB_SOTS_PerceptionState"));
}

USOTS_AIPerceptionComponent::USOTS_AIPerceptionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    PerceptionUpdateInterval = 0.2f;
    CurrentState = ESOTS_PerceptionState::Unaware;
    bPerceptionSuppressed = false;
    StealthCurveCached = nullptr;
    CurrentSuspicion = 0.0f;
    PreviousSuspicion = 0.0f;
}

void USOTS_AIPerceptionComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UWorld* World = GetWorld())
    {
        if (USOTS_AIPerceptionSubsystem* Subsys = World->GetSubsystem<USOTS_AIPerceptionSubsystem>())
        {
            Subsys->RegisterPerceptionComponent(this);
        }

        if (PerceptionUpdateInterval > 0.f)
        {
            World->GetTimerManager().SetTimer(
                PerceptionTimerHandle,
                this,
                &USOTS_AIPerceptionComponent::UpdatePerception,
                PerceptionUpdateInterval,
                true);
        }

        if (PerceptionConfig && PerceptionConfig->ShadowCheckInterval > 0.0f)
        {
            const double Now = World->GetTimeSeconds();
            NextShadowCheckTimeSeconds = Now + FMath::FRandRange(0.0f, PerceptionConfig->ShadowCheckInterval);
        }
    }

    RefreshWatchedTargets();
}

void USOTS_AIPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PerceptionTimerHandle);
        World->GetTimerManager().ClearTimer(SuppressionTimerHandle);

        if (USOTS_AIPerceptionSubsystem* Subsys = World->GetSubsystem<USOTS_AIPerceptionSubsystem>())
        {
            Subsys->UnregisterPerceptionComponent(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

float USOTS_AIPerceptionComponent::GetAwarenessForTarget(AActor* Target) const
{
    if (!Target)
    {
        return 0.f;
    }

    if (const FSOTS_PerceivedTargetState* State = TargetStates.Find(Target))
    {
        return State->Awareness;
    }

    return 0.f;
}

bool USOTS_AIPerceptionComponent::HasLineOfSightToTarget(AActor* Target) const
{
    if (!Target)
    {
        return false;
    }

    if (const FSOTS_PerceivedTargetState* State = TargetStates.Find(Target))
    {
        return State->SightScore > 0.f;
    }

    return false;
}

FSOTS_PerceivedTargetState USOTS_AIPerceptionComponent::GetTargetState(AActor* Target, bool& bFound) const
{
    bFound = false;

    if (Target)
    {
        if (const FSOTS_PerceivedTargetState* State = TargetStates.Find(Target))
        {
            bFound = true;
            return *State;
        }
    }

    return FSOTS_PerceivedTargetState();
}

float USOTS_AIPerceptionComponent::GetCurrentSuspicion01() const
{
    if (!GuardConfig)
    {
        return 0.0f;
    }

    const float MaxSuspicion =
        (GuardConfig->Config.MaxSuspicion > 0.0f)
            ? GuardConfig->Config.MaxSuspicion
            : 1.0f;

    if (MaxSuspicion <= 0.0f)
    {
        return 0.0f;
    }

    return FMath::Clamp(CurrentSuspicion / MaxSuspicion, 0.0f, 1.0f);
}

void USOTS_AIPerceptionComponent::SuppressPerceptionForDuration(float Seconds)
{
    if (Seconds <= 0.f)
    {
        bPerceptionSuppressed = false;
        return;
    }

    bPerceptionSuppressed = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            SuppressionTimerHandle,
            [this]()
            {
                bPerceptionSuppressed = false;
            },
            Seconds,
            false);
    }
}

void USOTS_AIPerceptionComponent::ForceAlertToLocation(FVector Location)
{
    // Bump awareness of all tracked targets to Alerted and update last known.
    for (auto& Pair : TargetStates)
    {
        FSOTS_PerceivedTargetState& State = Pair.Value;
        State.Awareness = 1.0f;
        State.State = ESOTS_PerceptionState::Alerted;
        State.LastKnownLocation = Location;
        State.TimeSinceLastSeen = 0.f;

        if (AActor* TargetActor = State.Target.Get())
        {
            OnTargetPerceptionChanged.Broadcast(TargetActor, State.State);
        }
    }

    ReportDetectionLevelChanged(ESOTS_PerceptionState::Alerted);

    UpdateBlackboardAndTags();
}

void USOTS_AIPerceptionComponent::ForceForgetTarget(AActor* Target)
{
    if (!Target)
    {
        return;
    }

    TargetStates.Remove(Target);
}

int32 USOTS_AIPerceptionComponent::GetNextTargetStartIndex(int32 NumTargets)
{
    if (NumTargets <= 0)
    {
        return 0;
    }

    TargetRoundRobinIndex = TargetRoundRobinIndex % NumTargets;
    return TargetRoundRobinIndex;
}

FSOTS_TargetPointVisibilityResult USOTS_AIPerceptionComponent::EvaluateTargetVisibility_MultiPoint(
    AActor* Target,
    const FVector& EyeLocation,
    ECollisionChannel TraceChannel,
    const USOTS_AIPerceptionConfig* Config,
    bool bDrawDebug,
    float DebugDrawDuration)
{
    FSOTS_TargetPointVisibilityResult Result;

    if (!Target || !Config)
    {
        return Result;
    }

    TArray<FVector> Points;
    TArray<bool> IsCorePoint;

    if (!GatherTargetPointWorldLocations(Target, Config->TargetPointBones, Config->MaxPointTracesPerTarget, Points, IsCorePoint))
    {
        return Result;
    }

    const int32 MaxPoints = FMath::Min(Config->MaxPointTracesPerTarget, Points.Num());

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return Result;
    }

    UWorld* World = Owner->GetWorld();
    if (!World)
    {
        return Result;
    }

    for (int32 i = 0; i < MaxPoints; ++i)
    {
        if (!CanSpendTraces(1))
        {
            break;
        }

        SpendTrace();
        Result.TestedPoints++;

        const FVector End = Points[i];

        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(SOTS_AIPerception_LOS), false);
        Params.AddIgnoredActor(Owner);

        const bool bHit = World->LineTraceSingleByChannel(
            Hit,
            EyeLocation,
            End,
            TraceChannel,
            Params);

        const bool bVisible = !bHit || Hit.GetActor() == Target;
        if (bVisible)
        {
            Result.VisiblePoints++;

            if (IsCorePoint.IsValidIndex(i) && IsCorePoint[i])
            {
                Result.bAnyCorePointVisible = true;
                if (Config->bEarlyOutOnCorePointVisible)
                {
                    break;
                }
            }
        }

    #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bDrawDebug && World)
        {
            const FColor Color = bVisible ? FColor::Green : FColor::Red;
            DrawDebugSphere(World, End, 8.f, 8, Color, false, DebugDrawDuration);
            DrawDebugLine(World, EyeLocation, End, Color, false, DebugDrawDuration);
        }
    #endif
    }

    if (Result.TestedPoints > 0)
    {
        Result.VisibilityFraction = static_cast<float>(Result.VisiblePoints) / static_cast<float>(Result.TestedPoints);
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDrawDebug && World && Result.TestedPoints > 0)
    {
        const FString Text = FString::Printf(TEXT("Vis %d/%d (%.2f)"), Result.VisiblePoints, Result.TestedPoints, Result.VisibilityFraction);
        const FVector TextLocation = Target ? Target->GetActorLocation() + FVector(0.f, 0.f, 120.f) : EyeLocation;
        DrawDebugString(World, TextLocation, Text, nullptr, FColor::White, DebugDrawDuration, true, 1.2f);
    }
#endif

    return Result;
}

bool USOTS_AIPerceptionComponent::ComputeTargetLOS(
    AActor* Target,
    const FVector& EyeLocation,
    ECollisionChannel TraceChannel,
    FSOTS_TargetPointVisibilityResult& OutVisibility,
    bool bDrawDebug,
    float DebugDrawDuration)
{
    OutVisibility = FSOTS_TargetPointVisibilityResult();

    if (!Target || !PerceptionConfig)
    {
        return false;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    UWorld* World = Owner->GetWorld();
    if (!World)
    {
        return false;
    }

    // Legacy single-point LOS path.
    if (!PerceptionConfig->bEnableTargetPointLOS)
    {
        if (!CanSpendTraces(1))
        {
            return false;
        }

        SpendTrace();

        const FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 80.f);

        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(SOTS_AIPerception_LOS), false);
        Params.AddIgnoredActor(Owner);

        const bool bHit = World->LineTraceSingleByChannel(
            Hit,
            EyeLocation,
            End,
            TraceChannel,
            Params);

        const bool bHasLOS = !bHit || Hit.GetActor() == Target;

        OutVisibility.TestedPoints = 1;
        OutVisibility.VisiblePoints = bHasLOS ? 1 : 0;
        OutVisibility.bAnyCorePointVisible = bHasLOS;
        OutVisibility.VisibilityFraction = bHasLOS ? 1.0f : 0.0f;

    #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (bDrawDebug && World)
        {
            const FColor Color = bHasLOS ? FColor::Green : FColor::Red;
            DrawDebugSphere(World, End, 8.f, 8, Color, false, DebugDrawDuration);
            DrawDebugLine(World, EyeLocation, End, Color, false, DebugDrawDuration);
        }
    #endif

        return bHasLOS;
    }

    // Multi-point LOS evaluation.
    OutVisibility = EvaluateTargetVisibility_MultiPoint(Target, EyeLocation, TraceChannel, PerceptionConfig, bDrawDebug, DebugDrawDuration);

    // Default: any visible point counts as LOS.
    return OutVisibility.VisiblePoints > 0;
}

USkeletalMeshComponent* USOTS_AIPerceptionComponent::FindBestSkeletalMeshComponent(AActor* Target)
{
    if (!Target)
    {
        return nullptr;
    }

    if (ACharacter* Character = Cast<ACharacter>(Target))
    {
        if (USkeletalMeshComponent* Mesh = Character->GetMesh())
        {
            return Mesh;
        }
    }

    return Target->FindComponentByClass<USkeletalMeshComponent>();
}

bool USOTS_AIPerceptionComponent::GatherTargetPointWorldLocations(
    AActor* Target,
    const TArray<FName>& BoneNames,
    int32 MaxPoints,
    TArray<FVector>& OutPoints,
    TArray<bool>& OutIsCorePoint) const
{
    OutPoints.Reset();
    OutIsCorePoint.Reset();

    if (!Target || MaxPoints <= 0)
    {
        return false;
    }

    const bool bAllowFallback = PerceptionConfig ? PerceptionConfig->bFallbackToSinglePointWhenNoSkelMesh : false;

    if (USkeletalMeshComponent* Mesh = FindBestSkeletalMeshComponent(Target))
    {
        for (int32 Index = 0; Index < BoneNames.Num() && OutPoints.Num() < MaxPoints; ++Index)
        {
            const FName BoneName = BoneNames[Index];
            if (BoneName.IsNone())
            {
                continue;
            }

            const bool bHasSocket = Mesh->DoesSocketExist(BoneName);
            const int32 BoneIndex = Mesh->GetBoneIndex(BoneName);

            if (!bHasSocket && BoneIndex == INDEX_NONE)
            {
                continue;
            }

            OutPoints.Add(Mesh->GetSocketLocation(BoneName));

            const bool bIsCorePoint = OutPoints.Num() <= 3;
            OutIsCorePoint.Add(bIsCorePoint);
        }
    }

    if (OutPoints.Num() == 0 && bAllowFallback)
    {
        OutPoints.Add(Target->GetActorLocation() + FVector(0.f, 0.f, 60.f));
        OutIsCorePoint.Add(true);
    }

    return OutPoints.Num() > 0;
}

void USOTS_AIPerceptionComponent::ReportDetectionLevelChanged(ESOTS_PerceptionState NewState)
{
    SetPerceptionState(NewState);
}

void USOTS_AIPerceptionComponent::ReportAlertStateChanged(bool bAlertedNow)
{
    if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
    {
        if (AActor* OwnerActor = GetOwner())
        {
            GSM->ReportEnemyDetectionEvent(OwnerActor, bAlertedNow);
        }
    }
}

void USOTS_AIPerceptionComponent::HandleReportedNoise(const FVector& Location, float Loudness)
{
    if (!PerceptionConfig)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    const float Distance = FVector::Dist(Owner->GetActorLocation(), Location);
    const float Radius = FMath::Lerp(
        PerceptionConfig->HearingRadius_Quiet,
        PerceptionConfig->HearingRadius_Loud,
        Loudness);

    if (Distance > Radius)
    {
        return;
    }

    // Simple hearing bump for the primary target (usually player).
    const float Delta = FMath::Clamp(1.0f - (Distance / Radius), 0.0f, 1.0f);

    for (auto& Pair : TargetStates)
    {
        FSOTS_PerceivedTargetState& State = Pair.Value;
        State.HearingScore = FMath::Clamp(State.HearingScore + Delta, 0.0f, 1.0f);
    }

    // Suspicion bump from hearing event.
    if (GuardConfig)
    {
        PreviousSuspicion = CurrentSuspicion;
        CurrentSuspicion += GuardConfig->Config.HearingSuspicionPerEvent;
    }
}

void USOTS_AIPerceptionComponent::RefreshWatchedTargets()
{
    WatchedActors.Reset();

    if (UWorld* World = GetWorld())
    {
        for (TSubclassOf<AActor> Class : WatchedActorClasses)
        {
            if (!*Class)
            {
                continue;
            }

            TArray<AActor*> Found;
            UGameplayStatics::GetAllActorsOfClass(World, Class, Found);

            for (AActor* Actor : Found)
            {
                if (Actor)
                {
                    WatchedActors.AddUnique(Actor);
                }
            }
        }
    }
}

void USOTS_AIPerceptionComponent::UpdatePerception()
{
    if (!PerceptionConfig)
    {
        return;
    }

    ResetTraceBudget();

    const float DeltaSeconds = PerceptionUpdateInterval;

    if (bPerceptionSuppressed)
    {
        // Even while suppressed, we allow awareness to decay over time.
        for (auto& Pair : TargetStates)
        {
            FSOTS_PerceivedTargetState& State = Pair.Value;
            State.Awareness = FMath::Max(0.0f, State.Awareness - PerceptionConfig->DetectionDecayPerSecond * DeltaSeconds);
            State.TimeSinceLastSeen += DeltaSeconds;
        }

        return;
    }

    RefreshWatchedTargets();

    // Ensure every watched actor has a corresponding state entry.
    for (const TWeakObjectPtr<AActor>& TargetPtr : WatchedActors)
    {
        if (AActor* Target = TargetPtr.Get())
        {
            TargetStates.FindOrAdd(Target).Target = Target;
        }
    }

    ESOTS_PerceptionState HighestState = ESOTS_PerceptionState::Unaware;

    // Determine evaluation order (round-robin) and cap per-update workload.
    TArray<AActor*> OrderedTargets;
    OrderedTargets.Reserve(WatchedActors.Num());
    for (const TWeakObjectPtr<AActor>& TargetPtr : WatchedActors)
    {
        if (AActor* Target = TargetPtr.Get())
        {
            OrderedTargets.Add(Target);
        }
    }

    const int32 NumTargets = OrderedTargets.Num();

    // Choose the most relevant target for debug draw (highest awareness pre-update).
    AActor* DebugDrawTarget = nullptr;
    if (PerceptionConfig->bDebugDrawTargetPoints)
    {
        float BestPreAwareness = -1.0f;
        for (auto& Pair : TargetStates)
        {
            const FSOTS_PerceivedTargetState& State = Pair.Value;
            if (State.Awareness > BestPreAwareness && State.Target.IsValid())
            {
                BestPreAwareness = State.Awareness;
                DebugDrawTarget = State.Target.Get();
            }
        }
    }
    const int32 StartIndex = GetNextTargetStartIndex(NumTargets);
    const int32 MaxTargetsToEvaluate = (PerceptionConfig->MaxTargetsPerUpdate > 0)
        ? PerceptionConfig->MaxTargetsPerUpdate
        : NumTargets;

    int32 EvaluatedTargetsCount = 0;

    for (int32 Offset = 0;
        Offset < NumTargets &&
        (MaxTargetsToEvaluate <= 0 || EvaluatedTargetsCount < MaxTargetsToEvaluate);
        ++Offset)
    {
        if (PerceptionConfig->MaxTotalTracesPerUpdate > 0 && !CanSpendTraces(1))
        {
            break;
        }

        const int32 Index = (StartIndex + Offset) % NumTargets;
        AActor* Target = OrderedTargets[Index];
        if (!Target)
        {
            continue;
        }

        if (FSOTS_PerceivedTargetState* State = TargetStates.Find(Target))
        {
            const bool bDrawThisTarget = PerceptionConfig->bDebugDrawTargetPoints && (Target == DebugDrawTarget);
            UpdateSingleTarget(*State, DeltaSeconds, bDrawThisTarget);

            if (State->LastTestedPoints > 0)
            {
                ++EvaluatedTargetsCount;
            }

            if (static_cast<uint8>(State->State) > static_cast<uint8>(HighestState))
            {
                HighestState = State->State;
            }
        }
    }

    if (NumTargets > 0)
    {
        TargetRoundRobinIndex = (TargetRoundRobinIndex + EvaluatedTargetsCount) % NumTargets;
    }

    // Ensure HighestState accounts for non-evaluated targets too.
    for (auto& Pair : TargetStates)
    {
        FSOTS_PerceivedTargetState& State = Pair.Value;
        if (static_cast<uint8>(State.State) > static_cast<uint8>(HighestState))
        {
            HighestState = State.State;
        }
    }

    // Suspicion drive based on primary target (highest awareness).
    FSOTS_PerceivedTargetState* BestState = nullptr;
    float SuspicionNormalized = 0.0f;
    bool bHasLOSOnPrimary = false;

    if (GuardConfig)
    {
        PreviousSuspicion = CurrentSuspicion;

        // Choose primary target (highest awareness) for suspicion purposes.
        float BestAwareness = -1.0f;

        for (auto& Pair : TargetStates)
        {
            FSOTS_PerceivedTargetState& State = Pair.Value;
            if (State.Awareness > BestAwareness && State.Target.IsValid())
            {
                BestAwareness = State.Awareness;
                BestState = &State;
            }
        }

        const FSOTS_AIGuardPerceptionConfig& Cfg = GuardConfig->Config;

        // Increase suspicion when we have clear LOS to the primary target.
        if (BestState && BestState->SightScore > 0.0f)
        {
            float VisibilityMultiplier = 1.0f;

            if (PerceptionConfig->bEnableTargetPointLOS)
            {
                FSOTS_TargetPointVisibilityResult Vis;
                Vis.VisibilityFraction = BestState->LastVisibilityFraction;
                Vis.VisiblePoints = BestState->LastVisiblePoints;
                Vis.TestedPoints = BestState->LastTestedPoints;
                Vis.bAnyCorePointVisible = BestState->bLastAnyCoreVisible;

                VisibilityMultiplier = ComputeVisibilitySuspicionMultiplier(Vis, PerceptionConfig);
            }

            CurrentSuspicion += Cfg.SightSuspicionPerSecond * DeltaSeconds * VisibilityMultiplier;
            bHasLOSOnPrimary = true;
        }

        // Optional shadow awareness (secondary bump via GSM cached shadow point; never enumerates lights).
        if (PerceptionConfig->bEnableShadowAwareness)
        {
            UWorld* World = GetWorld();
            if (World && ShouldRunShadowAwareness(PerceptionConfig, World->GetTimeSeconds()))
            {
                const double Now = World->GetTimeSeconds();
                const float Interval = FMath::Max(PerceptionConfig->ShadowCheckInterval, 0.05f);
                NextShadowCheckTimeSeconds = Now + Interval + FMath::FRandRange(0.0f, Interval * 0.25f);

                if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
                {
                    const FSOTS_ShadowCandidate Cand = GSM->GetPlayerShadowCandidate();

                    if (Cand.bValid && Cand.Illumination01 >= PerceptionConfig->ShadowMinIllumination)
                    {
                        AActor* PlayerActor = nullptr;

                        if (APlayerController* PC = World->GetFirstPlayerController())
                        {
                            PlayerActor = PC->GetPawn();
                        }

                        if (PlayerActor)
                        {
                            const float DistanceToShadow = FVector::Dist(GetOwner()->GetActorLocation(), Cand.ShadowPointWS);
                            if (DistanceToShadow <= PerceptionConfig->ShadowAwarenessMaxDistance)
                            {
                                if (CanSpendTraces(1))
                                {
                                    SpendTrace();
                                    ++ShadowTracesUsedThisSecond;

                                    const FVector EyeLocation = GetOwner()->GetActorLocation() + FVector(0.f, 0.f, 80.f);

                                    FHitResult Hit;
                                    FCollisionQueryParams Params(SCENE_QUERY_STAT(SOTS_AIPerception_ShadowLOS), false);
                                    Params.AddIgnoredActor(GetOwner());
                                    Params.AddIgnoredActor(PlayerActor);

                                    const bool bHit = World->LineTraceSingleByChannel(
                                        Hit,
                                        EyeLocation,
                                        Cand.ShadowPointWS,
                                        ECC_Visibility,
                                        Params);

                                    const bool bShadowVisible = !bHit || Hit.ImpactPoint.Equals(Cand.ShadowPointWS, 25.f);

                                    if (bShadowVisible)
                                    {
                                        if (FSOTS_PerceivedTargetState* PlayerState = TargetStates.Find(PlayerActor))
                                        {
                                            const float ShadowDelta = PerceptionConfig->ShadowSuspicionGainPerSecond * DeltaSeconds;
                                            CurrentSuspicion += ShadowDelta;
                                        }
                                    }

                                #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
                                    if (PerceptionConfig->bDebugDrawShadowAwareness)
                                    {
                                        const FColor Color = bShadowVisible ? FColor::Green : FColor::Red;
                                        const float DebugDuration = FMath::Max(PerceptionConfig->ShadowCheckInterval, 0.01f);
                                        DrawDebugLine(World, EyeLocation, Cand.ShadowPointWS, Color, false, DebugDuration);
                                        DrawDebugPoint(World, Cand.ShadowPointWS, 10.f, Color, false, DebugDuration);
                                    }
                                #endif
                                }
                            }
                        }
                    }
                }
            }
        }

        // Passive decay each update.
        CurrentSuspicion -= Cfg.SuspicionDecayPerSecond * DeltaSeconds;

        // Clamp to [0, MaxSuspicion].
        const float MaxSuspicion =
            (Cfg.MaxSuspicion > 0.0f) ? Cfg.MaxSuspicion : 1.0f;

        CurrentSuspicion = FMath::Clamp(CurrentSuspicion, 0.0f, MaxSuspicion);

        // Fold normalized suspicion into the global stealth manager.
        SuspicionNormalized =
            (MaxSuspicion > 0.0f)
                ? FMath::Clamp(CurrentSuspicion / MaxSuspicion, 0.0f, 1.0f)
                : 0.0f;

        if (USOTS_GlobalStealthManagerSubsystem* GSM =
                USOTS_GlobalStealthManagerSubsystem::Get(this))
        {
            if (AActor* OwnerActor = GetOwner())
            {
                GSM->ReportAISuspicion(OwnerActor, SuspicionNormalized);
            }
        }

        // Tag transitions: fully alerted / lost sight thresholds.
        const float CalmThreshold = MaxSuspicion * 0.2f;

        if (AActor* OwnerActor = GetOwner())
        {
            if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
            {
                if (PreviousSuspicion < MaxSuspicion && CurrentSuspicion >= MaxSuspicion)
                {
                    if (GuardConfig->Config.Tag_OnFullyAlerted.IsValid())
                    {
                        TagSubsystem->AddTagToActor(OwnerActor, GuardConfig->Config.Tag_OnFullyAlerted);
                    }
                }
                else if (PreviousSuspicion >= CalmThreshold && CurrentSuspicion < CalmThreshold)
                {
                    if (GuardConfig->Config.Tag_OnLostSight.IsValid())
                    {
                        TagSubsystem->AddTagToActor(OwnerActor, GuardConfig->Config.Tag_OnLostSight);
                    }

                    if (GuardConfig->Config.Tag_OnFullyAlerted.IsValid())
                    {
                        TagSubsystem->RemoveTagFromActor(OwnerActor, GuardConfig->Config.Tag_OnFullyAlerted);
                    }
                }
            }
        }
    }

    if (HighestState != CurrentState)
    {
        ReportDetectionLevelChanged(HighestState);
    }

    // Blackboard sync for suspicion / state / LOS using config-driven keys.
    if (PerceptionConfig)
    {
        AActor* OwnerActor = GetOwner();
        APawn* PawnOwner = OwnerActor ? Cast<APawn>(OwnerActor) : nullptr;
        AAIController* AIController = PawnOwner ? Cast<AAIController>(PawnOwner->GetController()) : nullptr;
        UBlackboardComponent* BlackboardComp = AIController ? AIController->GetBlackboardComponent() : nullptr;

        if (BlackboardComp)
        {
            const FSOTS_AIPerceptionBlackboardConfig& BB = PerceptionConfig->BlackboardConfig;

            // Suspicion float [0,1].
            if (BB.SuspicionKey.SelectedKeyType)
            {
                BlackboardComp->SetValueAsFloat(BB.SuspicionKey.SelectedKeyName, SuspicionNormalized);
            }

            // State enum/int.
            if (BB.StateKey.SelectedKeyType)
            {
                const int32 StateAsInt = static_cast<int32>(CurrentState);
                BlackboardComp->SetValueAsInt(BB.StateKey.SelectedKeyName, StateAsInt);
            }

            // LOS to primary target.
            if (BB.HasLOSKey.SelectedKeyType)
            {
                BlackboardComp->SetValueAsBool(BB.HasLOSKey.SelectedKeyName, bHasLOSOnPrimary);
            }
        }
    }

    LogTelemetrySnapshot(NumTargets, EvaluatedTargetsCount, MaxTargetsToEvaluate, bHasLOSOnPrimary, BestState, SuspicionNormalized);

    // Optional debug correlation between local suspicion and GSM aggregate.
    if (bDebugSuspicion)
    {
        AActor* OwnerActor = GetOwner();
        float GlobalAISusp01 = 0.0f;

        if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
        {
            GlobalAISusp01 = GSM->GetCurrentStealthBreakdown().AISuspicion01;
        }

        UE_LOG(LogTemp, Verbose,
            TEXT("[AIPerc] Guard=%s Susp=%.2f Norm=%.2f GlobalAISusp=%.2f State=%d HasLOS=%s"),
            *GetNameSafe(OwnerActor),
            CurrentSuspicion,
            SuspicionNormalized,
            GlobalAISusp01,
            static_cast<int32>(CurrentState),
            bHasLOSOnPrimary ? TEXT("true") : TEXT("false"));
    }

    UpdateBlackboardAndTags();
}

void USOTS_AIPerceptionComponent::HandlePerceptionStateChanged(
    ESOTS_PerceptionState OldState,
    ESOTS_PerceptionState NewState)
{

    // Optional FX: fire guard-specific cues when crossing key perception
    // thresholds. This is fully data-driven via GuardConfig and can be left
    // unconfigured without affecting gameplay.
    if (!GuardConfig)
    {
        return;
    }

    // FX manager is a global game-instance subsystem; if it does not exist,
    // perception logic should still function normally.
    USOTS_FXManagerSubsystem* FXMgr = USOTS_FXManagerSubsystem::Get();
    if (!FXMgr)
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    const FVector OwnerLocation = OwnerActor->GetActorLocation();

    const auto TriggerIfValid = [&](const FGameplayTag& FXTag)
    {
        if (!FXTag.IsValid())
        {
            return;
        }

        FXMgr->TriggerFXByTag(
            this,
            FXTag,
            OwnerActor,
            /*Target=*/nullptr,
            OwnerLocation,
            FRotator::ZeroRotator);
    };

    switch (NewState)
    {
    case ESOTS_PerceptionState::SoftSuspicious:
        TriggerIfValid(GuardConfig->Config.FXTag_OnSoftSuspicious);
        break;

    case ESOTS_PerceptionState::HardSuspicious:
        TriggerIfValid(GuardConfig->Config.FXTag_OnHardSuspicious);
        break;

    case ESOTS_PerceptionState::Alerted:
        TriggerIfValid(GuardConfig->Config.FXTag_OnAlerted);
        break;

    case ESOTS_PerceptionState::Unaware:
        // When dropping back to Unaware from a stronger state, treat that as
        // "lost sight" for FX purposes.
        if (OldState == ESOTS_PerceptionState::Alerted ||
            OldState == ESOTS_PerceptionState::HardSuspicious)
        {
            TriggerIfValid(GuardConfig->Config.FXTag_OnLostSight);
        }
        break;

    default:
        break;
    }

    PreviousState = NewState;
}

void USOTS_AIPerceptionComponent::UpdateSingleTarget(FSOTS_PerceivedTargetState& TargetState, float DeltaSeconds, bool bDebugDrawTargetPoints)
{
    AActor* Owner = GetOwner();
    AActor* Target = TargetState.Target.Get();

    if (!Owner || !Target || !PerceptionConfig)
    {
        return;
    }

    const FVector OwnerLocation = Owner->GetActorLocation();
    const FVector TargetLocation = Target->GetActorLocation();
    const FVector ToTarget = (TargetLocation - OwnerLocation);
    const float Distance = ToTarget.Size();

    // Basic sight gating.
    bool bInSightRange = Distance <= PerceptionConfig->MaxSightDistance;
    if (!bInSightRange)
    {
        TargetState.SightScore = 0.f;
    }

    bool bHasLOS = false;
    bool bInCoreFOV = false;
    bool bInPeripheralFOV = false;
    FSOTS_TargetPointVisibilityResult Visibility;

    if (bInSightRange && Distance > KINDA_SMALL_NUMBER)
    {
        const FVector Dir = ToTarget.GetSafeNormal();
        const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal();
        const float Dot = FVector::DotProduct(Forward, Dir);
        const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));

        const float HalfCoreFOV = PerceptionConfig->CoreFOVDegrees * 0.5f;
        const float HalfPeripheralFOV = PerceptionConfig->PeripheralFOVDegrees * 0.5f;

        bInCoreFOV = AngleDegrees <= HalfCoreFOV;
        bInPeripheralFOV = !bInCoreFOV && AngleDegrees <= HalfPeripheralFOV;

        const FVector EyeLocation = OwnerLocation + FVector(0.f, 0.f, 80.f);
        const float DebugDuration = PerceptionUpdateInterval;
        bHasLOS = ComputeTargetLOS(Target, EyeLocation, ECC_Visibility, Visibility, bDebugDrawTargetPoints, DebugDuration);
    }

    TargetState.SightScore = bHasLOS ? 1.0f : 0.0f;
    TargetState.LastVisibilityFraction = Visibility.VisibilityFraction;
    TargetState.LastVisiblePoints = Visibility.VisiblePoints;
    TargetState.LastTestedPoints = Visibility.TestedPoints;
    TargetState.bLastAnyCoreVisible = Visibility.bAnyCorePointVisible;

    // Awareness integration.
    float DetectionSpeed = 0.0f;
    if (bHasLOS)
    {
        if (bInCoreFOV)
        {
            DetectionSpeed = PerceptionConfig->DetectionSpeed_Core;
        }
        else if (bInPeripheralFOV)
        {
            DetectionSpeed = PerceptionConfig->DetectionSpeed_Peripheral;
        }

        // Stealth multiplier from global stealth manager.
        USOTS_GlobalStealthManagerSubsystem* GSM =
            USOTS_GlobalStealthManagerSubsystem::Get(this);

        const float StealthScoreRaw = (GSM && Target)
            ? GSM->GetStealthScoreFor(Target)
            : 1.0f;
        const float StealthScore = FMath::Clamp(StealthScoreRaw, 0.0f, 1.0f);

        // Lazy-load the curve once.
        if (!StealthCurveCached &&
            PerceptionConfig &&
            PerceptionConfig->StealthScoreToDetectionMultiplier.IsValid())
        {
            StealthCurveCached = PerceptionConfig->StealthScoreToDetectionMultiplier.LoadSynchronous();
        }

        float StealthMultiplier = 1.0f;
        if (StealthCurveCached)
        {
            StealthMultiplier = StealthCurveCached->GetFloatValue(StealthScore);
        }

        DetectionSpeed *= StealthMultiplier;

        TargetState.Awareness = FMath::Clamp(
            TargetState.Awareness + DetectionSpeed * DeltaSeconds,
            0.0f,
            1.0f);

        TargetState.LastKnownLocation = TargetLocation;
        TargetState.TimeSinceLastSeen = 0.0f;
    }
    else
    {
        // Decay awareness over time when not in LOS.
        TargetState.Awareness = FMath::Max(
            0.0f,
            TargetState.Awareness - PerceptionConfig->DetectionDecayPerSecond * DeltaSeconds);

        TargetState.TimeSinceLastSeen += DeltaSeconds;
    }

    // Apply any hearing score as a soft bump.
    if (TargetState.HearingScore > 0.0f)
    {
        TargetState.Awareness = FMath::Clamp(TargetState.Awareness + TargetState.HearingScore * DeltaSeconds, 0.0f, 1.0f);
        TargetState.HearingScore = FMath::Max(0.0f, TargetState.HearingScore - DeltaSeconds);
    }

    // Map awareness to perception state using config thresholds.
    ESOTS_PerceptionState OldState = TargetState.State;
    ESOTS_PerceptionState NewState = ESOTS_PerceptionState::Unaware;

    const float A = TargetState.Awareness;

    if (A >= PerceptionConfig->Threshold_Alerted)
    {
        NewState = ESOTS_PerceptionState::Alerted;
    }
    else if (A >= PerceptionConfig->Threshold_HardSuspicious)
    {
        NewState = ESOTS_PerceptionState::HardSuspicious;
    }
    else if (A >= PerceptionConfig->Threshold_SoftSuspicious)
    {
        NewState = ESOTS_PerceptionState::SoftSuspicious;
    }

    TargetState.State = NewState;

    if (NewState != OldState)
    {
        if (AActor* TargetActor = TargetState.Target.Get())
        {
            OnTargetPerceptionChanged.Broadcast(TargetActor, NewState);
        }
    }
}

void USOTS_AIPerceptionComponent::UpdateBlackboardAndTags()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(Owner);
    AAIController* AIController = OwnerPawn ? Cast<AAIController>(OwnerPawn->GetController()) : nullptr;
    if (!AIController)
    {
        return;
    }

    UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    // Choose the primary target (highest awareness).
    FSOTS_PerceivedTargetState* BestState = nullptr;
    float BestAwareness = -1.0f;

    for (auto& Pair : TargetStates)
    {
        FSOTS_PerceivedTargetState& State = Pair.Value;
        if (State.Awareness > BestAwareness && State.Target.IsValid())
        {
            BestAwareness = State.Awareness;
            BestState = &State;
        }
    }

    if (!BestState)
    {
        // No valid target; clear core keys.
        BlackboardComp->SetValueAsObject(SOTS_AIPerception_BBKeys::TargetActor, nullptr);
        BlackboardComp->SetValueAsBool(SOTS_AIPerception_BBKeys::HasLOSToTarget, false);
        BlackboardComp->SetValueAsFloat(SOTS_AIPerception_BBKeys::Awareness, 0.0f);
        // Keep last perception state for BT logic.
        return;
    }

    AActor* BestTargetActor = BestState->Target.Get();
    const bool bHasLOS = BestState->SightScore > 0.0f;

    BlackboardComp->SetValueAsObject(SOTS_AIPerception_BBKeys::TargetActor, BestTargetActor);
    BlackboardComp->SetValueAsBool(SOTS_AIPerception_BBKeys::HasLOSToTarget, bHasLOS);
    BlackboardComp->SetValueAsVector(SOTS_AIPerception_BBKeys::LastKnownTargetLocation, BestState->LastKnownLocation);
    BlackboardComp->SetValueAsFloat(SOTS_AIPerception_BBKeys::Awareness, BestState->Awareness);
    BlackboardComp->SetValueAsEnum(SOTS_AIPerception_BBKeys::PerceptionState, static_cast<uint8>(CurrentState));


    const bool bHasLOSOnPlayer = bHasLOS && BestTargetActor != nullptr;
    ApplyLOSStateTags(bHasLOSOnPlayer);
}

void USOTS_AIPerceptionComponent::ApplyStateTags()
{
    ApplyPerceptionStateTags(CurrentState);
}

void USOTS_AIPerceptionComponent::ApplyLOSStateTags(bool bHasLOS)
{
    if (!PerceptionConfig)
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
    {
        const FGameplayTag HasLOSTag = PerceptionConfig->Tag_OnHasLOS_Player;
        const FGameplayTag LostLOSTag = PerceptionConfig->Tag_OnLostLOS_Player;

        if (HasLOSTag.IsValid())
        {
            if (bHasLOS)
            {
                TagSubsystem->AddTagToActor(OwnerActor, HasLOSTag);
            }
            else
            {
                TagSubsystem->RemoveTagFromActor(OwnerActor, HasLOSTag);
            }
        }

        if (LostLOSTag.IsValid())
        {
            if (!bHasLOS)
            {
                TagSubsystem->AddTagToActor(OwnerActor, LostLOSTag);
            }
            else
            {
                TagSubsystem->RemoveTagFromActor(OwnerActor, LostLOSTag);
            }
        }
    }
}

void USOTS_AIPerceptionComponent::ApplyPerceptionStateTags(ESOTS_PerceptionState NewState)
{
    if (!PerceptionConfig)
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
    {
        TagSubsystem->RemoveTagFromActor(OwnerActor, PerceptionConfig->Tag_State_Unaware);
        TagSubsystem->RemoveTagFromActor(OwnerActor, PerceptionConfig->Tag_State_SoftSuspicious);
        TagSubsystem->RemoveTagFromActor(OwnerActor, PerceptionConfig->Tag_State_HardSuspicious);
        TagSubsystem->RemoveTagFromActor(OwnerActor, PerceptionConfig->Tag_State_Alerted);

        FGameplayTag StateTag;
        switch (NewState)
        {
        case ESOTS_PerceptionState::Unaware:
            StateTag = PerceptionConfig->Tag_State_Unaware;
            break;
        case ESOTS_PerceptionState::SoftSuspicious:
            StateTag = PerceptionConfig->Tag_State_SoftSuspicious;
            break;
        case ESOTS_PerceptionState::HardSuspicious:
            StateTag = PerceptionConfig->Tag_State_HardSuspicious;
            break;
        case ESOTS_PerceptionState::Alerted:
            StateTag = PerceptionConfig->Tag_State_Alerted;
            break;
        default:
            break;
        }

        if (StateTag.IsValid())
        {
            TagSubsystem->AddTagToActor(OwnerActor, StateTag);
        }
    }
}

void USOTS_AIPerceptionComponent::NotifyGlobalStealthManager(
    ESOTS_PerceptionState OldState,
    ESOTS_PerceptionState NewState)
{
    const bool bWasAlerted = OldState == ESOTS_PerceptionState::Alerted;
    const bool bIsAlerted = NewState == ESOTS_PerceptionState::Alerted;

    if (bWasAlerted != bIsAlerted)
    {
        ReportAlertStateChanged(bIsAlerted);
    }
}

void USOTS_AIPerceptionComponent::SetPerceptionState(ESOTS_PerceptionState NewState)
{
    if (NewState == CurrentState)
    {
        return;
    }

    const ESOTS_PerceptionState OldState = CurrentState;
    CurrentState = NewState;
    PreviousState = OldState;

    ApplyPerceptionStateTags(NewState);
    NotifyGlobalStealthManager(OldState, NewState);
    HandlePerceptionStateChanged(OldState, NewState);
    OnPerceptionStateChanged.Broadcast(CurrentState);
}

bool USOTS_AIPerceptionComponent::CanSpendTraces(int32 Count) const
{
    if (Count <= 0)
    {
        return true;
    }

    const int32 MaxTraces = (PerceptionConfig && PerceptionConfig->MaxTotalTracesPerUpdate > 0)
        ? PerceptionConfig->MaxTotalTracesPerUpdate
        : INT32_MAX;

    return (TracesUsedThisUpdate + Count) <= MaxTraces;
}

void USOTS_AIPerceptionComponent::LogTelemetrySnapshot(
    int32 NumTargets,
    int32 EvaluatedTargetsCount,
    int32 MaxTargetsToEvaluate,
    bool bHasLOSOnPrimary,
    const FSOTS_PerceivedTargetState* BestState,
    float SuspicionNormalized)
{
    if (!PerceptionConfig || !PerceptionConfig->bEnablePerceptionTelemetry)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const double Now = World->GetTimeSeconds();
    if (Now < NextTelemetryTimeSeconds)
    {
        return;
    }

    const float Interval = FMath::Max(PerceptionConfig->PerceptionTelemetryIntervalSeconds, 0.05f);
    NextTelemetryTimeSeconds = Now + Interval + FMath::FRandRange(0.0f, Interval * 0.25f);

    const FString MaxTracesStr = (PerceptionConfig->MaxTotalTracesPerUpdate > 0)
        ? FString::FromInt(PerceptionConfig->MaxTotalTracesPerUpdate)
        : TEXT("inf");

    const int32 MaxTargetsResolved = (MaxTargetsToEvaluate > 0)
        ? MaxTargetsToEvaluate
        : NumTargets;

    const FString MaxTargetsStr = (MaxTargetsResolved > 0)
        ? FString::FromInt(MaxTargetsResolved)
        : TEXT("inf");

    FString Message = FString::Printf(
        TEXT("[AIPerc/Telem] Guard=%s Susp=%.2f Targets=%d/%s Traces=%d/%s LOS=%s"),
        *GetNameSafe(GetOwner()),
        SuspicionNormalized,
        EvaluatedTargetsCount,
        *MaxTargetsStr,
        TracesUsedThisUpdate,
        *MaxTracesStr,
        bHasLOSOnPrimary ? TEXT("Y") : TEXT("N"));

    if (PerceptionConfig->bTelemetryIncludeTargetPoints && BestState && BestState->LastTestedPoints > 0)
    {
        Message += FString::Printf(
            TEXT(" TPVis=%.2f(%d/%d core=%s)"),
            BestState->LastVisibilityFraction,
            BestState->LastVisiblePoints,
            BestState->LastTestedPoints,
            BestState->bLastAnyCoreVisible ? TEXT("Y") : TEXT("N"));
    }

    if (PerceptionConfig->bTelemetryIncludeShadow)
    {
        const FString MaxShadowStr = (PerceptionConfig->MaxShadowTracesPerSecondPerGuard > 0)
            ? FString::FromInt(PerceptionConfig->MaxShadowTracesPerSecondPerGuard)
            : TEXT("inf");

        const float TimeToNextShadow = (NextShadowCheckTimeSeconds > 0.0)
            ? FMath::Max(0.0f, static_cast<float>(NextShadowCheckTimeSeconds - Now))
            : 0.0f;

        Message += FString::Printf(
            TEXT(" Shadow=%d/%s Next=%.2fs"),
            ShadowTracesUsedThisSecond,
            *MaxShadowStr,
            TimeToNextShadow);
    }

    UE_LOG(LogSOTS_AIPerceptionTelemetry, Verbose, TEXT("%s"), *Message);
}

bool USOTS_AIPerceptionComponent::ShouldRunShadowAwareness(const USOTS_AIPerceptionConfig* Config, double NowSeconds)
{
    if (!Config || !Config->bEnableShadowAwareness)
    {
        return false;
    }

    if (Config->ShadowCheckInterval <= 0.0f)
    {
        return false;
    }

    const int32 SecondStamp = FMath::FloorToInt(NowSeconds);
    if (SecondStamp != ShadowTraceSecondStamp)
    {
        ShadowTraceSecondStamp = SecondStamp;
        ShadowTracesUsedThisSecond = 0;
    }

    if (Config->MaxShadowTracesPerSecondPerGuard > 0 &&
        ShadowTracesUsedThisSecond >= Config->MaxShadowTracesPerSecondPerGuard)
    {
        return false;
    }

    if (NowSeconds < NextShadowCheckTimeSeconds)
    {
        return false;
    }

    return true;
}

float USOTS_AIPerceptionComponent::ComputeVisibilitySuspicionMultiplier(
    const FSOTS_TargetPointVisibilityResult& Vis,
    const USOTS_AIPerceptionConfig* Config) const
{
    if (!Config || !Config->bScaleSuspicionByVisibility)
    {
        return 1.0f;
    }

    if (Vis.TestedPoints <= 0 || Vis.VisiblePoints <= 0)
    {
        return 0.0f;
    }

    if (Vis.VisibilityFraction < Config->MinVisibilityFractionForSuspicion)
    {
        return 0.0f;
    }

    // Core visibility shortcut to max.
    if (Config->bCorePointGivesFullMultiplier && Vis.bAnyCorePointVisible)
    {
        return Config->VisibilitySuspicionMaxMultiplier;
    }

    const float Denominator = 1.0f - Config->MinVisibilityFractionForSuspicion;
    const float T = (Denominator > KINDA_SMALL_NUMBER)
        ? FMath::Clamp((Vis.VisibilityFraction - Config->MinVisibilityFractionForSuspicion) / Denominator, 0.0f, 1.0f)
        : 1.0f;

    float Multiplier = FMath::Lerp(
        Config->VisibilitySuspicionMinMultiplier,
        Config->VisibilitySuspicionMaxMultiplier,
        T);

    if (!Config->bCorePointGivesFullMultiplier && Vis.bAnyCorePointVisible)
    {
        Multiplier = FMath::Max(Multiplier, Config->CorePointVisibleMultiplier);
    }

    return Multiplier;
}
