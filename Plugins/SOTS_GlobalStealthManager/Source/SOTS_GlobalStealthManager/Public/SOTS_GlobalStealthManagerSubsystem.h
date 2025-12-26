#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_ProfileSnapshotProvider.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_StealthConfigDataAsset.h"
#include "SOTS_GlobalStealthManagerSubsystem.generated.h"

class AActor;
class APlayerController;
class APawn;
class UWorld;
class USOTS_PlayerStealthComponent;

USTRUCT(BlueprintType)
struct FSOTS_AISuspicionReport
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    TWeakObjectPtr<AActor> SubjectAI;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    float Suspicion01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    FGameplayTag ReasonTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    bool bHasLocation = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    TWeakObjectPtr<AActor> InstigatorActor;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GSM|AI")
    double TimestampSeconds = 0.0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSOTS_StealthLevelChangedSignature, ESOTSStealthLevel, OldLevel, ESOTSStealthLevel, NewLevel, float, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_PlayerDetectionStateChangedSignature, bool, bDetected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_StealthScoreChangedSignature, const FSOTS_StealthScoreChange&, Change);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_StealthTierTransitionSignature, const FSOTS_StealthTierTransition&, Transition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnAISuspicionReported, const FSOTS_AISuspicionReport&, Report);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSOTS_OnAIAwarenessStateChanged, AActor*, SubjectAI, ESOTS_AIAwarenessState, OldState, ESOTS_AIAwarenessState, NewState, const FSOTS_GSM_AIRecord&, Record);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnAIRecordUpdated, const FSOTS_GSM_AIRecord&, Record);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_OnGlobalAlertnessChanged, float, NewValue, float, OldValue);

/**
 * Global, non-ticking stealth aggregator.
 *
 * Other systems (player movement, cover, AI perception, Ultra Dynamic Sky traces)
 * report discrete samples and events into this subsystem. It computes a single,
 * authoritative stealth score and level that SOTS can use for gameplay, VFX,
 * and UI decisions.
 */
