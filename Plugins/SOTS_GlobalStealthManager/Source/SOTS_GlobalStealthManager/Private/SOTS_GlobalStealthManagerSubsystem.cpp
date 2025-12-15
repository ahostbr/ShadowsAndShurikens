#include "SOTS_GlobalStealthManagerSubsystem.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_GlobalStealthManagerModule.h"
#include "SOTS_PlayerStealthComponent.h"
#include "SOTS_TagLibrary.h"
#include "DrawDebugHelpers.h"
#include "HAL/PlatformTime.h"

USOTS_GlobalStealthManagerSubsystem::USOTS_GlobalStealthManagerSubsystem()
    : CurrentStealthScore(0.0f)
    , CurrentStealthLevel(ESOTSStealthLevel::Undetected)
    , bPlayerDetected(false)
{
    LastAppliedTier = ESOTS_StealthTier::Hidden;
}

void USOTS_GlobalStealthManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Start with default struct values.
    ActiveConfig = FSOTS_StealthScoringConfig();
    BaseConfig = ActiveConfig;
    ModifierEntries.Reset();
    ConfigEntries.Reset();

    // If a default asset is assigned on the CDO, copy its config.
    if (DefaultConfigAsset)
    {
        ActiveStealthConfig = DefaultConfigAsset;
        ActiveConfig = DefaultConfigAsset->Config;
        BaseConfig = ActiveConfig;
    }

    ResetStealthState(ESOTS_GSM_ResetReason::Initialize);
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
    FSOTS_StealthInputSample Input;
    Input.Type = ESOTS_StealthInputType::Visibility;
    Input.StrengthNormalized = Sample.LightExposure;
    Input.SourceTag = FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.Stealth.Driver.Light")), false);
    Input.TargetActor = FindPlayerActor();
    Input.WorldLocation = Input.TargetActor.IsValid() ? Input.TargetActor->GetActorLocation() : FVector::ZeroVector;
    Input.TimestampSeconds = Sample.TimeSeconds;

    FSOTS_StealthIngestReport IngestReport;
    if (!IngestSample(Input, IngestReport))
    {
        return;
    }

    if (Input.SourceTag.IsValid())
    {
        LastReasonTag = Input.SourceTag;
    }

    LastSample = Sample;
    if (Input.TimestampSeconds > 0.0)
    {
        LastSample.TimeSeconds = static_cast<float>(Input.TimestampSeconds);
    }

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
    FSOTS_StealthInputSample Input;
    Input.Type = ESOTS_StealthInputType::Perception;
    Input.StrengthNormalized = bDetectedNow ? 1.0f : 0.0f;
    Input.InstigatorActor = Source;
    Input.TargetActor = FindPlayerActor();
    Input.TimestampSeconds = GetWorldTimeSecondsSafe();
    Input.SourceTag = FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.Stealth.Driver.LineOfSight")), false);

    FSOTS_StealthIngestReport IngestReport;
    if (!IngestSample(Input, IngestReport))
    {
        return;
    }

    if (Input.SourceTag.IsValid())
    {
        LastReasonTag = Input.SourceTag;
    }

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

    FSOTS_StealthInputSample Input;
    Input.Type = ESOTS_StealthInputType::Perception;
    Input.StrengthNormalized = ClampedSuspicion;
    Input.InstigatorActor = GuardActor;
    Input.TargetActor = FindPlayerActor();
    Input.WorldLocation = GuardActor->GetActorLocation();
    Input.TimestampSeconds = GetWorldTimeSecondsSafe();
    Input.SourceTag = FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.AI")), false);

    FSOTS_StealthIngestReport IngestReport;
    if (!IngestSample(Input, IngestReport))
    {
        return;
    }

    if (Input.SourceTag.IsValid())
    {
        LastReasonTag = Input.SourceTag;
    }

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
    ResolveEffectiveConfig();

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

    // Sort modifiers by priority desc then creation time desc for deterministic application.
    if (ModifierEntries.Num() > 1)
    {
        ModifierEntries.StableSort([](const FGSM_ModifierEntry& A, const FGSM_ModifierEntry& B)
        {
            if (A.Priority != B.Priority)
            {
                return A.Priority > B.Priority;
            }
            return A.CreatedTime > B.CreatedTime;
        });
    }

    for (const FGSM_ModifierEntry& Entry : ModifierEntries)
    {
        if (!Entry.bEnabled)
        {
            continue;
        }

        const FSOTS_StealthModifier& Mod = Entry.Modifier;
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
    float ScoreForTier = EffectiveGlobal;

    const FSOTS_StealthTierTuning& TierTuning = ActiveConfig.TierTuning;

    // Optional calm decay toward zero (OFF by default).
    if (TierTuning.bEnableCalmDecay && TierTuning.CalmDecayPerSecond > 0.0f)
    {
        const double Now = GetWorldTimeSecondsSafe();
        const double Dt = (LastScoreUpdateTimeSeconds > 0.0) ? (Now - LastScoreUpdateTimeSeconds) : 0.0;

        // Treat any non-zero incoming score as alerting input.
        if (EffectiveGlobal > KINDA_SMALL_NUMBER)
        {
            LastAlertingInputTimeSeconds = Now;
        }

        const double SinceAlert = (LastAlertingInputTimeSeconds > 0.0) ? (Now - LastAlertingInputTimeSeconds) : TNumericLimits<double>::Max();
        if (Dt > 0.0 && SinceAlert >= static_cast<double>(TierTuning.CalmDecayStartDelaySeconds))
        {
            const float DecayAmount = TierTuning.CalmDecayPerSecond * static_cast<float>(Dt);
            EffectiveGlobal = FMath::Max(0.0f, EffectiveGlobal - DecayAmount);
        }
    }

    // Optional smoothing (OFF by default).
    const double NowTime = GetWorldTimeSecondsSafe();
    const double DtSmooth = (LastScoreUpdateTimeSeconds > 0.0) ? (NowTime - LastScoreUpdateTimeSeconds) : 0.0;
    LastScoreUpdateTimeSeconds = NowTime;

    if (TierTuning.bEnableScoreSmoothing && TierTuning.ScoreSmoothingHalfLifeSeconds > 0.0f && DtSmooth > 0.0)
    {
        const double Alpha = 1.0 - FMath::Exp(-FMath::Loge(2.0) * DtSmooth / static_cast<double>(TierTuning.ScoreSmoothingHalfLifeSeconds));
        SmoothedStealthScore = FMath::Lerp(SmoothedStealthScore, EffectiveGlobal, static_cast<float>(Alpha));
        ScoreForTier = SmoothedStealthScore;
    }
    else
    {
        SmoothedStealthScore = EffectiveGlobal;
        ScoreForTier = EffectiveGlobal;
    }

    CurrentState.GlobalStealthScore01 = ScoreForTier;
    CurrentStealthScore = ScoreForTier;

    // Map to the existing multi-level enum for backward compatibility.
    const ESOTSStealthLevel NewLevel = EvaluateStealthLevelFromScore(ScoreForTier);
    SetStealthLevel(NewLevel);

    // Stable tier selection with hysteresis and min dwell.
    const ESOTS_StealthTier OldTier = CurrentState.StealthTier;
    ESOTS_StealthTier NewTier = OldTier;

    auto ComputeTierFromScore = [](float InScore, const FSOTS_StealthTierTuning& Tuning, ESOTS_StealthTier Current) -> ESOTS_StealthTier
    {
        const float CautiousMin = Tuning.CautiousMin;
        const float DangerMin = Tuning.DangerMin;
        const float CompromisedMin = Tuning.CompromisedMin;

        if (!Tuning.bEnableHysteresis || Tuning.HysteresisPadding <= KINDA_SMALL_NUMBER)
        {
            if (InScore >= CompromisedMin)
            {
                return ESOTS_StealthTier::Compromised;
            }
            if (InScore >= DangerMin)
            {
                return ESOTS_StealthTier::Danger;
            }
            if (InScore >= CautiousMin)
            {
                return ESOTS_StealthTier::Cautious;
            }
            return ESOTS_StealthTier::Hidden;
        }

        const float Pad = Tuning.HysteresisPadding;

        switch (Current)
        {
        case ESOTS_StealthTier::Hidden:
            if (InScore >= CautiousMin + Pad)
            {
                return ESOTS_StealthTier::Cautious;
            }
            return Current;
        case ESOTS_StealthTier::Cautious:
            if (InScore >= DangerMin + Pad)
            {
                return ESOTS_StealthTier::Danger;
            }
            if (InScore < CautiousMin - Pad)
            {
                return ESOTS_StealthTier::Hidden;
            }
            return Current;
        case ESOTS_StealthTier::Danger:
            if (InScore >= CompromisedMin + Pad)
            {
                return ESOTS_StealthTier::Compromised;
            }
            if (InScore < DangerMin - Pad)
            {
                return ESOTS_StealthTier::Cautious;
            }
            return Current;
        case ESOTS_StealthTier::Compromised:
        default:
            if (InScore < CompromisedMin - Pad)
            {
                return ESOTS_StealthTier::Danger;
            }
            return Current;
        }
    };

    NewTier = ComputeTierFromScore(ScoreForTier, TierTuning, OldTier);

    // Optional minimum time between tier changes.
    if (NewTier != OldTier && TierTuning.MinSecondsBetweenTierChanges > 0.0f)
    {
        const double Now = GetWorldTimeSecondsSafe();
        if (LastTierChangeTimeSeconds > 0.0 &&
            (Now - LastTierChangeTimeSeconds) < static_cast<double>(TierTuning.MinSecondsBetweenTierChanges))
        {
            NewTier = OldTier;
        }
        else
        {
            LastTierChangeTimeSeconds = Now;
        }
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

    //  Debug ping for development only.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (ActiveConfig.bDebugLogStackOps)
    {
        UE_LOG(LogSOTSGlobalStealth, Warning,
            TEXT("[GSM] RecomputeGlobalScore: Light=%.3f Shadow=%.3f Vis=%.3f AISusp=%.3f Global=%.3f Tier=%d"),
            CurrentBreakdown.Light01,
            CurrentBreakdown.Shadow01,
            CurrentBreakdown.Visibility01,
            CurrentBreakdown.AISuspicion01,
            CurrentBreakdown.CombinedScore01,
            CurrentBreakdown.StealthTier);
    }
#endif

    // Build transition payload (even if tier unchanged, used for tag apply).
    FSOTS_StealthTierTransition Transition;
    Transition.OldTier = OldTier;
    Transition.NewTier = NewTier;
    Transition.OldScore = LastBroadcastStealthScore;
    Transition.NewScore = CurrentStealthScore;
    Transition.TimestampSeconds = LastScoreUpdateTimeSeconds;
    Transition.ReasonTag = LastReasonTag;
    for (const FGSM_ModifierEntry& Entry : ModifierEntries)
    {
        if (Entry.OwnerTag.IsValid())
        {
            Transition.ActiveModifierOwners.AddUnique(Entry.OwnerTag);
        }
    }

    // Score change event (default fires whenever recompute runs).
    if (!FMath::IsNearlyEqual(CurrentStealthScore, LastBroadcastStealthScore))
    {
        FSOTS_StealthScoreChange Change;
        Change.OldScore = LastBroadcastStealthScore;
        Change.NewScore = CurrentStealthScore;
        Change.ReasonTag = LastReasonTag;
        Change.TimestampSeconds = LastScoreUpdateTimeSeconds;
        OnStealthScoreChanged.Broadcast(Change);
        LastBroadcastStealthScore = CurrentStealthScore;
    }

    // Tier transition event.
    if (NewTier != OldTier)
    {
        OnStealthTierChanged.Broadcast(Transition);
    }

    ApplyStealthStateTags(FindPlayerActor(), NewTier, CurrentStealthScore, Transition);
    UpdateGameplayTags();
    SyncPlayerStealthComponent();
    OnStealthStateChanged.Broadcast(CurrentState);

    UpdateShadowCandidateForPlayerIfNeeded();
}

void USOTS_GlobalStealthManagerSubsystem::UpdateGameplayTags()
{
    AActor* PlayerActor = FindPlayerActor();
    if (!PlayerActor)
    {
        return;
    }

    static const FGameplayTag BrightTag = FGameplayTag::RequestGameplayTag(FName(TEXT("SOTS.Stealth.Light.Bright")), false);
    static const FGameplayTag DarkTag = FGameplayTag::RequestGameplayTag(FName(TEXT("SOTS.Stealth.Light.Dark")), false);

    const bool bBright = CurrentState.LightLevel01 > 0.6f;
    const bool bDark = CurrentState.ShadowLevel01 > 0.6f;

    if (BrightTag.IsValid())
    {
        if (bBright)
        {
            USOTS_TagLibrary::AddTagToActor(this, PlayerActor, BrightTag);
        }
        else
        {
            USOTS_TagLibrary::RemoveTagFromActor(this, PlayerActor, BrightTag);
        }
    }

    if (DarkTag.IsValid())
    {
        if (bDark)
        {
            USOTS_TagLibrary::AddTagToActor(this, PlayerActor, DarkTag);
        }
        else
        {
            USOTS_TagLibrary::RemoveTagFromActor(this, PlayerActor, DarkTag);
        }
    }
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

double USOTS_GlobalStealthManagerSubsystem::GetWorldTimeSecondsSafe() const
{
    if (const UWorld* World = GetWorld())
    {
        return World->GetTimeSeconds();
    }

    return FPlatformTime::Seconds();
}

float USOTS_GlobalStealthManagerSubsystem::GetMinSecondsBetweenType(const FSOTS_StealthIngestTuning& Tuning, ESOTS_StealthInputType Type) const
{
    switch (Type)
    {
    case ESOTS_StealthInputType::Visibility:
        return Tuning.MinSecondsBetweenVisibility;
    case ESOTS_StealthInputType::Light:
        return Tuning.MinSecondsBetweenLight;
    case ESOTS_StealthInputType::Perception:
        return Tuning.MinSecondsBetweenPerception;
    case ESOTS_StealthInputType::Noise:
        return Tuning.MinSecondsBetweenNoise;
    case ESOTS_StealthInputType::Weather:
        return Tuning.MinSecondsBetweenWeather;
    case ESOTS_StealthInputType::Custom:
        return Tuning.MinSecondsBetweenCustom;
    default:
        return 0.0f;
    }
}

void USOTS_GlobalStealthManagerSubsystem::ResolveEffectiveConfig()
{
    // Base config (from default asset or SetStealthConfig).
    FSOTS_StealthScoringConfig Resolved = BaseConfig;
    ActiveStealthConfig = nullptr;

    // Choose top config by priority then creation time (latest wins on ties).
    if (ConfigEntries.Num() > 0)
    {
        ConfigEntries.StableSort([](const FGSM_ConfigEntry& A, const FGSM_ConfigEntry& B)
        {
            if (A.Priority != B.Priority)
            {
                return A.Priority > B.Priority;
            }
            return A.CreatedTime > B.CreatedTime;
        });

        for (const FGSM_ConfigEntry& Entry : ConfigEntries)
        {
            if (!Entry.bEnabled)
            {
                continue;
            }

            if (Entry.Asset)
            {
                Resolved = Entry.Asset->Config;
                ActiveStealthConfig = Entry.Asset;
                break;
            }
        }
    }

    ActiveConfig = Resolved;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (ActiveConfig.bDebugDumpEffectiveTuning)
    {
        UE_LOG(LogSOTSGlobalStealth, Log,
            TEXT("[GSM] ResolveEffectiveConfig | Light=%.3f LOS=%.3f Dist=%.3f Noise=%.3f VisW=%.3f AIW=%.3f Tier=(%.2f,%.2f,%.2f)"),
            ActiveConfig.LightWeight,
            ActiveConfig.LOSWeight,
            ActiveConfig.DistanceWeight,
            ActiveConfig.NoiseWeight,
            ActiveConfig.LocalVisibilityWeight,
            ActiveConfig.AISuspicionWeight,
            ActiveConfig.TierTuning.CautiousMin,
            ActiveConfig.TierTuning.DangerMin,
            ActiveConfig.TierTuning.CompromisedMin);
    }

    if (ActiveConfig.bDebugLogStackOps)
    {
        DebugDumpStackToLog();
    }
#endif
}

void USOTS_GlobalStealthManagerSubsystem::GetActiveModifierHandles(TArray<FSOTS_GSM_Handle>& OutHandles) const
{
    OutHandles.Reset();
    for (const FGSM_ModifierEntry& Entry : ModifierEntries)
    {
        if (Entry.Handle.IsValid())
        {
            OutHandles.Add(Entry.Handle);
        }
    }
}

void USOTS_GlobalStealthManagerSubsystem::GetActiveConfigHandles(TArray<FSOTS_GSM_Handle>& OutHandles) const
{
    OutHandles.Reset();
    for (const FGSM_ConfigEntry& Entry : ConfigEntries)
    {
        if (Entry.Handle.IsValid())
        {
            OutHandles.Add(Entry.Handle);
        }
    }
}

FSOTS_GSM_TuningSummary USOTS_GlobalStealthManagerSubsystem::GetEffectiveTuningSummary() const
{
    FSOTS_GSM_TuningSummary Summary;
    Summary.LightWeight = ActiveConfig.LightWeight;
    Summary.LOSWeight = ActiveConfig.LOSWeight;
    Summary.DistanceWeight = ActiveConfig.DistanceWeight;
    Summary.NoiseWeight = ActiveConfig.NoiseWeight;
    Summary.LocalVisibilityWeight = ActiveConfig.LocalVisibilityWeight;
    Summary.AISuspicionWeight = ActiveConfig.AISuspicionWeight;
    Summary.CautiousMin = ActiveConfig.TierTuning.CautiousMin;
    Summary.DangerMin = ActiveConfig.TierTuning.DangerMin;
    Summary.CompromisedMin = ActiveConfig.TierTuning.CompromisedMin;
    return Summary;
}

void USOTS_GlobalStealthManagerSubsystem::DebugDumpStackToLog() const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (!ActiveConfig.bDebugLogStackOps)
    {
        return;
    }

    UE_LOG(LogSOTSGlobalStealth, Log, TEXT("[GSM] BaseConfig set (Light=%.3f LOS=%.3f Dist=%.3f Noise=%.3f)"),
        BaseConfig.LightWeight,
        BaseConfig.LOSWeight,
        BaseConfig.DistanceWeight,
        BaseConfig.NoiseWeight);

    if (ConfigEntries.Num() == 0)
    {
        UE_LOG(LogSOTSGlobalStealth, Log, TEXT("[GSM] No config overrides active."));
    }
    else
    {
        UE_LOG(LogSOTSGlobalStealth, Log, TEXT("[GSM] Config overrides (priority desc, created desc):"));
        for (const FGSM_ConfigEntry& Entry : ConfigEntries)
        {
            UE_LOG(LogSOTSGlobalStealth, Log,
                TEXT("  Config=%s Priority=%d Owner=%s Created=%.3f Enabled=%d"),
                *GetNameSafe(Entry.Asset),
                Entry.Priority,
                Entry.OwnerTag.IsValid() ? *Entry.OwnerTag.ToString() : TEXT("-"),
                Entry.CreatedTime,
                Entry.bEnabled ? 1 : 0);
        }
    }

    if (ModifierEntries.Num() == 0)
    {
        UE_LOG(LogSOTSGlobalStealth, Log, TEXT("[GSM] No modifiers active."));
    }
    else
    {
        UE_LOG(LogSOTSGlobalStealth, Log, TEXT("[GSM] Modifiers (priority desc, created desc):"));
        for (const FGSM_ModifierEntry& Entry : ModifierEntries)
        {
            UE_LOG(LogSOTSGlobalStealth, Log,
                TEXT("  Source=%s Priority=%d Owner=%s Mul(Light=%.2f Vis=%.2f) Add(Light=%.2f Global=%.2f) Enabled=%d"),
                *Entry.Modifier.SourceId.ToString(),
                Entry.Priority,
                Entry.OwnerTag.IsValid() ? *Entry.OwnerTag.ToString() : TEXT("-"),
                Entry.Modifier.LightMultiplier,
                Entry.Modifier.VisibilityMultiplier,
                Entry.Modifier.LightOffset01,
                Entry.Modifier.GlobalScoreOffset01,
                Entry.bEnabled ? 1 : 0);
        }
    }
#endif
}

FGameplayTag USOTS_GlobalStealthManagerSubsystem::GetTierTag(ESOTS_StealthTier Tier) const
{
    switch (Tier)
    {
    case ESOTS_StealthTier::Hidden:
        return FGameplayTag::RequestGameplayTag(FName(TEXT("SOTS.Stealth.State.Hidden")), false);
    case ESOTS_StealthTier::Cautious:
        return FGameplayTag::RequestGameplayTag(FName(TEXT("SOTS.Stealth.State.Cautious")), false);
    case ESOTS_StealthTier::Danger:
        return FGameplayTag::RequestGameplayTag(FName(TEXT("SOTS.Stealth.State.Danger")), false);
    case ESOTS_StealthTier::Compromised:
        return FGameplayTag::RequestGameplayTag(FName(TEXT("SOTS.Stealth.State.Compromised")), false);
    default:
        return FGameplayTag();
    }
}

bool USOTS_GlobalStealthManagerSubsystem::ActorHasTag(AActor* Actor, const FGameplayTag& Tag) const
{
    if (!Actor || !Tag.IsValid())
    {
        return false;
    }

    return USOTS_TagLibrary::ActorHasTag(this, Actor, Tag);
}

void USOTS_GlobalStealthManagerSubsystem::ApplyStealthStateTags(AActor* TargetActor, ESOTS_StealthTier NewTier, float NewScore, const FSOTS_StealthTierTransition& Transition)
{
    if (!TargetActor)
    {
        return;
    }

    const FGameplayTag NewTag = GetTierTag(NewTier);
    if (!NewTag.IsValid())
    {
        return;
    }

    const bool bTierChanged = (NewTier != LastAppliedTier);
    const bool bTagDrifted = ActiveConfig.bRepairStealthTagsIfDriftDetected && !ActorHasTag(TargetActor, NewTag);

    if (!bTierChanged && !bTagDrifted)
    {
        return;
    }

    // Remove old tier tags.
    const FGameplayTag Hidden = GetTierTag(ESOTS_StealthTier::Hidden);
    const FGameplayTag Cautious = GetTierTag(ESOTS_StealthTier::Cautious);
    const FGameplayTag Danger = GetTierTag(ESOTS_StealthTier::Danger);
    const FGameplayTag Compromised = GetTierTag(ESOTS_StealthTier::Compromised);

    if (Hidden.IsValid()) { USOTS_TagLibrary::RemoveTagFromActor(this, TargetActor, Hidden); }
    if (Cautious.IsValid()) { USOTS_TagLibrary::RemoveTagFromActor(this, TargetActor, Cautious); }
    if (Danger.IsValid()) { USOTS_TagLibrary::RemoveTagFromActor(this, TargetActor, Danger); }
    if (Compromised.IsValid()) { USOTS_TagLibrary::RemoveTagFromActor(this, TargetActor, Compromised); }

    // Apply new tier tag.
    USOTS_TagLibrary::AddTagToActor(this, TargetActor, NewTag);

    LastAppliedTier = NewTier;
    LastAppliedTierTag = NewTag;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (ActiveConfig.bDebugLogStackOps)
    {
        UE_LOG(LogSOTSGlobalStealth, Log,
            TEXT("[GSM] ApplyStealthStateTags Tier=%d Tag=%s Score=%.3f Reason=%s"),
            static_cast<int32>(NewTier),
            *NewTag.ToString(),
            NewScore,
            Transition.ReasonTag.IsValid() ? *Transition.ReasonTag.ToString() : TEXT("-"));
    }
#endif
}

void USOTS_GlobalStealthManagerSubsystem::LogIngestDecision(const FSOTS_StealthIngestReport& Report, const FSOTS_StealthInputSample& Sample, bool bAccepted)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    const FSOTS_StealthIngestTuning& Tuning = ActiveConfig.IngestTuning;
    const bool bWantsLog = bAccepted ? Tuning.bDebugLogStealthIngest : Tuning.bDebugLogStealthDrops;
    if (!bWantsLog)
    {
        return;
    }

    const double Now = GetWorldTimeSecondsSafe();
    TMap<ESOTS_StealthInputType, double>& LastLogMap = bAccepted ? LastLoggedIngestTime : LastLoggedDropTime;
    const double LastLog = LastLogMap.FindRef(Report.Type);
    const double ThrottleSeconds = FMath::Max(0.0f, Tuning.DebugLogThrottleSeconds);
    if (ThrottleSeconds > 0.0 && LastLog > 0.0 && (Now - LastLog) < ThrottleSeconds)
    {
        return;
    }

    LastLogMap.FindOrAdd(Report.Type) = Now;

    const TCHAR* ResultStr = bAccepted ? TEXT("ACCEPT") : TEXT("DROP");
    const FString Reason = Report.DebugReason.IsEmpty() ? TEXT("-") : Report.DebugReason;

    UE_LOG(LogSOTSGlobalStealth, Log,
        TEXT("[GSM Ingest] %s Type=%d StrengthIn=%.3f StrengthClamped=%.3f Reason=%s Source=%s Target=%s Time=%.3f"),
        ResultStr,
        static_cast<int32>(Report.Type),
        Report.StrengthIn,
        Report.StrengthClamped,
        *Reason,
        *GetNameSafe(Sample.InstigatorActor.Get()),
        *GetNameSafe(Sample.TargetActor.Get()),
        Sample.TimestampSeconds);
#endif
}

