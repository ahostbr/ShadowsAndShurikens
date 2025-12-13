#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_GlobalStealthManagerModule.h"
#include "SOTS_PlayerStealthComponent.h"
#include "DrawDebugHelpers.h"

USOTS_GlobalStealthManagerSubsystem::USOTS_GlobalStealthManagerSubsystem()
    : CurrentStealthScore(0.0f)
    , CurrentStealthLevel(ESOTSStealthLevel::Undetected)
    , bPlayerDetected(false)
{
}

void USOTS_GlobalStealthManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Start with default struct values.
    ActiveConfig = FSOTS_StealthScoringConfig();

    // If a default asset is assigned on the CDO, copy its config.
    if (DefaultConfigAsset)
    {
        ActiveStealthConfig = DefaultConfigAsset;
        ActiveConfig = DefaultConfigAsset->Config;
    }
}

USOTS_GlobalStealthManagerSubsystem* USOTS_GlobalStealthManagerSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<USOTS_GlobalStealthManagerSubsystem>();
}

void USOTS_GlobalStealthManagerSubsystem::ReportStealthSample(const FSOTS_StealthSample& Sample)
{
    LastSample = Sample;

    const FSOTS_StealthScoringConfig& Cfg = ActiveConfig;

    const float LightWeight    = Cfg.LightWeight;
    const float LOSWeight      = Cfg.LOSWeight;
    const float DistanceWeight = Cfg.DistanceWeight;
    const float NoiseWeight    = Cfg.NoiseWeight;
    const float DistanceClamp  = FMath::Max(Cfg.DistanceClamp, 1.0f);

    float Score = 0.0f;

    // Light: 0 (dark) to 1 (fully lit).
    Score += FMath::Clamp(Sample.LightExposure, 0.0f, 1.0f) * LightWeight;

    // Line of sight: hard bump if seen.
    if (Sample.bInEnemyLineOfSight)
    {
        Score += LOSWeight;
    }

    // Distance: closer = more dangerous. Clamp at 0-1 with a falloff range.
    const float DistanceFactor = 1.0f - FMath::Clamp(Sample.DistanceToNearestEnemy / DistanceClamp, 0.0f, 1.0f);
    Score += DistanceFactor * DistanceWeight;

    // Noise.
    Score += FMath::Clamp(Sample.NoiseLevel, 0.0f, 1.0f) * NoiseWeight;

    // Cover: reduce effective score if in cover.
    if (Sample.bInCover)
    {
        Score *= 0.6f;
    }

    Score = FMath::Clamp(Score, 0.0f, 1.0f);

    CurrentStealthScore = Score;

    ESOTSStealthLevel NewLevel = EvaluateStealthLevelFromScore(Score);
    SetStealthLevel(NewLevel);

    // Keep the player-facing state in sync for debug paths that still use
    // ReportStealthSample directly (e.g. GSM debug library).
    CurrentState.GlobalStealthScore01 = Score;
    CurrentState.LightLevel01 = FMath::Clamp(Sample.LightExposure, 0.0f, 1.0f);
    CurrentState.ShadowLevel01 = 1.0f - CurrentState.LightLevel01;
    CurrentState.MovementNoise01 = FMath::Clamp(Sample.NoiseLevel, 0.0f, 1.0f);
    CurrentState.CoverExposure01 = Sample.bInCover ? 0.0f : 1.0f;

    // Map score into the simpler tier enum for UI/KEM.
    ESOTS_StealthTier NewTier = ESOTS_StealthTier::Hidden;
    if (Score >= 0.8f)
    {
        NewTier = ESOTS_StealthTier::Compromised;
    }
    else if (Score >= 0.5f)
    {
        NewTier = ESOTS_StealthTier::Danger;
    }
    else if (Score >= 0.2f)
    {
        NewTier = ESOTS_StealthTier::Cautious;
    }

    CurrentState.StealthTier = NewTier;

    UpdateGameplayTags();
    SyncPlayerStealthComponent();
    OnStealthStateChanged.Broadcast(CurrentState);
    UpdateShadowCandidateForPlayerIfNeeded();

    UE_LOG(LogSOTSGlobalStealth, VeryVerbose,
        TEXT("Stealth sample processed. Score=%.3f, Level=%d"),
        Score,
        static_cast<int32>(NewLevel));
}

