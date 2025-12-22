#include "SOTS_AIPerceptionComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_AIGuardPerceptionDataAsset.h"
#include "SOTS_AIPerceptionConfig.h"
#include "SOTS_AIPerceptionSubsystem.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_GameplayTagManagerSubsystem.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "SOTS_TagAccessHelpers.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogSOTS_AIPerception);
DEFINE_LOG_CATEGORY(LogSOTS_AIPerceptionTelemetry);

namespace SOTS_AIPerceptionBBKeys
{
    const FName TargetActor(TEXT("TargetActor"));
    const FName HasLOSToTarget(TEXT("HasLOSToTarget"));
    const FName LastKnownTargetLocation(TEXT("LastKnownTargetLocation"));
    const FName Awareness(TEXT("Awareness"));
    const FName PerceptionState(TEXT("PerceptionState"));
}

enum class ESOTS_InstigatorRelation : uint8
{
    Self,
    Friendly,
    Neutral,
    Hostile,
    Unknown
};

static ESOTS_InstigatorRelation DetermineInstigatorRelation(const AActor* Instigator, const AActor* Owner)
{
    if (!Owner || !Instigator)
    {
        return ESOTS_InstigatorRelation::Unknown;
    }

    if (Instigator == Owner)
    {
        return ESOTS_InstigatorRelation::Self;
    }

    const IGenericTeamAgentInterface* InstigatorTeam = Cast<IGenericTeamAgentInterface>(Instigator);
    const IGenericTeamAgentInterface* OwnerTeam = Cast<IGenericTeamAgentInterface>(Owner);

    if (InstigatorTeam && OwnerTeam)
    {
        const FGenericTeamId InstigatorId = InstigatorTeam->GetGenericTeamId();
        const FGenericTeamId OwnerId = OwnerTeam->GetGenericTeamId();

        if (InstigatorId == OwnerId)
        {
            return ESOTS_InstigatorRelation::Friendly;
        }

        return ESOTS_InstigatorRelation::Hostile;
    }

    return ESOTS_InstigatorRelation::Unknown;
}

USOTS_AIPerceptionComponent::USOTS_AIPerceptionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PerceptionUpdateInterval = 0.2f;

    CurrentState = ESOTS_PerceptionState::Unaware;
    PreviousState = ESOTS_PerceptionState::Unaware;

    CurrentSuspicion = 0.0f;
    PreviousSuspicion = 0.0f;
    bPerceptionSuppressed = false;
    bIsDetected = false;
    PendingHearingStrength = 0.0f;
    LastSuspicionNormalized = 0.0f;
    LastSuspicionEventValue = -1.0f;
}

void USOTS_AIPerceptionComponent::BeginPlay()
{
    Super::BeginPlay();

    ResetPerceptionState(ESOTS_AIPerceptionResetReason::BeginPlay);
    InitializePerceptionTimer();

    if (AActor* OwnerActor = GetOwner())
    {
        if (!bDamageDelegatesBound)
        {
            OwnerActor->OnTakeAnyDamage.AddDynamic(this, &USOTS_AIPerceptionComponent::OnOwnerTakeAnyDamage);
            bDamageDelegatesBound = true;
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (USOTS_AIPerceptionSubsystem* Subsys = World->GetSubsystem<USOTS_AIPerceptionSubsystem>())
        {
            Subsys->RegisterPerceptionComponent(this);
        }
    }
}

void USOTS_AIPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearPerceptionTimers();

    if (bDamageDelegatesBound)
    {
        if (AActor* OwnerActor = GetOwner())
        {
            OwnerActor->OnTakeAnyDamage.RemoveDynamic(this, &USOTS_AIPerceptionComponent::OnOwnerTakeAnyDamage);
        }

        bDamageDelegatesBound = false;
    }

    if (UWorld* World = GetWorld())
    {
        if (USOTS_AIPerceptionSubsystem* Subsys = World->GetSubsystem<USOTS_AIPerceptionSubsystem>())
        {
            Subsys->UnregisterPerceptionComponent(this);
        }
    }

    ResetInternalState(ESOTS_AIPerceptionResetReason::EndPlay, true);

    Super::EndPlay(EndPlayReason);
}

void USOTS_AIPerceptionComponent::OnUnregister()
{
    ClearPerceptionTimers();
    ResetInternalState(ESOTS_AIPerceptionResetReason::EndPlay, true);
    Super::OnUnregister();
}

void USOTS_AIPerceptionComponent::ResetPerceptionState(ESOTS_AIPerceptionResetReason ResetReason)
{
    ResetInternalState(ResetReason, true);
    UpdateBlackboardAndTags();
}

float USOTS_AIPerceptionComponent::GetAwarenessForTarget(AActor* Target) const
{
    if (!Target)
    {
        return 0.0f;
    }

    if (const FSOTS_PerceivedTargetState* State = TargetStates.Find(Target))
    {
        return State->Awareness;
    }

    return 0.0f;
}

bool USOTS_AIPerceptionComponent::HasLineOfSightToTarget(AActor* Target) const
{
    if (!Target)
    {
        return false;
    }

    if (const FSOTS_PerceivedTargetState* State = TargetStates.Find(Target))
    {
        return State->SightScore > 0.0f || State->LastVisiblePoints > 0;
    }

    return false;
}

FSOTS_PerceivedTargetState USOTS_AIPerceptionComponent::GetTargetState(AActor* Target, bool& bFound) const
{
    bFound = false;

    if (!Target)
    {
        return FSOTS_PerceivedTargetState();
    }

    if (const FSOTS_PerceivedTargetState* State = TargetStates.Find(Target))
    {
        bFound = true;
        return *State;
    }

    return FSOTS_PerceivedTargetState();
}

float USOTS_AIPerceptionComponent::GetCurrentSuspicion01() const
{
    if (!FMath::IsFinite(CurrentSuspicion))
    {
        return 0.0f;
    }

    const float MaxSuspicion = (GuardConfig && GuardConfig->Config.MaxSuspicion > 0.0f)
        ? GuardConfig->Config.MaxSuspicion
        : 1.0f;

    if (MaxSuspicion <= 0.0f)
    {
        return 0.0f;
    }

    return FMath::Clamp(CurrentSuspicion / MaxSuspicion, 0.0f, 1.0f);
}

void USOTS_AIPerceptionComponent::InitializePerceptionTimer()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FTimerManager& TimerManager = World->GetTimerManager();
    TimerManager.ClearTimer(PerceptionTimerHandle);

    const float Interval = FMath::Max(PerceptionUpdateInterval, 0.01f);
    TimerManager.SetTimer(PerceptionTimerHandle, this, &USOTS_AIPerceptionComponent::UpdatePerception, Interval, true);
}