UCLASS()
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_GlobalStealthManagerSubsystem : public UGameInstanceSubsystem, public ISOTS_ProfileSnapshotProvider
{
    GENERATED_BODY()

public:
    USOTS_GlobalStealthManagerSubsystem();

    // Easy access helper for Blueprints and C++
    UFUNCTION(BlueprintCallable, Category="Stealth", meta=(WorldContext="WorldContextObject"))
    static USOTS_GlobalStealthManagerSubsystem* Get(const UObject* WorldContextObject);

    // Last sample that was processed into the score.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    FSOTS_StealthSample LastSample;

    // Current player-facing stealth state (light, cover, noise, AI, tiers).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    FSOTS_PlayerStealthState CurrentState;

    // 0 = perfectly hidden, 1 = fully exposed/detected.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    float CurrentStealthScore;

    // Discrete, layered stealth state built from the score and events.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    ESOTSStealthLevel CurrentStealthLevel;

    // True if at least one enemy has actively "spotted" the player and triggered a fail/alert state.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stealth")
    bool bPlayerDetected;

    // Latest breakdown of how the score was composed, for UI/FX/debug.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|Stealth")
    FSOTS_StealthScoreBreakdown CurrentBreakdown;

    // Broadcast when the stealth level changes (e.g., for dragon visibility or radial vignette).
    UPROPERTY(BlueprintAssignable, Category="Stealth")
    FSOTS_StealthLevelChangedSignature OnStealthLevelChanged;

    // Broadcast when the score changes.
    UPROPERTY(BlueprintAssignable, Category="Stealth")
    FSOTS_StealthScoreChangedSignature OnStealthScoreChanged;

    // Broadcast when the tier changes with transition context.
    UPROPERTY(BlueprintAssignable, Category="Stealth")
    FSOTS_StealthTierTransitionSignature OnStealthTierChanged;

    // Broadcast when the binary detection state changes.
    UPROPERTY(BlueprintAssignable, Category="Stealth")
    FSOTS_PlayerDetectionStateChangedSignature OnPlayerDetectionStateChanged;

    UPROPERTY(BlueprintAssignable, Category="SOTS|GSM|AI")
    FSOTS_OnAISuspicionReported OnAISuspicionReported;

    UPROPERTY(BlueprintAssignable, Category="SOTS|GSM|AI")
    FSOTS_OnAIAwarenessStateChanged OnAIAwarenessStateChanged;

    UPROPERTY(BlueprintAssignable, Category="SOTS|GSM|AI")
    FSOTS_OnAIRecordUpdated OnAIRecordUpdated;

    UPROPERTY(BlueprintAssignable, Category="SOTS|GSM|Global")
    FSOTS_OnGlobalAlertnessChanged OnGlobalAlertnessChanged;

public:
    /**
     * Main entry point for the player/dragon: submit a new stealth sample.
     * This recalculates the score and, if thresholds are crossed, fires events.
     */
    UFUNCTION(BlueprintCallable, Category="Stealth")
    void ReportStealthSample(const FSOTS_StealthSample& Sample);

    /**
     * Called by AI or mission logic whenever an enemy explicitly detects or loses the player.
     * This drives the bPlayerDetected flag and the detection delegate.
     */
    UFUNCTION(BlueprintCallable, Category="Stealth")
    void ReportEnemyDetectionEvent(AActor* Source, bool bDetectedNow);

    UFUNCTION(BlueprintPure, Category="Stealth")
    float GetCurrentStealthScore() const { return CurrentStealthScore; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    ESOTSStealthLevel GetCurrentStealthLevel() const { return CurrentStealthLevel; }

    UFUNCTION(BlueprintPure, Category="Stealth")
    bool IsPlayerDetected() const { return bPlayerDetected; }

    // Default config asset for this world (can be set on the CDO or via Mission Director).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Config")
    USOTS_StealthConfigDataAsset* DefaultConfigAsset;

    // Active scoring config used at runtime. Copied from the active asset so we can tweak/override if needed.
    UPROPERTY(Transient)
    FSOTS_StealthScoringConfig ActiveConfig;

    // High-level state accessors used by other systems (KEM, FX, dragon).
    UFUNCTION(BlueprintCallable, Category="Stealth")
    const FSOTS_PlayerStealthState& GetStealthState() const { return CurrentState; }

    UFUNCTION(BlueprintCallable, Category="Stealth")
    ESOTS_StealthTier GetStealthTier() const { return CurrentState.StealthTier; }

    // Returns the latest score breakdown snapshot (copied by value).
    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    FSOTS_StealthScoreBreakdown GetCurrentStealthBreakdown() const { return CurrentBreakdown; }

    /** Returns 0-1 final stealth/visibility score for an actor (0=fully hidden, 1=fully exposed). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Stealth")
    float GetStealthScoreFor(const AActor* Actor) const;

    // Called when AI bridge updates suspicion (0..1).
    void SetAISuspicion(float In01);

    // LEGACY: Shim for callers that only send suspicion strength; forwards to ReportAISuspicionEx.
    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth|AI")
    void ReportAISuspicion(AActor* GuardActor, float SuspicionNormalized);

    UFUNCTION(BlueprintCallable, Category="SOTS|GSM|AI")
    void ReportAISuspicionEx(AActor* SubjectAI, float Suspicion01, FGameplayTag ReasonTag, const FVector& Location, bool bHasLocation, AActor* InstigatorActor);

    UFUNCTION(BlueprintPure, Category="SOTS|GSM|AI")
    bool GetLastAISuspicionReport(AActor* SubjectAI, FSOTS_AISuspicionReport& OutReport) const;

    UFUNCTION(BlueprintPure, Category="SOTS|GSM|Global")
    float GetGlobalAlertness() const { return GlobalAlertness; }

    UFUNCTION(BlueprintPure, Category="SOTS|GSM|AI")
    bool GetAIRecord(AActor* SubjectAI, FSOTS_GSM_AIRecord& OutRecord) const;

    UFUNCTION(BlueprintPure, Category="SOTS|GSM|AI")
    ESOTS_AIAwarenessState GetAwarenessStateForAI(AActor* SubjectAI) const;

    // Event-driven update from the player component (no Tick required).
    void UpdateFromPlayer(const FSOTS_PlayerStealthState& PlayerState);

    // C++ delegate for systems that want the full state (FX, dragon, etc.).
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnStealthStateChanged, const FSOTS_PlayerStealthState&);
    FOnStealthStateChanged OnStealthStateChanged;

    // SOTS_Core lifecycle bridge (BRIDGE12): state-only notifications.
    void HandleCoreWorldReady(UWorld* World);
    void HandleCorePrimaryPlayerReady(APlayerController* PC, APawn* Pawn);

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnCoreWorldReady, UWorld*);
    FOnCoreWorldReady OnCoreWorldReady;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCorePrimaryPlayerReady, APlayerController*, APawn*);
    FOnCorePrimaryPlayerReady OnCorePrimaryPlayerReady;

private:
    ESOTSStealthLevel EvaluateStealthLevelFromScore(float Score) const;
    void SetStealthLevel(ESOTSStealthLevel NewLevel);
    void SetPlayerDetected(bool bDetectedNow);

    void RecomputeGlobalScore();
    void UpdateGameplayTags();

    USOTS_PlayerStealthComponent* FindPlayerStealthComponent() const;
    void SyncPlayerStealthComponent();

    struct FGSM_ModifierEntry
    {
        FSOTS_StealthModifier Modifier;
        FSOTS_GSM_Handle Handle;
        int32 Priority = 0;
        double CreatedTime = 0.0;
        FGameplayTag OwnerTag;
        bool bEnabled = true;
    };

    struct FGSM_ConfigEntry
    {
        TObjectPtr<USOTS_StealthConfigDataAsset> Asset = nullptr;
        FSOTS_GSM_Handle Handle;
        int32 Priority = 0;
        double CreatedTime = 0.0;
        FGameplayTag OwnerTag;
        bool bEnabled = true;
    };

    // Active modifiers applied on top of raw state.
    TArray<FGSM_ModifierEntry> ModifierEntries;

    // Ingestion bookkeeping for throttling/logging.
    TMap<ESOTS_StealthInputType, double> LastAcceptedSampleTime;
    TMap<ESOTS_StealthInputType, double> LastLoggedDropTime;
    TMap<ESOTS_StealthInputType, double> LastLoggedIngestTime;

    // Per-guard AI suspicion (normalized [0..1]) used to build the
    // aggregated AISuspicion term in the stealth score.
    UPROPERTY()
    TMap<TWeakObjectPtr<AActor>, float> GuardSuspicion;

    UPROPERTY()
    TMap<TWeakObjectPtr<AActor>, FSOTS_AISuspicionReport> LastAISuspicionReports;

    UPROPERTY()
    TMap<TWeakObjectPtr<AActor>, FSOTS_GSM_AIRecord> AIRecords;

    // Stack of config assets used for scoped overrides (e.g., per-mission).
    TArray<FGSM_ConfigEntry> ConfigEntries;

    // Currently active config asset driving ActiveConfig (may be null to mean "no override").
    UPROPERTY()
    TObjectPtr<USOTS_StealthConfigDataAsset> ActiveStealthConfig = nullptr;
    FSOTS_StealthScoringConfig BaseConfig;

public:
    UFUNCTION(BlueprintCallable, Category="Stealth|Modifiers")
    FSOTS_GSM_Handle AddStealthModifier(const FSOTS_StealthModifier& Modifier, FGameplayTag OwnerTag = FGameplayTag(), int32 Priority = 0);

    UFUNCTION(BlueprintCallable, Category="Stealth|Modifiers")
    ESOTS_GSM_RemoveResult RemoveStealthModifierByHandle(const FSOTS_GSM_Handle& Handle, FGameplayTag RequesterTag = FGameplayTag());

    UFUNCTION(BlueprintCallable, Category="Stealth|Modifiers")
    int32 RemoveStealthModifierBySource(FName SourceId);

    // Config API for Mission Director / difficulty systems.
    UFUNCTION(BlueprintCallable, Category="Stealth|Config")
    void SetStealthConfig(const FSOTS_StealthScoringConfig& InConfig);

    UFUNCTION(BlueprintCallable, Category="Stealth|Config")
    void SetStealthConfigAsset(USOTS_StealthConfigDataAsset* InAsset);

    // Scoped config override API (e.g., used by MissionDirector).
    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    FSOTS_GSM_Handle PushStealthConfig(USOTS_StealthConfigDataAsset* NewConfig, int32 Priority = 0, FGameplayTag OwnerTag = FGameplayTag());

    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    ESOTS_GSM_RemoveResult PopStealthConfig(const FSOTS_GSM_Handle& Handle, FGameplayTag RequesterTag = FGameplayTag());

    UFUNCTION(BlueprintCallable, Category="Stealth")
    void ResetStealthState(ESOTS_GSM_ResetReason Reason);

    // Introspection helpers (dev-safe, no logging).
    UFUNCTION(BlueprintCallable, Category="Stealth|Debug")
    void GetActiveModifierHandles(TArray<FSOTS_GSM_Handle>& OutHandles) const;

    UFUNCTION(BlueprintCallable, Category="Stealth|Debug")
    void GetActiveConfigHandles(TArray<FSOTS_GSM_Handle>& OutHandles) const;

    UFUNCTION(BlueprintCallable, Category="Stealth|Debug")
    FSOTS_GSM_TuningSummary GetEffectiveTuningSummary() const;

    UFUNCTION(BlueprintCallable, Category="Stealth|Debug")
    void DebugDumpStackToLog() const;

    UFUNCTION(BlueprintCallable, Category="Stealth|Lighting")
    void SetDominantDirectionalLightDirectionWS(FVector InDirWS, bool bValid);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Stealth|Shadow")
    FSOTS_ShadowCandidate GetPlayerShadowCandidate() const;

    // Debug helper for BP/UI tooling.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Stealth|Shadow")
    void GetShadowCandidateDebugState(
        bool& bOutHasDominantDir,
        FVector& OutDominantDir,
        bool& bOutCandidateValid,
        FVector& OutCandidatePoint,
        float& OutCandidateIllum01) const;

    UFUNCTION(BlueprintPure, Category="Stealth|Debug")
    FGameplayTag GetLastReasonTag() const { return LastReasonTag; }

    UFUNCTION(BlueprintPure, Category="Stealth|Tags")
    FGameplayTag GetTierTag(ESOTS_StealthTier Tier) const;

    void BuildProfileData(FSOTS_GSMProfileData& OutData) const;
    void ApplyProfileData(const FSOTS_GSMProfileData& InData);
    virtual void BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot) override;
    virtual void ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot) override;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    bool IngestSample(const FSOTS_StealthInputSample& Sample, FSOTS_StealthIngestReport& OutReport);
    void LogIngestDecision(const FSOTS_StealthIngestReport& Report, const FSOTS_StealthInputSample& Sample, bool bAccepted);
    double GetWorldTimeSecondsSafe() const;
    float GetMinSecondsBetweenType(const FSOTS_StealthIngestTuning& Tuning, ESOTS_StealthInputType Type) const;
    void ResolveEffectiveConfig();
    void ApplyStealthStateTags(AActor* TargetActor, ESOTS_StealthTier NewTier, float NewScore, const FSOTS_StealthTierTransition& Transition);
    bool ActorHasTag(AActor* Actor, const FGameplayTag& Tag) const;
    ESOTS_AIAwarenessState ResolveAwarenessStateFromSuspicion(float Suspicion01) const;
    FGameplayTag GetAwarenessTierTag(ESOTS_AIAwarenessState State) const;
    FGameplayTag GetFocusTagForTarget(AActor* TargetActor) const;
    void ApplyAwarenessTags(const FSOTS_GSM_AIRecord& Record);
    void PruneInvalidAIRecords();
    void AppendSuspicionEvent(FSOTS_GSM_AIRecord& Record, const FSOTS_AISuspicionReport& Report, double NowSeconds);
    void PruneOldEvents(FSOTS_GSM_AIRecord& Record, double NowSeconds) const;
    float ComputeEvidencePoints(const FSOTS_GSM_AIRecord& Record, const FSOTS_AISuspicionReport& Incoming, const FSOTS_GSM_EvidenceConfig& Cfg) const;
    FGameplayTag RequestTagSafe(const TCHAR* TagName) const;
    bool ShouldLogMissingTag(const FName& TagName) const;
    void ApplyGlobalAlertnessFromReport(float IncomingSuspicion, ESOTS_AIAwarenessState NewState, float EvidenceBonus, double TimestampSeconds);
    void UpdateGlobalAlertnessDecay();
    void StartGlobalAlertnessDecayTimer();
    float GetGlobalAlertnessTierWeight(ESOTS_AIAwarenessState State) const;
    bool ApplyGlobalAlertnessTags(float Value);
    void SetGlobalAlertnessInternal(float NewValue, double TimestampSeconds, bool bForceBroadcast = false);

    void UpdateShadowCandidateForPlayerIfNeeded();
    AActor* FindPlayerActor() const;

private:
    FSOTS_ShadowCandidate CachedPlayerShadowCandidate;
    double NextShadowCandidateUpdateTimeSeconds = 0.0;
    double NextShadowCandidateDebugDrawTimeSeconds = 0.0;
    double LastTierChangeTimeSeconds = 0.0;
    double LastScoreUpdateTimeSeconds = 0.0;
    double LastAlertingInputTimeSeconds = 0.0;
    double LastGlobalAlertnessUpdateTimeSeconds = 0.0;
    float SmoothedStealthScore = 0.0f;
    float LastBroadcastStealthScore = 0.0f;
    float GlobalAlertness = 0.0f;
    FGameplayTag LastReasonTag;
    FGameplayTag LastGlobalAlertnessTag;
    ESOTS_StealthTier LastAppliedTier = ESOTS_StealthTier::Hidden;
    FGameplayTag LastAppliedTierTag;
    FTimerHandle GlobalAlertnessDecayTimerHandle;
    mutable TSet<FName> MissingTagWarnings;

    FVector DominantDirectionalLightDirWS = FVector::ForwardVector;
    bool bHasDominantDirectionalLightDir = false;

    TWeakObjectPtr<UWorld> LastCoreWorld;
    TWeakObjectPtr<APlayerController> LastCorePC;
    TWeakObjectPtr<APawn> LastCorePawn;
};