void USOTS_GlobalStealthManagerSubsystem::ReportEnemyDetectionEvent(AActor* Source, bool bDetectedNow)
{
    UE_LOG(LogSOTSGlobalStealth, Log, TEXT("Enemy detection event from %s: %s"),
        Source ? *Source->GetName() : TEXT("Unknown"),
        bDetectedNow ? TEXT("DETECTED") : TEXT("LOST"));

    SetPlayerDetected(bDetectedNow);

    // When fully detected, clamp level to FullyDetected.
    if (bDetectedNow)
    {
        SetStealthLevel(ESOTSStealthLevel::FullyDetected);
        CurrentStealthScore = 1.0f;
    }
}

void USOTS_GlobalStealthManagerSubsystem::SetAISuspicion(float In01)
{
    CurrentState.AISuspicion01 = FMath::Clamp(In01, 0.0f, 1.0f);
    RecomputeGlobalScore();
}

void USOTS_GlobalStealthManagerSubsystem::ReportAISuspicion(AActor* GuardActor, float SuspicionNormalized)
{
    if (!GuardActor)
    {
        return;
    }

    const float ClampedSuspicion = FMath::Clamp(SuspicionNormalized, 0.0f, 1.0f);

    // Update or add this guard's suspicion entry.
    GuardSuspicion.FindOrAdd(GuardActor) = ClampedSuspicion;

    // Aggregate suspicion across all active guards (simple max for now).
    float MaxSuspicion = 0.0f;

    for (auto It = GuardSuspicion.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
            continue;
        }

        MaxSuspicion = FMath::Max(MaxSuspicion, It.Value());
    }

    SetAISuspicion(MaxSuspicion);
}

void USOTS_GlobalStealthManagerSubsystem::UpdateFromPlayer(const FSOTS_PlayerStealthState& PlayerState)
{
    CurrentState.LightLevel01 = FMath::Clamp(PlayerState.LightLevel01, 0.0f, 1.0f);
    CurrentState.ShadowLevel01 = FMath::Clamp(PlayerState.ShadowLevel01, 0.0f, 1.0f);
    CurrentState.CoverExposure01 = FMath::Clamp(PlayerState.CoverExposure01, 0.0f, 1.0f);
    CurrentState.MovementNoise01 = FMath::Clamp(PlayerState.MovementNoise01, 0.0f, 1.0f);
    CurrentState.LocalVisibility01 = FMath::Clamp(PlayerState.LocalVisibility01, 0.0f, 1.0f);
    // Preserve AISuspicion01 set from AI bridge; do not overwrite here.

    RecomputeGlobalScore();
    UpdateShadowCandidateForPlayerIfNeeded();
}

ESOTSStealthLevel USOTS_GlobalStealthManagerSubsystem::EvaluateStealthLevelFromScore(float Score) const
{
    const FSOTS_StealthScoringConfig& Cfg = ActiveConfig;

    if (Score <= Cfg.UndetectedMax)
    {
        return ESOTSStealthLevel::Undetected;
    }
    if (Score <= Cfg.LowRiskMax)
    {
        return ESOTSStealthLevel::LowRisk;
    }
    if (Score <= Cfg.MediumRiskMax)
    {
        return ESOTSStealthLevel::MediumRisk;
    }
    if (Score <= Cfg.HighRiskMax)
    {
        return ESOTSStealthLevel::HighRisk;
    }

    return ESOTSStealthLevel::FullyDetected;
}

void USOTS_GlobalStealthManagerSubsystem::SetStealthLevel(ESOTSStealthLevel NewLevel)
{
    if (NewLevel == CurrentStealthLevel)
    {
        return;
    }

    ESOTSStealthLevel OldLevel = CurrentStealthLevel;
    CurrentStealthLevel = NewLevel;

    OnStealthLevelChanged.Broadcast(OldLevel, NewLevel, CurrentStealthScore);
}

void USOTS_GlobalStealthManagerSubsystem::SetPlayerDetected(bool bDetectedNow)
{
    if (bPlayerDetected == bDetectedNow)
    {
        return;
    }

    bPlayerDetected = bDetectedNow;
    OnPlayerDetectionStateChanged.Broadcast(bPlayerDetected);
}