void USOTS_AIPerceptionComponent::ClearPerceptionTimers()
{
    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(PerceptionTimerHandle);
        TimerManager.ClearTimer(SuppressionTimerHandle);
    }
}

void USOTS_AIPerceptionComponent::ResetInternalState(ESOTS_AIPerceptionResetReason ResetReason, bool bResetTargets)
{
    LastResetReason = ResetReason;

    bPerceptionSuppressed = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SuppressionTimerHandle);
    }

    if (bResetTargets)
    {
        WatchedActors.Reset();
        TargetStates.Reset();
        PrimaryTarget = nullptr;
    }

    CurrentSuspicion = 0.0f;
    PreviousSuspicion = 0.0f;
    LastStimulusTimeSeconds = 0.0;
    LastDetectionChangeTimeSeconds = 0.0;
    LastDetectionEnterTimeSeconds = 0.0;
    LastSuspicionEventTimeSeconds = 0.0;
    LastSuspicionEventValue = -1.0f;
    bIsDetected = false;
    PendingHearingStrength = 0.0f;
    LastSuspicionNormalized = 0.0f;
    bHasLastDamageStimulus = false;
    LastDamageStimulus = FSOTS_DamageStimulus();
    LastStimulusCache = FSOTS_LastPerceptionStimulus();
    NextAllowedNoiseTimeByTag.Reset();
    NextAllowedDamageTimeByTag.Reset();

    TracesUsedThisUpdate = 0;
    TargetRoundRobinIndex = 0;
    NextShadowCheckTimeSeconds = 0.0;
    ShadowTracesUsedThisSecond = 0;
    ShadowTraceSecondStamp = 0;

    NextTelemetryTimeSeconds = 0.0;

    CachedGSM.Reset();
    LastGSMReportTimeSeconds = 0.0;
    LastReportedSuspicion01 = -1.0f;
    bForceNextGSMReport = false;
    bHasPendingGSMLocation = false;
    PendingGSMLocation = FVector::ZeroVector;
    PendingGSMReasonTag = FGameplayTag();
    LastGSMReasonTag = FGameplayTag();

    StealthCurveCached = nullptr;

    if (CurrentState == ESOTS_PerceptionState::Unaware)
    {
        ApplyPerceptionStateTags(CurrentState);
    }
    else
    {
        SetPerceptionState(ESOTS_PerceptionState::Unaware);
    }

    PreviousState = ESOTS_PerceptionState::Unaware;
    ApplyLOSStateTags(false);
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

void USOTS_AIPerceptionComponent::HandleReportedNoise(const FVector& Location, float Loudness, AActor* InstigatorActor, FGameplayTag NoiseTag)
{
    if (!PerceptionConfig || !GuardConfig)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    const FSOTS_AIGuardPerceptionConfig& Cfg = GuardConfig->Config;

    const ESOTS_InstigatorRelation Relation = DetermineInstigatorRelation(InstigatorActor, Owner);
    const bool bIgnoreSelf = Cfg.bIgnoreSelfNoise && Relation == ESOTS_InstigatorRelation::Self;
    const bool bIgnoreFriendly = Cfg.bIgnoreFriendlyNoise && Relation == ESOTS_InstigatorRelation::Friendly;
    const bool bIgnoreNeutral = Cfg.bIgnoreNeutralNoise && Relation == ESOTS_InstigatorRelation::Neutral;
    const bool bIgnoreUnknown = Cfg.bIgnoreUnknownNoise && Relation == ESOTS_InstigatorRelation::Unknown;

    if (bIgnoreSelf || bIgnoreFriendly || bIgnoreNeutral || bIgnoreUnknown)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (PerceptionConfig->bEnablePerceptionTelemetry || Cfg.bDebugLogAIPerceptionEvents)
        {
            UE_LOG(LogSOTS_AIPerceptionTelemetry, VeryVerbose, TEXT("[AIPerc/Noise] Ignored due to relation %d Instigator=%s"),
                static_cast<int32>(Relation), *GetNameSafe(InstigatorActor));
        }
#endif
        return;
    }

    const FSOTS_NoiseStimulusPolicy* Policy = nullptr;
    if (NoiseTag.IsValid())
    {
        Policy = Cfg.NoisePolicies.Find(NoiseTag);
    }
    if (!Policy)
    {
        Policy = &Cfg.DefaultNoisePolicy;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const double Now = World->GetTimeSeconds();

    // Optional range gate.
    if (Policy->MaxRange > 0.0f)
    {
        const FVector OwnerLoc = Owner->GetActorLocation();
        const FVector RefLoc = InstigatorActor ? InstigatorActor->GetActorLocation() : Location;
        const float Distance = FVector::Dist(OwnerLoc, RefLoc);
        if (Distance > Policy->MaxRange)
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (PerceptionConfig->bEnablePerceptionTelemetry || Cfg.bDebugLogAIPerceptionEvents)
            {
                UE_LOG(LogSOTS_AIPerceptionTelemetry, VeryVerbose, TEXT("[AIPerc/Noise] Ignored range Dist=%.1f Max=%.1f Instigator=%s"),
                    Distance, Policy->MaxRange, *GetNameSafe(InstigatorActor));
            }
#endif
            return;
        }
    }

    // Cooldown gate per tag (empty tag uses default key).
    if (Policy->CooldownSeconds > 0.0)
    {
        const double NextAllowed = NextAllowedNoiseTimeByTag.FindRef(NoiseTag);
        if (NextAllowed > 0.0 && Now < NextAllowed)
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (PerceptionConfig->bEnablePerceptionTelemetry || Cfg.bDebugLogAIPerceptionEvents)
            {
                UE_LOG(LogSOTS_AIPerceptionTelemetry, VeryVerbose, TEXT("[AIPerc/Noise] Ignored cooldown Until=%.2f Now=%.2f Tag=%s"),
                    NextAllowed, Now, *NoiseTag.ToString());
            }
#endif
            return;
        }

        NextAllowedNoiseTimeByTag.FindOrAdd(NoiseTag) = Now + Policy->CooldownSeconds;
    }

    AActor* PrimaryTargetActor = ResolvePrimaryTarget();
    if (PrimaryTargetActor)
    {
        FSOTS_PerceivedTargetState& State = TargetStates.FindOrAdd(PrimaryTargetActor);
        State.Target = PrimaryTargetActor;
        const float HearingDelta = FMath::Clamp(Policy->SuspicionDelta * Loudness * Policy->LoudnessScale, 0.0f, 1.0f);
        State.HearingScore = FMath::Clamp(State.HearingScore + HearingDelta, 0.0f, 1.0f);
    }

    const float MaxSuspicion = (Cfg.MaxSuspicion > 0.0f) ? Cfg.MaxSuspicion : 1.0f;
    const float CurrentNorm = (MaxSuspicion > 0.0f) ? FMath::Clamp(CurrentSuspicion / MaxSuspicion, 0.0f, 1.0f) : 0.0f;
    const float DeltaNorm = FMath::Clamp(Policy->SuspicionDelta * Loudness * Policy->LoudnessScale, 0.0f, 1.0f);
    const float NewNorm = FMath::Clamp(CurrentNorm + DeltaNorm, 0.0f, 1.0f);
    CurrentSuspicion = NewNorm * MaxSuspicion;

    const FGameplayTag ReasonTag = Policy->OverrideReasonTag.IsValid()
        ? Policy->OverrideReasonTag
        : (Cfg.ReasonTag_Hearing.IsValid() ? Cfg.ReasonTag_Hearing : Cfg.ReasonTag_Generic);

    UpdateLastStimulusCache(ESOTS_LocalSense::Hearing, DeltaNorm, Location, InstigatorActor ? InstigatorActor : ResolvePrimaryTarget(), true);

    PendingGSMLocation = Location;
    bHasPendingGSMLocation = true;
    PendingGSMReasonTag = ReasonTag;

    ReportSuspicionToGSM(NewNorm, ReasonTag, &Location, /*bForceReport=*/true, InstigatorActor);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (PerceptionConfig->bEnablePerceptionTelemetry || Cfg.bDebugLogAIPerceptionEvents)
    {
        UE_LOG(LogSOTS_AIPerceptionTelemetry, VeryVerbose,
            TEXT("[AIPerc/Noise] Applied Delta=%.3f NewSusp=%.3f Tag=%s Instigator=%s Loc=(%.1f,%.1f,%.1f)"),
            DeltaNorm,
            NewNorm,
            *NoiseTag.ToString(),
            *GetNameSafe(InstigatorActor),
            Location.X, Location.Y, Location.Z);
    }