bool USOTS_GlobalStealthManagerSubsystem::IngestSample(const FSOTS_StealthInputSample& Sample, FSOTS_StealthIngestReport& OutReport)
{
    FSOTS_StealthInputSample WorkingSample = Sample;

    OutReport = FSOTS_StealthIngestReport();
    OutReport.Type = WorkingSample.Type;
    OutReport.StrengthIn = WorkingSample.StrengthNormalized;

    // Ensure timestamp is valid.
    double Timestamp = WorkingSample.TimestampSeconds;
    if (!FMath::IsFinite(Timestamp) || Timestamp <= 0.0)
    {
        Timestamp = GetWorldTimeSecondsSafe();
    }
    if (!FMath::IsFinite(Timestamp) || Timestamp <= 0.0)
    {
        Timestamp = FPlatformTime::Seconds();
    }
    WorkingSample.TimestampSeconds = Timestamp;

    // Validate normalized strength.
    if (!FMath::IsFinite(WorkingSample.StrengthNormalized))
    {
        OutReport.Result = ESOTS_StealthIngestResult::Dropped_Invalid;
        OutReport.DebugReason = TEXT("Strength NaN/Inf");
        LogIngestDecision(OutReport, WorkingSample, false);
        return false;
    }

    WorkingSample.StrengthNormalized = FMath::Clamp(WorkingSample.StrengthNormalized, 0.0f, 1.0f);
    OutReport.StrengthClamped = WorkingSample.StrengthNormalized;

    const FSOTS_StealthIngestTuning& Tuning = ActiveConfig.IngestTuning;

    // Optional stale drop.
    if (Tuning.bDropStaleSamples && Tuning.MaxSampleAgeSeconds > 0.0f)
    {
        const double AgeSeconds = GetWorldTimeSecondsSafe() - WorkingSample.TimestampSeconds;
        if (AgeSeconds > static_cast<double>(Tuning.MaxSampleAgeSeconds))
        {
            OutReport.Result = ESOTS_StealthIngestResult::Dropped_OutOfWindow;
            OutReport.DebugReason = TEXT("Sample stale");
            LogIngestDecision(OutReport, WorkingSample, false);
            return false;
        }
    }

    // Optional throttling per type.
    if (Tuning.bEnableIngestThrottle)
    {
        const float MinGap = GetMinSecondsBetweenType(Tuning, WorkingSample.Type);
        if (MinGap > 0.0f)
        {
            const double LastTime = LastAcceptedSampleTime.FindRef(WorkingSample.Type);
            if (LastTime > 0.0 && (WorkingSample.TimestampSeconds - LastTime) < static_cast<double>(MinGap))
            {
                OutReport.Result = ESOTS_StealthIngestResult::Dropped_TooSoon;
                OutReport.DebugReason = FString::Printf(TEXT("Throttle %.2fs"), MinGap);
                LogIngestDecision(OutReport, WorkingSample, false);
                return false;
            }
        }
    }

    OutReport.Result = ESOTS_StealthIngestResult::Accepted;
    LastAcceptedSampleTime.FindOrAdd(WorkingSample.Type) = WorkingSample.TimestampSeconds;
    LogIngestDecision(OutReport, WorkingSample, true);
    return true;
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

FSOTS_GSM_Handle USOTS_GlobalStealthManagerSubsystem::AddStealthModifier(const FSOTS_StealthModifier& Modifier, FGameplayTag OwnerTag, int32 Priority)
{
    FSOTS_GSM_Handle Handle;
    Handle.Id = FGuid::NewGuid();
    Handle.KindTag = FGameplayTag::RequestGameplayTag(FName(TEXT("GSM.Handle.Modifier")), false);
    Handle.OwnerTag = OwnerTag;
    Handle.CreatedAtSeconds = GetWorldTimeSecondsSafe();

    // Replace any existing modifier from the same SourceId.
    if (!Modifier.SourceId.IsNone())
    {
        for (FGSM_ModifierEntry& Entry : ModifierEntries)
        {
            if (Entry.Modifier.SourceId == Modifier.SourceId)
            {
                Entry.Modifier = Modifier;
                Entry.Handle = Handle;
                Entry.CreatedTime = Handle.CreatedAtSeconds;
                Entry.OwnerTag = OwnerTag;
                RecomputeGlobalScore();
                return Handle;
            }
        }
    }

    FGSM_ModifierEntry Entry;
    Entry.Modifier = Modifier;
    Entry.Handle = Handle;
    Entry.Priority = Priority;
    Entry.CreatedTime = Handle.CreatedAtSeconds;
    Entry.OwnerTag = OwnerTag;
    ModifierEntries.Add(Entry);
    RecomputeGlobalScore();
    return Handle;
}

ESOTS_GSM_RemoveResult USOTS_GlobalStealthManagerSubsystem::RemoveStealthModifierByHandle(const FSOTS_GSM_Handle& Handle, FGameplayTag RequesterTag)
{
    if (!Handle.IsValid())
    {
        return ESOTS_GSM_RemoveResult::InvalidHandle;
    }

    const FGameplayTag ExpectedKind = FGameplayTag::RequestGameplayTag(FName(TEXT("GSM.Handle.Modifier")));
    if (Handle.KindTag != ExpectedKind)
    {
        return ESOTS_GSM_RemoveResult::WrongKind;
    }

    for (int32 Index = 0; Index < ModifierEntries.Num(); ++Index)
    {
        if (ModifierEntries[Index].Handle.Id == Handle.Id)
        {
            if (Handle.OwnerTag.IsValid() && ModifierEntries[Index].OwnerTag.IsValid() && Handle.OwnerTag != ModifierEntries[Index].OwnerTag)
            {
                if (!RequesterTag.IsValid() || RequesterTag != ModifierEntries[Index].OwnerTag)
                {
                    return ESOTS_GSM_RemoveResult::OwnerMismatch;
                }
            }

            ModifierEntries.RemoveAt(Index);
            RecomputeGlobalScore();
            return ESOTS_GSM_RemoveResult::Removed;
        }
    }

    return ESOTS_GSM_RemoveResult::NotFound;
}

int32 USOTS_GlobalStealthManagerSubsystem::RemoveStealthModifierBySource(FName SourceId)
{
    if (SourceId.IsNone() || ModifierEntries.Num() == 0)
    {
        return 0;
    }

    int32 Removed = 0;
    for (int32 Index = ModifierEntries.Num() - 1; Index >= 0; --Index)
    {
        if (ModifierEntries[Index].Modifier.SourceId == SourceId)
        {
            ModifierEntries.RemoveAt(Index);
            ++Removed;
        }
    }

    if (Removed > 0)
    {
        RecomputeGlobalScore();
    }

    return Removed;
}

void USOTS_GlobalStealthManagerSubsystem::SetStealthConfig(const FSOTS_StealthScoringConfig& InConfig)
{
    BaseConfig = InConfig;
    ResolveEffectiveConfig();
    RecomputeGlobalScore();
}

void USOTS_GlobalStealthManagerSubsystem::SetStealthConfigAsset(USOTS_StealthConfigDataAsset* InAsset)
{
    if (!InAsset)
    {
        return;
    }

    DefaultConfigAsset = InAsset;
    BaseConfig = InAsset->Config;
    ResolveEffectiveConfig();
    RecomputeGlobalScore();
}

FSOTS_GSM_Handle USOTS_GlobalStealthManagerSubsystem::PushStealthConfig(USOTS_StealthConfigDataAsset* NewConfig, int32 Priority, FGameplayTag OwnerTag)
{
    if (!NewConfig)
    {
        return FSOTS_GSM_Handle();
    }

    FSOTS_GSM_Handle Handle;
    Handle.Id = FGuid::NewGuid();
    Handle.KindTag = FGameplayTag::RequestGameplayTag(FName(TEXT("GSM.Handle.Config")), false);
    Handle.OwnerTag = OwnerTag;
    Handle.CreatedAtSeconds = GetWorldTimeSecondsSafe();

    FGSM_ConfigEntry Entry;
    Entry.Asset = NewConfig;
    Entry.Handle = Handle;
    Entry.Priority = Priority;
    Entry.CreatedTime = Handle.CreatedAtSeconds;
    Entry.OwnerTag = OwnerTag;
    ConfigEntries.Add(Entry);

    UE_LOG(LogSOTSGlobalStealth, Log,
        TEXT("PushStealthConfig: Added '%s' (priority=%d, total=%d)"),
        *GetNameSafe(NewConfig),
        Priority,
        ConfigEntries.Num());

    ResolveEffectiveConfig();
    RecomputeGlobalScore();
    return Handle;
}

ESOTS_GSM_RemoveResult USOTS_GlobalStealthManagerSubsystem::PopStealthConfig(const FSOTS_GSM_Handle& Handle, FGameplayTag RequesterTag)
{
    if (!Handle.IsValid())
    {
        return ESOTS_GSM_RemoveResult::InvalidHandle;
    }

    const FGameplayTag ExpectedKind = FGameplayTag::RequestGameplayTag(FName(TEXT("GSM.Handle.Config")), false);
    if (Handle.KindTag.IsValid() && ExpectedKind.IsValid() && Handle.KindTag != ExpectedKind)
    {
        return ESOTS_GSM_RemoveResult::WrongKind;
    }

    for (int32 Index = 0; Index < ConfigEntries.Num(); ++Index)
    {
        if (ConfigEntries[Index].Handle.Id == Handle.Id)
        {
            if (Handle.OwnerTag.IsValid() && ConfigEntries[Index].OwnerTag.IsValid() && Handle.OwnerTag != ConfigEntries[Index].OwnerTag)
            {
                if (!RequesterTag.IsValid() || RequesterTag != ConfigEntries[Index].OwnerTag)
                {
                    return ESOTS_GSM_RemoveResult::OwnerMismatch;
                }
            }

            ConfigEntries.RemoveAt(Index);

            UE_LOG(LogSOTSGlobalStealth, Log,
                TEXT("PopStealthConfig: Removed handle %s (remaining=%d)"),
                *Handle.Id.ToString(),
                ConfigEntries.Num());

            ResolveEffectiveConfig();
            RecomputeGlobalScore();
            return ESOTS_GSM_RemoveResult::Removed;
        }
    }

    return ESOTS_GSM_RemoveResult::NotFound;
}

void USOTS_GlobalStealthManagerSubsystem::BuildProfileData(FSOTS_GSMProfileData& OutData) const
{
    // Stateless by design: always serialize defaults.
    OutData = FSOTS_GSMProfileData();
}

void USOTS_GlobalStealthManagerSubsystem::ApplyProfileData(const FSOTS_GSMProfileData& InData)
{
    (void)InData;
    ResetStealthState(ESOTS_GSM_ResetReason::ProfileLoaded);
}

void USOTS_GlobalStealthManagerSubsystem::ResetStealthState(ESOTS_GSM_ResetReason Reason)
{
    // Clear dynamic stacks (modifiers/config overrides) but preserve BaseConfig/default asset.
    ModifierEntries.Reset();
    ConfigEntries.Reset();
    ActiveStealthConfig = DefaultConfigAsset;
    ActiveConfig = BaseConfig;
    ResolveEffectiveConfig();

    LastSample = FSOTS_StealthSample();
    CurrentState = FSOTS_PlayerStealthState();
    CurrentStealthScore = 0.0f;
    CurrentStealthLevel = ESOTSStealthLevel::Undetected;
    bPlayerDetected = false;
    CurrentBreakdown = FSOTS_StealthScoreBreakdown();
    SmoothedStealthScore = 0.0f;
    LastBroadcastStealthScore = 0.0f;
    LastReasonTag = FGameplayTag();
    LastTierChangeTimeSeconds = 0.0;
    LastScoreUpdateTimeSeconds = GetWorldTimeSecondsSafe();
    LastAlertingInputTimeSeconds = 0.0;
    LastAppliedTier = ESOTS_StealthTier::Hidden;
    LastAppliedTierTag = GetTierTag(LastAppliedTier);

    FSOTS_StealthTierTransition Transition;
    Transition.OldTier = LastAppliedTier;
    Transition.NewTier = LastAppliedTier;
    Transition.OldScore = 0.0f;
    Transition.NewScore = 0.0f;
    Transition.TimestampSeconds = LastScoreUpdateTimeSeconds;
    Transition.ReasonTag = FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.Debug")), false);

    ApplyStealthStateTags(FindPlayerActor(), LastAppliedTier, 0.0f, Transition);
    UpdateGameplayTags();
    SyncPlayerStealthComponent();
    OnStealthStateChanged.Broadcast(CurrentState);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (ActiveConfig.bDebugLogStealthResets)
    {
        UE_LOG(LogSOTSGlobalStealth, Log,
            TEXT("[GSM] ResetStealthState Reason=%d"), static_cast<int32>(Reason));
    }
#endif
}