float USOTS_GlobalStealthManagerSubsystem::GetStealthScoreFor(const AActor* Actor) const
{
    // For now, the global manager tracks a single aggregated stealth state
    // (primarily the player). Until we add per-actor tracking, we simply
    // expose the current global score. Unknown actors fall back to 1.0.

    if (!Actor)
    {
        return 1.0f;
    }

    return FMath::Clamp(CurrentState.GlobalStealthScore01, 0.0f, 1.0f);
}

void USOTS_GlobalStealthManagerSubsystem::RecomputeGlobalScore()
{
    // Simple blend: configurable weights for local visibility vs AI suspicion.
    const float RawLocal = FMath::Clamp(CurrentState.LocalVisibility01, 0.0f, 1.0f);
    const float RawSusp  = FMath::Clamp(CurrentState.AISuspicion01, 0.0f, 1.0f);

    const float LocalW = ActiveConfig.LocalVisibilityWeight;
    const float SuspW  = ActiveConfig.AISuspicionWeight;
    const float SumW   = LocalW + SuspW;

    float Score = 0.0f;
    if (SumW > KINDA_SMALL_NUMBER)
    {
        Score = (RawLocal * LocalW + RawSusp * SuspW) / SumW;
    }

    Score = FMath::Clamp(Score, 0.0f, 1.0f);

    // Apply modifiers on top of the raw state.
    float EffectiveLight = CurrentState.LightLevel01;
    float EffectiveVisibility = RawLocal;
    float EffectiveGlobal = Score;

    for (const FSOTS_StealthModifier& Mod : ActiveModifiers)
    {
        EffectiveLight = EffectiveLight * Mod.LightMultiplier + Mod.LightOffset01;
        EffectiveVisibility = EffectiveVisibility * Mod.VisibilityMultiplier;
        EffectiveGlobal = EffectiveGlobal + Mod.GlobalScoreOffset01;
    }

    EffectiveLight = FMath::Clamp(EffectiveLight, 0.0f, 1.0f);
    EffectiveVisibility = FMath::Clamp(EffectiveVisibility, 0.0f, 1.0f);
    EffectiveGlobal = FMath::Clamp(EffectiveGlobal, 0.0f, 1.0f);

    CurrentState.LightLevel01 = EffectiveLight;
    CurrentState.ShadowLevel01 = 1.0f - EffectiveLight;
    CurrentState.LocalVisibility01 = EffectiveVisibility;
    CurrentState.GlobalStealthScore01 = EffectiveGlobal;
    CurrentStealthScore = EffectiveGlobal;

    // Map to the existing multi-level enum for backward compatibility.
    const ESOTSStealthLevel NewLevel = EvaluateStealthLevelFromScore(EffectiveGlobal);
    SetStealthLevel(NewLevel);

    // Map into the simpler tier enum.
    ESOTS_StealthTier NewTier = ESOTS_StealthTier::Hidden;
    if (EffectiveGlobal >= 0.8f)
    {
        NewTier = ESOTS_StealthTier::Compromised;
    }
    else if (EffectiveGlobal >= 0.5f)
    {
        NewTier = ESOTS_StealthTier::Danger;
    }
    else if (EffectiveGlobal >= 0.2f)
    {
        NewTier = ESOTS_StealthTier::Cautious;
    }

    CurrentState.StealthTier = NewTier;

    // Update the public breakdown snapshot so UI/FX/diagnostics can read it.
    float ModifierMultiplier = 1.0f;
    if (Score > KINDA_SMALL_NUMBER)
    {
        ModifierMultiplier = EffectiveGlobal / Score;
    }

    CurrentBreakdown.Light01            = CurrentState.LightLevel01;
    CurrentBreakdown.Shadow01           = CurrentState.ShadowLevel01;
    CurrentBreakdown.Visibility01       = CurrentState.LocalVisibility01;
    CurrentBreakdown.AISuspicion01      = CurrentState.AISuspicion01;
    CurrentBreakdown.CombinedScore01    = CurrentState.GlobalStealthScore01;
    CurrentBreakdown.StealthTier        = static_cast<int32>(CurrentState.StealthTier);
    CurrentBreakdown.ModifierMultiplier = ModifierMultiplier;

    //  Hard debug ping so we know GSM is actually updating at runtime.
    UE_LOG(LogSOTSGlobalStealth, Warning,
        TEXT("[GSM] RecomputeGlobalScore: Light=%.3f Shadow=%.3f Vis=%.3f AISusp=%.3f Global=%.3f Tier=%d"),
        CurrentBreakdown.Light01,
        CurrentBreakdown.Shadow01,
        CurrentBreakdown.Visibility01,
        CurrentBreakdown.AISuspicion01,
        CurrentBreakdown.CombinedScore01,
        CurrentBreakdown.StealthTier);

    UpdateGameplayTags();
    SyncPlayerStealthComponent();
    OnStealthStateChanged.Broadcast(CurrentState);

    UpdateShadowCandidateForPlayerIfNeeded();
}