#endif
}

void USOTS_AIPerceptionComponent::OnOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    (void)DamageType;

    AActor* InstigatorActor = DamageCauser;

    if (!InstigatorActor && InstigatedBy)
    {
        InstigatorActor = InstigatedBy->GetPawn();
    }

    FVector Location = FVector::ZeroVector;
    bool bHasLocation = false;

    if (DamageCauser)
    {
        Location = DamageCauser->GetActorLocation();
        bHasLocation = true;
    }
    else if (APawn* InstigatorPawn = InstigatedBy ? InstigatedBy->GetPawn() : nullptr)
    {
        Location = InstigatorPawn->GetActorLocation();
        bHasLocation = true;
    }

    ApplyDamageStimulus(InstigatorActor, Damage, FGameplayTag(), Location, bHasLocation);
}

void USOTS_AIPerceptionComponent::ApplyDamageStimulus(AActor* InstigatorActor, float DamageAmount, FGameplayTag DamageTag, const FVector& Location, bool bHasLocation)
{
    if (!PerceptionConfig || !GuardConfig)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    const FSOTS_AIGuardPerceptionConfig& Cfg = GuardConfig->Config;

    const FSOTS_DamageStimulusPolicy* Policy = nullptr;
    if (DamageTag.IsValid())
    {
        Policy = Cfg.DamagePolicies.Find(DamageTag);
    }
    if (!Policy)
    {
        Policy = &Cfg.DefaultDamagePolicy;
    }

    const ESOTS_InstigatorRelation Relation = DetermineInstigatorRelation(InstigatorActor, Owner);
    const bool bIgnoreSelf = Policy->bIgnoreSelf && Relation == ESOTS_InstigatorRelation::Self;
    const bool bIgnoreFriendly = Policy->bIgnoreFriendly && Relation == ESOTS_InstigatorRelation::Friendly;
    const bool bIgnoreNeutral = Policy->bIgnoreNeutral && Relation == ESOTS_InstigatorRelation::Neutral;
    const bool bIgnoreUnknown = Policy->bIgnoreUnknown && Relation == ESOTS_InstigatorRelation::Unknown;

    if (bIgnoreSelf || bIgnoreFriendly || bIgnoreNeutral || bIgnoreUnknown)
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (PerceptionConfig->bEnablePerceptionTelemetry || Cfg.bDebugLogAIPerceptionEvents)
        {
            UE_LOG(LogSOTS_AIPerceptionTelemetry, VeryVerbose, TEXT("[AIPerc/Damage] Ignored due to relation %d Instigator=%s"),
                static_cast<int32>(Relation), *GetNameSafe(InstigatorActor));
        }
#endif
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const double Now = World->GetTimeSeconds();

    if (Policy->MaxRange > 0.0f)
    {
        const FVector OwnerLoc = Owner->GetActorLocation();
        float Distance = 0.0f;
        bool bHasDistance = false;

        if (InstigatorActor)
        {
            Distance = FVector::Dist(OwnerLoc, InstigatorActor->GetActorLocation());
            bHasDistance = true;
        }
        else if (bHasLocation)
        {
            Distance = FVector::Dist(OwnerLoc, Location);
            bHasDistance = true;
        }

        if (bHasDistance && Distance > Policy->MaxRange)
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (PerceptionConfig->bEnablePerceptionTelemetry || Cfg.bDebugLogAIPerceptionEvents)
            {
                UE_LOG(LogSOTS_AIPerceptionTelemetry, VeryVerbose, TEXT("[AIPerc/Damage] Ignored range Dist=%.1f Max=%.1f Instigator=%s"),
                    Distance, Policy->MaxRange, *GetNameSafe(InstigatorActor));
            }
#endif
            return;
        }
    }

    if (Policy->CooldownSeconds > 0.0)
    {
        const double NextAllowed = NextAllowedDamageTimeByTag.FindRef(DamageTag);
        if (NextAllowed > 0.0 && Now < NextAllowed)
        {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (PerceptionConfig->bEnablePerceptionTelemetry || Cfg.bDebugLogAIPerceptionEvents)
            {
                UE_LOG(LogSOTS_AIPerceptionTelemetry, VeryVerbose, TEXT("[AIPerc/Damage] Ignored cooldown Until=%.2f Now=%.2f Tag=%s"),
                    NextAllowed, Now, *DamageTag.ToString());
            }
#endif
            return;
        }

        NextAllowedDamageTimeByTag.FindOrAdd(DamageTag) = Now + Policy->CooldownSeconds;
    }

    float Impulse = Policy->SuspicionImpulse;
    if (Policy->bScaleByDamageAmount)
    {
        const float Denominator = FMath::Max(Policy->SeverityRefDamage, KINDA_SMALL_NUMBER);
        const float Scale = FMath::Clamp(DamageAmount / Denominator, 0.0f, Policy->MaxSeverityScale);
        Impulse *= Scale;
    }

    const float MaxSuspicion = (Cfg.MaxSuspicion > 0.0f) ? Cfg.MaxSuspicion : 1.0f;
    const float CurrentNorm = (MaxSuspicion > 0.0f) ? FMath::Clamp(CurrentSuspicion / MaxSuspicion, 0.0f, 1.0f) : 0.0f;
    const float NewNorm = FMath::Clamp(CurrentNorm + Impulse, 0.0f, 1.0f);
    CurrentSuspicion = NewNorm * MaxSuspicion;
    LastStimulusTimeSeconds = Now;

    FGameplayTag ReasonTag = Policy->OverrideReasonTag.IsValid()
        ? Policy->OverrideReasonTag
        : (Cfg.ReasonTag_Damage.IsValid() ? Cfg.ReasonTag_Damage : Cfg.ReasonTag_Generic);

    FVector ResolvedLocation = Location;
    bool bResolvedHasLocation = bHasLocation;

    if (!bResolvedHasLocation && InstigatorActor)
    {
        ResolvedLocation = InstigatorActor->GetActorLocation();
        bResolvedHasLocation = true;
    }

    UpdateLastStimulusCache(ESOTS_LocalSense::Damage, Impulse, ResolvedLocation, InstigatorActor ? InstigatorActor : ResolvePrimaryTarget(), true);

    if (bResolvedHasLocation)
    {
        PendingGSMLocation = ResolvedLocation;
        bHasPendingGSMLocation = true;
    }

    PendingGSMReasonTag = ReasonTag;

    ReportSuspicionToGSM(NewNorm, ReasonTag, bResolvedHasLocation ? &ResolvedLocation : nullptr, Policy->bAlwaysReportToGSM, InstigatorActor);

    if (Policy->bForceMinPerceptionState)
    {
        const int32 ClampedStateIndex = FMath::Clamp(Policy->MinPerceptionState, static_cast<int32>(ESOTS_PerceptionState::Unaware), static_cast<int32>(ESOTS_PerceptionState::Alerted));
        const ESOTS_PerceptionState MinState = static_cast<ESOTS_PerceptionState>(ClampedStateIndex);
        if (static_cast<int32>(CurrentState) < ClampedStateIndex)
        {
            SetPerceptionState(MinState);
        }
    }

    LastDamageStimulus.VictimActor = Owner;
    LastDamageStimulus.InstigatorActor = InstigatorActor;
    LastDamageStimulus.DamageAmount = DamageAmount;
    LastDamageStimulus.DamageTag = DamageTag;
    LastDamageStimulus.bHasLocation = bResolvedHasLocation;
    LastDamageStimulus.Location = ResolvedLocation;
    LastDamageStimulus.TimestampSeconds = Now;
    bHasLastDamageStimulus = true;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (PerceptionConfig->bEnablePerceptionTelemetry || Cfg.bDebugLogAIPerceptionEvents)
    {
        UE_LOG(LogSOTS_AIPerceptionTelemetry, VeryVerbose,
            TEXT("[AIPerc/Damage] Applied Impulse=%.3f NewSusp=%.3f Tag=%s Instigator=%s LocValid=%s Loc=(%.1f,%.1f,%.1f)"),
            Impulse,
            NewNorm,
            *DamageTag.ToString(),
            *GetNameSafe(InstigatorActor),
            bResolvedHasLocation ? TEXT("true") : TEXT("false"),
            ResolvedLocation.X, ResolvedLocation.Y, ResolvedLocation.Z);
    }
#endif
}

void USOTS_AIPerceptionComponent::RefreshWatchedTargets()
{
    WatchedActors.Reset();

    if (AActor* PrimaryTargetActor = ResolvePrimaryTarget())
    {
        WatchedActors.Add(PrimaryTargetActor);
        TargetStates.FindOrAdd(PrimaryTargetActor).Target = PrimaryTargetActor;
    }
    else if (UWorld* World = GetWorld())
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
                    PrimaryTarget = Actor;
                    WatchedActors.Add(Actor);
                    TargetStates.FindOrAdd(Actor).Target = Actor;
                    break;
                }
            }

            if (PrimaryTarget.IsValid())
            {
                break;
            }
        }
    }

    for (auto It = TargetStates.CreateIterator(); It; ++It)
    {
        if (!IsPrimaryTarget(It.Key().Get()))
        {
            It.RemoveCurrent();
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

    if (WatchedActors.Num() == 0)
    {
        CurrentSuspicion = FMath::Max(0.0f, CurrentSuspicion - PerceptionConfig->SuspicionDecayPerSecond * DeltaSeconds);
        if (GuardConfig)
        {
            const FSOTS_AIGuardPerceptionConfig& Cfg = GuardConfig->Config;
            const FGameplayTag ReasonTag = Cfg.ReasonTag_Generic.IsValid() ? Cfg.ReasonTag_Generic : FGameplayTag();
            ReportSuspicionToGSM(GetCurrentSuspicion01(), ReasonTag, nullptr, /*bForceReport=*/false, nullptr);
        }
        return;
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

        float SightStrength = 0.0f;
        float HearingStrength = PendingHearingStrength;
        float ShadowStrength = 0.0f;
        ESOTS_LocalSense DominantSenseHint = ESOTS_LocalSense::Unknown;

        FGameplayTag AccumulatedReasonTag = PendingGSMReasonTag;
        bool bForceReportThisFrame = bForceNextGSMReport;

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

        // Sight stimulus: normalize using target-point visibility if available.
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

            SightStrength = FMath::Clamp(VisibilityMultiplier, 0.0f, 1.0f);
            DominantSenseHint = ESOTS_LocalSense::Sight;

            if (!AccumulatedReasonTag.IsValid())
            {
                AccumulatedReasonTag = Cfg.ReasonTag_Sight.IsValid() ? Cfg.ReasonTag_Sight : Cfg.ReasonTag_Generic;
            }

            if (AActor* SightActor = BestState->Target.Get())
            {
                UpdateLastStimulusCache(ESOTS_LocalSense::Sight, SightStrength, BestState->LastKnownLocation, SightActor, true);
            }

            bHasLOSOnPrimary = true;
        }

        // Optional shadow awareness (secondary stimulus via GSM cached shadow point; never enumerates lights).
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
                                        ShadowStrength = 1.0f;
                                        DominantSenseHint = (SightStrength > ShadowStrength) ? DominantSenseHint : ESOTS_LocalSense::Shadow;

                                        if (!AccumulatedReasonTag.IsValid())
                                        {
                                            AccumulatedReasonTag = Cfg.ReasonTag_Shadow.IsValid() ? Cfg.ReasonTag_Shadow : Cfg.ReasonTag_Generic;
                                        }

                                        UpdateLastStimulusCache(ESOTS_LocalSense::Shadow, ShadowStrength, Cand.ShadowPointWS, PlayerActor, true);
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

        // Centralized suspicion update (ramps, decay, hysteresis, clamps).
        bool bDetectedChanged = false;
        bool bDetectedNow = false;
        SuspicionNormalized = UpdateSuspicionModel(
            DeltaSeconds,
            SightStrength,
            HearingStrength,
            ShadowStrength,
            DominantSenseHint,
            AccumulatedReasonTag,
            bDetectedChanged,
            bDetectedNow);

        // Consume one-shot hearing stimulus after application.
        PendingHearingStrength = 0.0f;

        if (HighestState != CurrentState)
        {
            bForceReportThisFrame = true;
        }

        if (!AccumulatedReasonTag.IsValid())
        {
            AccumulatedReasonTag = Cfg.ReasonTag_Generic;
        }

        const FVector* ReportLocationPtr = bHasPendingGSMLocation ? &PendingGSMLocation : nullptr;
        ReportSuspicionToGSM(SuspicionNormalized, AccumulatedReasonTag, ReportLocationPtr, bForceReportThisFrame, nullptr);

        bForceNextGSMReport = false;
        bHasPendingGSMLocation = false;
        PendingGSMReasonTag = FGameplayTag();

        // Tag transitions: fully alerted / lost sight thresholds.
        const float MaxSuspicion = (Cfg.MaxSuspicion > 0.0f) ? Cfg.MaxSuspicion : 1.0f;
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

    LogTelemetrySnapshot(NumTargets, EvaluatedTargetsCount, MaxTargetsToEvaluate, bHasLOSOnPrimary, BestState, SuspicionNormalized);

    // Optional debug correlation between local suspicion and GSM aggregate.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDebugSuspicion)
    {
        AActor* OwnerActor = GetOwner();
        float GlobalAISusp01 = 0.0f;

        if (USOTS_GlobalStealthManagerSubsystem* GSM = USOTS_GlobalStealthManagerSubsystem::Get(this))
        {
            GlobalAISusp01 = GSM->GetCurrentStealthBreakdown().AISuspicion01;
        }

        UE_LOG(LogSOTS_AIPerceptionTelemetry, Verbose,
            TEXT("[AIPerc] Guard=%s Susp=%.2f Norm=%.2f GlobalAISusp=%.2f State=%d HasLOS=%s"),
            *GetNameSafe(OwnerActor),
            CurrentSuspicion,
            SuspicionNormalized,
            GlobalAISusp01,
            static_cast<int32>(CurrentState),
            bHasLOSOnPrimary ? TEXT("true") : TEXT("false"));
    }
#endif

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

void USOTS_AIPerceptionComponent::WriteBlackboardSnapshot(
    UBlackboardComponent* BlackboardComp,
    AActor* PrimaryTargetActor,
    bool bHasLOSOnPrimary,
    const FVector& LastKnownLocation,
    bool bHasValidLastKnownLocation,
    float SuspicionNormalized,
    float PrimaryTargetAwareness,
    ESOTS_PerceptionState PerceptionState)
{
    if (!ensureMsgf(BlackboardComp != nullptr, TEXT("AIPerception: Null BlackboardComp on %s"), *GetNameSafe(GetOwner())))
    {
        return;
    }

    const FSOTS_AIPerceptionBlackboardConfig* BB = PerceptionConfig ? &PerceptionConfig->BlackboardConfig : nullptr;
    const auto HasSelector = [](const FBlackboardKeySelector& Selector)
    {
        return Selector.SelectedKeyName != NAME_None;
    };

    const bool bHasTargetSelector = BB && HasSelector(BB->TargetActorKey);
    const bool bHasSuspicionSelector = BB && HasSelector(BB->SuspicionKey);
    const bool bHasStateSelector = BB && HasSelector(BB->StateKey);
    const bool bHasLOSSelector = BB && HasSelector(BB->HasLOSKey);
    const bool bHasLocationSelector = BB && HasSelector(BB->LastKnownLocationKey);
    const bool bHasLocationValidSelector = BB && HasSelector(BB->LastKnownLocationValidKey);

    bool bUsedLegacyFallback = false;
    bool bMissingSelector = false;

    if (bHasTargetSelector)
    {
        BlackboardComp->SetValueAsObject(BB->TargetActorKey.SelectedKeyName, PrimaryTargetActor);
    }
    else
    {
        BlackboardComp->SetValueAsObject(SOTS_AIPerceptionBBKeys::TargetActor, PrimaryTargetActor);
        bUsedLegacyFallback = true;
        bMissingSelector = true;
    }

    if (bHasLOSSelector)
    {
        BlackboardComp->SetValueAsBool(BB->HasLOSKey.SelectedKeyName, bHasLOSOnPrimary);
    }
    else
    {
        BlackboardComp->SetValueAsBool(SOTS_AIPerceptionBBKeys::HasLOSToTarget, bHasLOSOnPrimary);
        bUsedLegacyFallback = true;
        bMissingSelector = true;
    }

    const FVector LocationToWrite = bHasValidLastKnownLocation ? LastKnownLocation : FVector::ZeroVector;
    if (bHasLocationSelector)
    {
        BlackboardComp->SetValueAsVector(BB->LastKnownLocationKey.SelectedKeyName, LocationToWrite);
    }
    else
    {
        BlackboardComp->SetValueAsVector(SOTS_AIPerceptionBBKeys::LastKnownTargetLocation, LocationToWrite);
        bUsedLegacyFallback = true;
        bMissingSelector = true;
    }

    if (bHasLocationValidSelector)
    {
        BlackboardComp->SetValueAsBool(BB->LastKnownLocationValidKey.SelectedKeyName, bHasValidLastKnownLocation);
    }
    else if (!bHasValidLastKnownLocation)
    {
        // Ensure legacy consumers don't act on stale vectors by clearing when invalid.
        BlackboardComp->SetValueAsVector(SOTS_AIPerceptionBBKeys::LastKnownTargetLocation, FVector::ZeroVector);
    }

    const float ClampedSuspicion = FMath::Clamp(SuspicionNormalized, 0.0f, 1.0f);
    const float ClampedAwareness = FMath::Clamp(PrimaryTargetAwareness, 0.0f, 1.0f);

    if (bHasSuspicionSelector)
    {
        BlackboardComp->SetValueAsFloat(BB->SuspicionKey.SelectedKeyName, ClampedSuspicion);
    }
    else
    {
        BlackboardComp->SetValueAsFloat(SOTS_AIPerceptionBBKeys::Awareness, ClampedAwareness);
        bUsedLegacyFallback = true;
        bMissingSelector = true;
    }

    if (bHasStateSelector)
    {
        BlackboardComp->SetValueAsInt(BB->StateKey.SelectedKeyName, static_cast<int32>(PerceptionState));
    }
    else
    {
        BlackboardComp->SetValueAsEnum(SOTS_AIPerceptionBBKeys::PerceptionState, static_cast<uint8>(PerceptionState));
        bUsedLegacyFallback = true;
        bMissingSelector = true;
    }

    if ((bUsedLegacyFallback || bMissingSelector) && !bWarnedAboutLegacyBBFallback)
    {
        UE_LOG(LogSOTS_AIPerception, Warning, TEXT("AIPerception: Blackboard selectors missing on %s; writing legacy BB keys."), *GetNameSafe(GetOwner()));
        bWarnedAboutLegacyBBFallback = true;
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

    AActor* BestTargetActor = (BestState && BestState->Target.IsValid()) ? BestState->Target.Get() : nullptr;
    const bool bHasValidTarget = BestTargetActor != nullptr;
    const bool bHasLOS = bHasValidTarget && BestState->SightScore > 0.0f;
    const FVector LastKnownLocation = bHasValidTarget ? BestState->LastKnownLocation : FVector::ZeroVector;
    const float SuspicionNormalized = GetCurrentSuspicion01();
    const float PrimaryTargetAwareness = BestState ? BestState->Awareness : 0.0f;

    WriteBlackboardSnapshot(
        BlackboardComp,
        BestTargetActor,
        bHasLOS,
        LastKnownLocation,
        bHasValidTarget,
        SuspicionNormalized,
        PrimaryTargetAwareness,
        CurrentState);

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
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
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
#else
    (void)NumTargets;
    (void)EvaluatedTargetsCount;
    (void)MaxTargetsToEvaluate;
    (void)bHasLOSOnPrimary;
    (void)BestState;
    (void)SuspicionNormalized;
#endif
}

AActor* USOTS_AIPerceptionComponent::ResolvePrimaryTarget()
{
    if (PrimaryTarget.IsValid())
    {
        return PrimaryTarget.Get();
    }

    if (UWorld* World = GetWorld())
    {
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
        {
            PrimaryTarget = PlayerPawn;
            return PlayerPawn;
        }
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (GuardConfig && GuardConfig->Config.bDebugLogAIPerceptionEvents)
    {
        UE_LOG(LogSOTS_AIPerception, Verbose, TEXT("[AIPerc] Primary target not resolved for %s"), *GetNameSafe(GetOwner()));
    }
#endif

    return nullptr;
}

bool USOTS_AIPerceptionComponent::IsPrimaryTarget(const AActor* Actor) const
{
    return PrimaryTarget.IsValid() && Actor == PrimaryTarget.Get();
}

void USOTS_AIPerceptionComponent::UpdateLastStimulusCache(ESOTS_LocalSense Sense, float Strength01, const FVector& WorldLocation, AActor* SourceActor, bool bSuccessfullySensed)
{
    if (!SourceActor || !IsPrimaryTarget(SourceActor))
    {
        return;
    }

    UWorld* World = GetWorld();
    const double Now = World ? World->GetTimeSeconds() : 0.0;
    const bool bSameTimestamp = FMath::IsNearlyEqual(static_cast<float>(Now), static_cast<float>(LastStimulusCache.TimestampSeconds), KINDA_SMALL_NUMBER);

    const bool bPreferThis = bSuccessfullySensed
        ? (!LastStimulusCache.bSuccessfullySensed || !bSameTimestamp || Strength01 >= LastStimulusCache.Strength01)
        : (!LastStimulusCache.bSuccessfullySensed && (!bSameTimestamp || Strength01 >= LastStimulusCache.Strength01));

    if (!bPreferThis)
    {
        return;
    }

    LastStimulusCache.Strength01 = Strength01;
    LastStimulusCache.SenseTag = MapSenseToReasonTag(Sense);
    LastStimulusCache.WorldLocation = WorldLocation;
    LastStimulusCache.SourceActor = SourceActor;
    LastStimulusCache.TimestampSeconds = Now;
    LastStimulusCache.bSuccessfullySensed = bSuccessfullySensed;
}

FGameplayTag USOTS_AIPerceptionComponent::MapSenseToReasonTag(ESOTS_LocalSense Sense) const
{
    if (!GuardConfig)
    {
        return FGameplayTag();
    }

    const FSOTS_AIGuardPerceptionConfig& Cfg = GuardConfig->Config;

    switch (Sense)
    {
    case ESOTS_LocalSense::Sight:
        return Cfg.ReasonTag_Sight.IsValid() ? Cfg.ReasonTag_Sight : Cfg.ReasonTag_Generic;
    case ESOTS_LocalSense::Hearing:
        return Cfg.ReasonTag_Hearing.IsValid() ? Cfg.ReasonTag_Hearing : Cfg.ReasonTag_Generic;
    case ESOTS_LocalSense::Shadow:
        return Cfg.ReasonTag_Shadow.IsValid() ? Cfg.ReasonTag_Shadow : Cfg.ReasonTag_Generic;
    case ESOTS_LocalSense::Damage:
        return Cfg.ReasonTag_Damage.IsValid() ? Cfg.ReasonTag_Damage : Cfg.ReasonTag_Generic;
    default:
        return Cfg.ReasonTag_Generic;
    }
}

void USOTS_AIPerceptionComponent::EvaluateStimuliForTarget(float SightStrength, float HearingStrength, float ShadowStrength, ESOTS_LocalSense& OutDominantSense, float& OutDominantStrength, bool& bOutHasStimulus) const
{
    OutDominantSense = ESOTS_LocalSense::Unknown;
    OutDominantStrength = 0.0f;

    const auto ConsiderSense = [&](ESOTS_LocalSense Sense, float Strength)
    {
        if (Strength > OutDominantStrength)
        {
            OutDominantStrength = Strength;
            OutDominantSense = Sense;
        }
    };

    ConsiderSense(ESOTS_LocalSense::Sight, SightStrength);
    ConsiderSense(ESOTS_LocalSense::Hearing, HearingStrength);
    ConsiderSense(ESOTS_LocalSense::Shadow, ShadowStrength);

    bOutHasStimulus = OutDominantStrength > 0.0f;
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

float USOTS_AIPerceptionComponent::UpdateSuspicionModel(
    float DeltaSeconds,
    float SightStrength,
    float HearingStrength,
    float ShadowStrength,
    ESOTS_LocalSense DominantSense,
    FGameplayTag& OutDominantReasonTag,
    bool& bOutDetectedChanged,
    bool& bOutDetectedNow)
{
    bOutDetectedChanged = false;
    bOutDetectedNow = bIsDetected;
    OutDominantReasonTag = FGameplayTag();

    if (!GuardConfig)
    {
        return 0.0f;
    }

    const FSOTS_AIGuardPerceptionConfig& Cfg = GuardConfig->Config;

    UWorld* World = GetWorld();
    const double NowSeconds = World ? World->GetTimeSeconds() : 0.0;

    const bool bHasStimulus = (SightStrength > 0.0f) || (HearingStrength > 0.0f) || (ShadowStrength > 0.0f);

    // Dominant sense selection for reason tagging and ramp choice.
    float DominantStrength = 0.0f;
    float RampRatePerSecond = 0.0f;

    switch (DominantSense)
    {
    case ESOTS_LocalSense::Sight:
        DominantStrength = SightStrength;
        RampRatePerSecond = Cfg.RampUpPerSecond_Sight;
        break;
    case ESOTS_LocalSense::Hearing:
        DominantStrength = HearingStrength;
        RampRatePerSecond = Cfg.RampUpPerSecond_Hearing;
        break;
    case ESOTS_LocalSense::Shadow:
        DominantStrength = ShadowStrength;
        RampRatePerSecond = Cfg.RampUpPerSecond_Shadow;
        break;
    case ESOTS_LocalSense::Damage:
        DominantStrength = 1.0f;
        RampRatePerSecond = Cfg.RampUpPerSecond_Damage;
        break;
    default:
        if (SightStrength >= HearingStrength && SightStrength >= ShadowStrength)
        {
            DominantSense = ESOTS_LocalSense::Sight;
            DominantStrength = SightStrength;
            RampRatePerSecond = Cfg.RampUpPerSecond_Sight;
        }
        else if (HearingStrength >= ShadowStrength)
        {
            DominantSense = ESOTS_LocalSense::Hearing;
            DominantStrength = HearingStrength;
            RampRatePerSecond = Cfg.RampUpPerSecond_Hearing;
        }
        else
        {
            DominantSense = ESOTS_LocalSense::Shadow;
            DominantStrength = ShadowStrength;
            RampRatePerSecond = Cfg.RampUpPerSecond_Shadow;
        }
        break;
    }

    OutDominantReasonTag = MapSenseToReasonTag(DominantSense);

    if (bHasStimulus)
    {
        LastStimulusTimeSeconds = NowSeconds;
        const float RampDelta = RampRatePerSecond * FMath::Clamp(DominantStrength, 0.0f, 1.0f) * DeltaSeconds;
        CurrentSuspicion += RampDelta;
    }
    else
    {
        const double SinceStimulus = NowSeconds - LastStimulusTimeSeconds;
        if (SinceStimulus >= Cfg.StimulusForgetDelaySeconds)
        {
            CurrentSuspicion -= Cfg.DecayPerSecond * DeltaSeconds;
        }
    }

    const float MaxSuspicion = (Cfg.MaxSuspicion > 0.0f) ? Cfg.MaxSuspicion : 1.0f;

    if (!FMath::IsFinite(CurrentSuspicion))
    {
        CurrentSuspicion = 0.0f;
    }

    CurrentSuspicion = FMath::Clamp(CurrentSuspicion, 0.0f, MaxSuspicion);

    const float SuspicionNormalized = (MaxSuspicion > 0.0f)
        ? FMath::Clamp(CurrentSuspicion / MaxSuspicion, 0.0f, 1.0f)
        : 0.0f;

    LastSuspicionNormalized = SuspicionNormalized;

    // Detection hysteresis and min-time gates.
    const float SpottedThreshold = FMath::Clamp(Cfg.SpottedThreshold01, 0.0f, 1.0f);
    const float LostThreshold = Cfg.bEnableHysteresis
        ? FMath::Clamp(Cfg.LostThreshold01, 0.0f, SpottedThreshold)
        : SpottedThreshold;

    const double SinceLastChange = NowSeconds - LastDetectionChangeTimeSeconds;
    const double SinceEnter = NowSeconds - LastDetectionEnterTimeSeconds;

    if (!bIsDetected)
    {
        if (SuspicionNormalized >= SpottedThreshold)
        {
            const bool bPassMinBetween = Cfg.MinSecondsBetweenStateFlips <= 0.0 || SinceLastChange >= Cfg.MinSecondsBetweenStateFlips;
            if (bPassMinBetween)
            {
                bIsDetected = true;
                bOutDetectedChanged = true;
                bOutDetectedNow = true;
                LastDetectionChangeTimeSeconds = NowSeconds;
                LastDetectionEnterTimeSeconds = NowSeconds;
                OnTargetSpotted.Broadcast();
            }
        }
    }
    else
    {
        const bool bPassMinBetween = Cfg.MinSecondsBetweenStateFlips <= 0.0 || SinceLastChange >= Cfg.MinSecondsBetweenStateFlips;
        const bool bPassMinDwell = Cfg.MinSecondsInSpottedState <= 0.0 || SinceEnter >= Cfg.MinSecondsInSpottedState;

        if (SuspicionNormalized <= LostThreshold && bPassMinBetween && bPassMinDwell)
        {
            bIsDetected = false;
            bOutDetectedChanged = true;
            bOutDetectedNow = false;
            LastDetectionChangeTimeSeconds = NowSeconds;
            OnTargetLost.Broadcast();
        }
    }

    // Suspicion change event throttling.
    const float Epsilon = FMath::Clamp(Cfg.SuspicionChangeEpsilon01, 0.0f, 1.0f);
    const double EventInterval = FMath::Max(Cfg.SuspicionEventMinIntervalSeconds, 0.0f);
    const double SinceLastEvent = NowSeconds - LastSuspicionEventTimeSeconds;

    const bool bShouldBroadcastSuspicion = (SinceLastEvent >= EventInterval) && (FMath::Abs(SuspicionNormalized - LastSuspicionEventValue) >= Epsilon);

    if (bShouldBroadcastSuspicion)
    {
        OnSuspicionChanged.Broadcast(SuspicionNormalized);
        LastSuspicionEventValue = SuspicionNormalized;
        LastSuspicionEventTimeSeconds = NowSeconds;
    }

    const FSOTS_AIPerceptionTelemetry Telemetry = BuildTelemetrySnapshot(SuspicionNormalized, OutDominantReasonTag);

    if (bOutDetectedChanged)
    {
        BroadcastSuspicionTelemetry(Telemetry, true);
    }

    if (bShouldBroadcastSuspicion)
    {
        BroadcastSuspicionTelemetry(Telemetry, false);
    }

    return SuspicionNormalized;
}

FSOTS_AIPerceptionTelemetry USOTS_AIPerceptionComponent::BuildTelemetrySnapshot(float SuspicionNormalized, const FGameplayTag& ReasonTag) const
{
    FSOTS_AIPerceptionTelemetry Telemetry;
    Telemetry.Suspicion01 = SuspicionNormalized;
    Telemetry.bDetected = bIsDetected;
    Telemetry.ReasonSenseTag = ReasonTag.IsValid() ? ReasonTag : LastStimulusCache.SenseTag;
    Telemetry.LastStimulusWorldLocation = LastStimulusCache.WorldLocation;
    Telemetry.LastStimulusActor = LastStimulusCache.SourceActor.Get();
    Telemetry.LastStimulusTimeSeconds = LastStimulusCache.TimestampSeconds;
    return Telemetry;
}

void USOTS_AIPerceptionComponent::BroadcastSuspicionTelemetry(const FSOTS_AIPerceptionTelemetry& Telemetry, bool bDetectedChanged)
{
    if (OnAIPerceptionSuspicionChanged.IsBound())
    {
        OnAIPerceptionSuspicionChanged.Broadcast(Telemetry);
    }

    if (bDetectedChanged)
    {
        if (Telemetry.bDetected)
        {
            OnAIPerceptionSpotted.Broadcast(Telemetry);
        }
        else
        {
            OnAIPerceptionLost.Broadcast(Telemetry);
        }
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (GuardConfig && GuardConfig->Config.bDebugLogAIPerceptionEvents)
    {
        const FString ActorName = Telemetry.LastStimulusActor.IsValid() ? Telemetry.LastStimulusActor->GetName() : TEXT("None");
        UE_LOG(LogSOTS_AIPerception, Verbose, TEXT("[AIPerc/Telem] Susp=%.2f Detected=%s Sense=%s StimActor=%s StimLoc=%s StimTime=%.2f"),
            Telemetry.Suspicion01,
            Telemetry.bDetected ? TEXT("true") : TEXT("false"),
            *Telemetry.ReasonSenseTag.ToString(),
            *ActorName,
            *Telemetry.LastStimulusWorldLocation.ToString(),
            Telemetry.LastStimulusTimeSeconds);
    }
#endif
}

USOTS_GlobalStealthManagerSubsystem* USOTS_AIPerceptionComponent::ResolveGSM()
{
    if (CachedGSM.IsValid())
    {
        return CachedGSM.Get();
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    if (UGameInstance* GI = World->GetGameInstance())
    {
        if (USOTS_GlobalStealthManagerSubsystem* GSM = GI->GetSubsystem<USOTS_GlobalStealthManagerSubsystem>())
        {
            CachedGSM = GSM;
            return GSM;
        }
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (GuardConfig && GuardConfig->Config.bDebugLogMissingGSM)
    {
        UE_LOG(LogSOTS_AIPerceptionTelemetry, Verbose, TEXT("[AIPerc/GSM] Missing GSM for %s"), *GetNameSafe(GetOwner()));
    }
#endif

    return nullptr;
}

void USOTS_AIPerceptionComponent::ReportSuspicionToGSM(
    float AISuspicion01,
    const FGameplayTag& ReasonTag,
    const FVector* OptionalLocation,
    bool bForceReport,
    AActor* InstigatorActorOverride)
{
    if (!GuardConfig || !GuardConfig->Config.bEnableGSMReporting)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float ClampedSuspicion = FMath::Clamp(AISuspicion01, 0.0f, 1.0f);
    const double NowSeconds = World->GetTimeSeconds();

    const float MinDelta = FMath::Max(GuardConfig->Config.GSMReportMinDelta01, 0.0f);
    const double MinInterval = FMath::Max(GuardConfig->Config.GSMReportMinIntervalSeconds, 0.0f);

    const float DeltaSinceLast = FMath::Abs(ClampedSuspicion - LastReportedSuspicion01);
    const double Elapsed = NowSeconds - LastGSMReportTimeSeconds;

    bool bShouldSend = bForceReport;
    if (!bShouldSend)
    {
        bShouldSend = (Elapsed >= MinInterval) && (DeltaSinceLast >= MinDelta);
    }

    if (!bShouldSend)
    {
        return;
    }

    if (USOTS_GlobalStealthManagerSubsystem* GSM = ResolveGSM())
    {
        if (AActor* OwnerActor = GetOwner())
        {
            LastGSMReportTimeSeconds = NowSeconds;
            LastReportedSuspicion01 = ClampedSuspicion;
            LastGSMReasonTag = ReasonTag;

            const bool bHasLocation = OptionalLocation != nullptr;
            const FVector LocationToSend = bHasLocation ? *OptionalLocation : FVector::ZeroVector;
            AActor* InstigatorActor = InstigatorActorOverride
                ? InstigatorActorOverride
                : (PrimaryTarget.IsValid() ? PrimaryTarget.Get() : ResolvePrimaryTarget());

            GSM->ReportAISuspicionEx(OwnerActor, ClampedSuspicion, ReasonTag, LocationToSend, bHasLocation, InstigatorActor);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if (PerceptionConfig && PerceptionConfig->bEnablePerceptionTelemetry)
            {
                const FString ReasonStr = ReasonTag.IsValid() ? ReasonTag.ToString() : TEXT("<None>");
                const FString LocationStr = bHasLocation
                    ? FString::Printf(TEXT(" loc=(%.1f,%.1f,%.1f)"), LocationToSend.X, LocationToSend.Y, LocationToSend.Z)
                    : FString();

                UE_LOG(LogSOTS_AIPerceptionTelemetry, Verbose,
                    TEXT("[AIPerc/GSM] Reported Susp=%.3f Reason=%s%s"),
                    ClampedSuspicion,
                    *ReasonStr,
                    *LocationStr);
            }
#endif
        }
    }
}