void USOTS_GlobalStealthManagerSubsystem::UpdateGameplayTags()
{
    // NOTE: Integration with the existing CGF / Global Gameplay Tag Manager
    // is intentionally left as a TODO here, because that plugin is not part
    // of this repository. The intended behavior is:
    //
    // - Set exactly one of:
    //     SOTS.Stealth.State.Hidden
    //     SOTS.Stealth.State.Cautious
    //     SOTS.Stealth.State.Danger
    //     SOTS.Stealth.State.Compromised
    //   on the player, plus
    // - SOTS.Stealth.Light.Bright when LightLevel01 > 0.6
    // - SOTS.Stealth.Light.Dark when ShadowLevel01 > 0.6
    //
    // For now we just log the tier to aid tuning; callers that rely on tags
    // can hook this subsystem up to CGF in project-specific code.

    UE_LOG(LogSOTSGlobalStealth, VeryVerbose,
        TEXT("Stealth state updated: Score=%.2f Tier=%d Light=%.2f Shadow=%.2f"),
        CurrentState.GlobalStealthScore01,
        static_cast<int32>(CurrentState.StealthTier),
        CurrentState.LightLevel01,
        CurrentState.ShadowLevel01);
}

USOTS_PlayerStealthComponent* USOTS_GlobalStealthManagerSubsystem::FindPlayerStealthComponent() const
{
    if (UWorld* World = GetWorld())
    {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            if (APlayerController* Controller = It->Get())
            {
                if (APawn* Pawn = Controller->GetPawn())
                {
                    if (USOTS_PlayerStealthComponent* Component = Pawn->FindComponentByClass<USOTS_PlayerStealthComponent>())
                    {
                        return Component;
                    }
                }
            }
        }
    }

    return nullptr;
}

void USOTS_GlobalStealthManagerSubsystem::SyncPlayerStealthComponent()
{
    if (USOTS_PlayerStealthComponent* PlayerStealth = FindPlayerStealthComponent())
    {
        PlayerStealth->State = CurrentState;
        PlayerStealth->SetStealthTier(CurrentState.StealthTier);

        constexpr float CoverExposureThreshold = 0.35f;
        constexpr float ShadowThreshold = 0.6f;

        PlayerStealth->SetIsInCover(CurrentState.CoverExposure01 <= CoverExposureThreshold);
        PlayerStealth->SetIsInShadow(CurrentState.ShadowLevel01 >= ShadowThreshold);

        PlayerStealth->LastDetectionScore = CurrentStealthScore;
        PlayerStealth->SetDetected(bPlayerDetected);
    }
}

AActor* USOTS_GlobalStealthManagerSubsystem::FindPlayerActor() const
{
    if (USOTS_PlayerStealthComponent* PlayerStealth = FindPlayerStealthComponent())
    {
        return PlayerStealth->GetOwner();
    }

    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            return PC->GetPawn();
        }
    }

    return nullptr;
}

void USOTS_GlobalStealthManagerSubsystem::SetDominantDirectionalLightDirectionWS(FVector InDirWS, bool bValid)
{
    if (bValid && !InDirWS.IsNearlyZero())
    {
        DominantDirectionalLightDirWS = InDirWS.GetSafeNormal();
        bHasDominantDirectionalLightDir = true;
    }
    else
    {
        DominantDirectionalLightDirWS = FVector::ForwardVector;
        bHasDominantDirectionalLightDir = false;
    }
}

FSOTS_ShadowCandidate USOTS_GlobalStealthManagerSubsystem::GetPlayerShadowCandidate() const
{
    return CachedPlayerShadowCandidate;
}

void USOTS_GlobalStealthManagerSubsystem::GetShadowCandidateDebugState(
    bool& bOutHasDominantDir,
    FVector& OutDominantDir,
    bool& bOutCandidateValid,
    FVector& OutCandidatePoint,
    float& OutCandidateIllum01) const
{
    bOutHasDominantDir = bHasDominantDirectionalLightDir;
    OutDominantDir = DominantDirectionalLightDirWS;
    bOutCandidateValid = CachedPlayerShadowCandidate.bValid;
    OutCandidatePoint = CachedPlayerShadowCandidate.ShadowPointWS;
    OutCandidateIllum01 = CachedPlayerShadowCandidate.Illumination01;
}

void USOTS_GlobalStealthManagerSubsystem::UpdateShadowCandidateForPlayerIfNeeded()
{
    const FSOTS_StealthScoringConfig& Cfg = ActiveConfig;

    if (!Cfg.bEnableShadowCandidateCache)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const double Now = World->GetTimeSeconds();
    if (Now < NextShadowCandidateUpdateTimeSeconds)
    {
        return;
    }

    NextShadowCandidateUpdateTimeSeconds = Now + Cfg.ShadowCandidateUpdateInterval;

    CachedPlayerShadowCandidate = FSOTS_ShadowCandidate();
    CachedPlayerShadowCandidate.LastUpdateTimeSeconds = Now;
    CachedPlayerShadowCandidate.DominantLightDirWS = DominantDirectionalLightDirWS;

    const float Illumination01 = FMath::Clamp(CurrentState.LightLevel01, 0.0f, 1.0f);
    CachedPlayerShadowCandidate.Illumination01 = Illumination01;
    CachedPlayerShadowCandidate.Strength01 = Illumination01;

    if (Illumination01 < Cfg.ShadowMinIlluminationForCandidate)
    {
    #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (Cfg.bDebugShadowCandidateCache && World)
        {
            if (Now >= NextShadowCandidateDebugDrawTimeSeconds)
            {
                NextShadowCandidateDebugDrawTimeSeconds = Now + 1.0;
                DrawDebugString(World, FVector::ZeroVector + FVector(0.f, 0.f, 120.f), TEXT("GSM: Illum below min; no shadow candidate"), nullptr, FColor::Red, 1.0f, true, 1.2f);
            }
        }
    #endif
        return;
    }

    AActor* PlayerActor = FindPlayerActor();
    const FVector PlayerLoc = PlayerActor ? PlayerActor->GetActorLocation() : FVector::ZeroVector;

    if (!bHasDominantDirectionalLightDir)
    {
    #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        if (Cfg.bDebugShadowCandidateCache && World)
        {
            if (Now >= NextShadowCandidateDebugDrawTimeSeconds)
            {
                NextShadowCandidateDebugDrawTimeSeconds = Now + 1.0;
                DrawDebugString(World, PlayerLoc + FVector(0.f, 0.f, 140.f), TEXT("GSM: DominantDirectionalLightDir NOT SET — shadow candidate disabled"), nullptr, FColor::Red, 1.0f, true, 1.5f);
            }
        }
    #endif
        return;
    }

    if (!PlayerActor)
    {
        return;
    }

    const FVector LightDir = DominantDirectionalLightDirWS.GetSafeNormal();
    const FVector Start = PlayerActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
    const FVector End = Start + (LightDir * Cfg.ShadowCastDistance);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SOTS_ShadowCandidateTrace), Cfg.bShadowTraceComplex);
    Params.AddIgnoredActor(PlayerActor);

    if (World->LineTraceSingleByChannel(Hit, Start, End, Cfg.ShadowTraceChannel, Params))
    {
        CachedPlayerShadowCandidate.ShadowPointWS = Hit.ImpactPoint;
        CachedPlayerShadowCandidate.bValid = true;
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (Cfg.bDebugShadowCandidateCache && World)
    {
        if (Now >= NextShadowCandidateDebugDrawTimeSeconds)
        {
            NextShadowCandidateDebugDrawTimeSeconds = Now + 1.0;

            if (CachedPlayerShadowCandidate.bValid)
            {
                DrawDebugLine(World, Start, CachedPlayerShadowCandidate.ShadowPointWS, FColor::Yellow, false, 1.0f, 0, 1.5f);
                DrawDebugPoint(World, CachedPlayerShadowCandidate.ShadowPointWS, 12.f, FColor::Yellow, false, 1.0f);
            }
            else
            {
                DrawDebugString(World, Start, TEXT("GSM: Shadow candidate trace missed"), nullptr, FColor::Red, 1.0f, true, 1.2f);
            }
        }
    }
#endif
}

void USOTS_GlobalStealthManagerSubsystem::AddStealthModifier(const FSOTS_StealthModifier& Modifier)
{
    if (!Modifier.SourceId.IsNone())
    {
        // Replace any existing modifier from the same source.
        for (FSOTS_StealthModifier& Existing : ActiveModifiers)
        {
            if (Existing.SourceId == Modifier.SourceId)
            {
                Existing = Modifier;
                RecomputeGlobalScore();
                return;
            }
        }
    }

    ActiveModifiers.Add(Modifier);
    RecomputeGlobalScore();
}

void USOTS_GlobalStealthManagerSubsystem::RemoveStealthModifierBySource(FName SourceId)
{
    if (SourceId.IsNone() || ActiveModifiers.Num() == 0)
    {
        return;
    }

    ActiveModifiers.RemoveAll([SourceId](const FSOTS_StealthModifier& Mod)
        {
            return Mod.SourceId == SourceId;
        });

    RecomputeGlobalScore();
}

void USOTS_GlobalStealthManagerSubsystem::SetStealthConfig(const FSOTS_StealthScoringConfig& InConfig)
{
    ActiveConfig = InConfig;
    RecomputeGlobalScore();
}

void USOTS_GlobalStealthManagerSubsystem::SetStealthConfigAsset(USOTS_StealthConfigDataAsset* InAsset)
{
    if (!InAsset)
    {
        return;
    }

    DefaultConfigAsset = InAsset;

    // If there is no active override, also adopt this as the active asset.
    if (!ActiveStealthConfig)
    {
        ActiveStealthConfig = InAsset;
    }

    if (ActiveStealthConfig)
    {
        ActiveConfig = ActiveStealthConfig->Config;
    }
    else
    {
        ActiveConfig = InAsset->Config;
    }

    RecomputeGlobalScore();
}

void USOTS_GlobalStealthManagerSubsystem::PushStealthConfig(USOTS_StealthConfigDataAsset* NewConfig)
{
    if (!NewConfig)
    {
        return;
    }

    // Remember the previous active asset (may be null) so we can restore exactly.
    StealthConfigStack.Add(ActiveStealthConfig);

    ActiveStealthConfig = NewConfig;
    ActiveConfig = NewConfig->Config;

    UE_LOG(LogSOTSGlobalStealth, Log,
        TEXT("PushStealthConfig: ActiveStealthConfig set to '%s' (stack depth=%d)"),
        *GetNameSafe(ActiveStealthConfig),
        StealthConfigStack.Num());

    RecomputeGlobalScore();
}

void USOTS_GlobalStealthManagerSubsystem::PopStealthConfig()
{
    if (StealthConfigStack.Num() == 0)
    {
        UE_LOG(LogSOTSGlobalStealth, Warning,
            TEXT("PopStealthConfig: Stack is empty; ignoring pop."));
        return;
    }

    // Restore the previous asset (may be null).
    ActiveStealthConfig = StealthConfigStack.Pop();

    if (ActiveStealthConfig)
    {
        ActiveConfig = ActiveStealthConfig->Config;
    }
    else if (DefaultConfigAsset)
    {
        ActiveConfig = DefaultConfigAsset->Config;
    }
    else
    {
        ActiveConfig = FSOTS_StealthScoringConfig();
    }

    UE_LOG(LogSOTSGlobalStealth, Log,
        TEXT("PopStealthConfig: ActiveStealthConfig restored to '%s' (stack depth=%d)"),
        *GetNameSafe(ActiveStealthConfig),
        StealthConfigStack.Num());

    RecomputeGlobalScore();
}
