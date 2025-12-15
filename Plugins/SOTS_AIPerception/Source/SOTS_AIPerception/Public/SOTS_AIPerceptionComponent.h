#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_AIPerceptionTypes.h"
#include "SOTS_AIGuardPerceptionDataAsset.h"
#include "SOTS_AIPerceptionComponent.generated.h"

class USOTS_AIPerceptionConfig;
class USkeletalMeshComponent;
class USOTS_GlobalStealthManagerSubsystem;

/**
 * Per-AI perception component. Tracks awareness of important targets
 * (player, dragon, etc.) and exposes a simple state machine for BT/BAT.
 */
UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_AIPERCEPTION_API USOTS_AIPerceptionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_AIPerceptionComponent();

    // DevTools: Keep this component as the canonical UAIPerception →
    // TagManager/GSM bridge so per-guard detection tagging stays centralized.

    // --- CONFIG ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Config")
    TObjectPtr<USOTS_AIPerceptionConfig> PerceptionConfig;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI")
    TObjectPtr<USOTS_AIGuardPerceptionDataAsset> GuardConfig = nullptr;

    // Classes this AI should track (Player, Dragon, etc.).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Config")
    TArray<TSubclassOf<AActor>> WatchedActorClasses;

    // Perception update interval (seconds). Kept separate from Tick.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Config", meta=(ClampMin="0.01"))
    float PerceptionUpdateInterval;

    // --- RUNTIME STATE (READ-ONLY) ---

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="State")
    ESOTS_PerceptionState CurrentState;

    // --- Core API ---

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    ESOTS_PerceptionState GetCurrentPerceptionState() const { return CurrentState; }

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    float GetAwarenessForTarget(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    bool HasLineOfSightToTarget(AActor* Target) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    FSOTS_PerceivedTargetState GetTargetState(AActor* Target, bool& bFound) const;

    // Normalized suspicion value [0,1] for this guard.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|AI")
    float GetCurrentSuspicion01() const;

    // Lightweight Blueprint helpers for key telemetry without exposing internals.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|AI")
    bool IsDetected() const { return bIsDetected; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|AI")
    FGameplayTag GetReasonSenseTag() const { return LastStimulusCache.SenseTag; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|AI")
    FVector GetLastStimulusLocation() const { return LastStimulusCache.WorldLocation; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|AI")
    AActor* GetLastStimulusActor() const { return LastStimulusCache.SourceActor.Get(); }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|AI")
    double GetLastStimulusTimeSeconds() const { return LastStimulusCache.TimestampSeconds; }

    // --- SCRIPTED CONTROL ---

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void SuppressPerceptionForDuration(float Seconds);

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void ForceAlertToLocation(FVector Location);

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void ForceForgetTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void ResetPerceptionState(ESOTS_AIPerceptionResetReason ResetReason = ESOTS_AIPerceptionResetReason::Manual);

    // Convenience hooks so profile/mission systems can clear state explicitly.
    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void ResetForProfileLoaded() { ResetPerceptionState(ESOTS_AIPerceptionResetReason::ProfileLoaded); }

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void ResetForMissionStart() { ResetPerceptionState(ESOTS_AIPerceptionResetReason::MissionStart); }

    /**
     * Public hook for DevTools and other systems to report perception state
     * changes without re-implementing TagManager/GSM wiring.
     */
    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void ReportDetectionLevelChanged(ESOTS_PerceptionState NewState);

    /**
     * Notifies the GlobalStealthManager when this guard toggles Alerted.
     */
    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void ReportAlertStateChanged(bool bAlertedNow);

    // --- EVENTS ---

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnPerceptionStateChanged, ESOTS_PerceptionState, NewState);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_OnTargetPerceptionChanged, AActor*, Target, ESOTS_PerceptionState, NewState);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSOTS_OnTargetSpotted);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSOTS_OnTargetLost);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnSuspicionChanged, float, NewSuspicion01);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnAIPerceptionSpotted, const FSOTS_AIPerceptionTelemetry&, Telemetry);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnAIPerceptionLost, const FSOTS_AIPerceptionTelemetry&, Telemetry);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnAIPerceptionSuspicionChanged, const FSOTS_AIPerceptionTelemetry&, Telemetry);

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnPerceptionStateChanged OnPerceptionStateChanged;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnTargetPerceptionChanged OnTargetPerceptionChanged;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnTargetSpotted OnTargetSpotted;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnTargetLost OnTargetLost;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnSuspicionChanged OnSuspicionChanged;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnAIPerceptionSpotted OnAIPerceptionSpotted;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnAIPerceptionLost OnAIPerceptionLost;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnAIPerceptionSuspicionChanged OnAIPerceptionSuspicionChanged;

    // Internal: called by subsystem to apply noise stim.
    void HandleReportedNoise(const FVector& Location, float Loudness);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnUnregister() override;

private:
    // Last reason used when clearing caches; useful for debugging lifecycle.
    ESOTS_AIPerceptionResetReason LastResetReason = ESOTS_AIPerceptionResetReason::Unknown;

    enum class ESOTS_LocalSense : uint8
    {
        Unknown,
        Sight,
        Hearing,
        Shadow,
        Damage
    };

    void UpdatePerception();

    void UpdateSingleTarget(FSOTS_PerceivedTargetState& TargetState, float DeltaSeconds, bool bDebugDrawTargetPoints);

    int32 GetNextTargetStartIndex(int32 NumTargets);

    FSOTS_TargetPointVisibilityResult EvaluateTargetVisibility_MultiPoint(AActor* Target, const FVector& EyeLocation, ECollisionChannel TraceChannel, const USOTS_AIPerceptionConfig* Config, bool bDrawDebug, float DebugDrawDuration);
    bool ComputeTargetLOS(AActor* Target, const FVector& EyeLocation, ECollisionChannel TraceChannel, FSOTS_TargetPointVisibilityResult& OutVisibility, bool bDrawDebug, float DebugDrawDuration);

    static USkeletalMeshComponent* FindBestSkeletalMeshComponent(AActor* Target);
    bool GatherTargetPointWorldLocations(AActor* Target, const TArray<FName>& BoneNames, int32 MaxPoints, TArray<FVector>& OutPoints, TArray<bool>& OutIsCorePoint) const;
    float ComputeVisibilitySuspicionMultiplier(const FSOTS_TargetPointVisibilityResult& Vis, const USOTS_AIPerceptionConfig* Config) const;

    void RefreshWatchedTargets();

    void UpdateBlackboardAndTags();

    void ApplyLOSStateTags(bool bHasLOS);
    void ApplyPerceptionStateTags(ESOTS_PerceptionState NewState);
    void NotifyGlobalStealthManager(ESOTS_PerceptionState OldState, ESOTS_PerceptionState NewState);
    void SetPerceptionState(ESOTS_PerceptionState NewState);
    void ApplyStateTags();
    bool ShouldRunShadowAwareness(const USOTS_AIPerceptionConfig* Config, double NowSeconds);
    void LogTelemetrySnapshot(int32 NumTargets, int32 EvaluatedTargetsCount, int32 MaxTargetsToEvaluate, bool bHasLOSOnPrimary, const FSOTS_PerceivedTargetState* BestState, float SuspicionNormalized);
    AActor* ResolvePrimaryTarget();
    bool IsPrimaryTarget(const AActor* Actor) const;
    void UpdateLastStimulusCache(ESOTS_LocalSense Sense, float Strength01, const FVector& WorldLocation, AActor* SourceActor, bool bSuccessfullySensed);
    FGameplayTag MapSenseToReasonTag(ESOTS_LocalSense Sense) const;
    void EvaluateStimuliForTarget(float SightStrength, float HearingStrength, float ShadowStrength, ESOTS_LocalSense& OutDominantSense, float& OutDominantStrength, bool& bOutHasStimulus) const;

    void InitializePerceptionTimer();
    void ClearPerceptionTimers();
    void ResetInternalState(ESOTS_AIPerceptionResetReason ResetReason, bool bResetTargets);

    // Internal hook used to translate high-level perception state changes
    // into optional FX cues (and any future side-effects).
    void HandlePerceptionStateChanged(ESOTS_PerceptionState OldState, ESOTS_PerceptionState NewState);
    float UpdateSuspicionModel(float DeltaSeconds, float SightStrength, float HearingStrength, float ShadowStrength, ESOTS_LocalSense DominantSense, FGameplayTag& OutDominantReasonTag, bool& bOutDetectedChanged, bool& bOutDetectedNow);
    FSOTS_AIPerceptionTelemetry BuildTelemetrySnapshot(float SuspicionNormalized, const FGameplayTag& ReasonTag) const;
    void BroadcastSuspicionTelemetry(const FSOTS_AIPerceptionTelemetry& Telemetry, bool bDetectedChanged);

    USOTS_GlobalStealthManagerSubsystem* ResolveGSM();
    void ReportSuspicionToGSM(float AISuspicion01, const FGameplayTag& ReasonTag, const FVector* OptionalLocation, bool bForceReport);

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> WatchedActors;

    UPROPERTY()
    TMap<TWeakObjectPtr<AActor>, FSOTS_PerceivedTargetState> TargetStates;

    TWeakObjectPtr<AActor> PrimaryTarget;

    FTimerHandle PerceptionTimerHandle;
    FTimerHandle SuppressionTimerHandle;

    bool bPerceptionSuppressed;

    // Suspicion values for this guard, driven by GuardConfig.
    float CurrentSuspicion;
    float PreviousSuspicion;

    // Suspicion stability bookkeeping.
    double LastStimulusTimeSeconds = 0.0;
    double LastDetectionChangeTimeSeconds = 0.0;
    double LastDetectionEnterTimeSeconds = 0.0;
    double LastSuspicionEventTimeSeconds = 0.0;
    float LastSuspicionEventValue = -1.0f;
    bool bIsDetected = false;
    float PendingHearingStrength = 0.0f;
    float LastSuspicionNormalized = 0.0f;
    FSOTS_LastPerceptionStimulus LastStimulusCache;

    // Last perception state observed for this guard. Used to detect
    // transitions and drive optional FX cues.
    UPROPERTY(Transient)
    ESOTS_PerceptionState PreviousState = ESOTS_PerceptionState::Unaware;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|AI|Debug", meta=(AllowPrivateAccess="true"))
    bool bDebugSuspicion = false;

    // Cached curve pointer for stealth detection multiplier.
    UPROPERTY(Transient)
    UCurveFloat* StealthCurveCached;

    // Optional reference to higher-level behavior component (internal only for now).
    TWeakObjectPtr<UActorComponent> AIBT_BehaviorComponentRef;

    // Trace budget tracking for future multi-point LOS.
    int32 TracesUsedThisUpdate = 0;
    int32 TargetRoundRobinIndex = 0;

    void ResetTraceBudget() { TracesUsedThisUpdate = 0; }
    bool CanSpendTraces(int32 Count) const;
    void SpendTrace() { ++TracesUsedThisUpdate; }

    // Shadow awareness scheduling.
    double NextShadowCheckTimeSeconds = 0.0;
    int32 ShadowTracesUsedThisSecond = 0;
    int32 ShadowTraceSecondStamp = 0;

    // Telemetry scheduling.
    double NextTelemetryTimeSeconds = 0.0;

    // GSM reporting throttle state.
    TWeakObjectPtr<USOTS_GlobalStealthManagerSubsystem> CachedGSM;
    double LastGSMReportTimeSeconds = 0.0;
    float LastReportedSuspicion01 = -1.0f;
    bool bForceNextGSMReport = false;
    bool bHasPendingGSMLocation = false;
    FVector PendingGSMLocation = FVector::ZeroVector;
    FGameplayTag PendingGSMReasonTag;
    FGameplayTag LastGSMReasonTag;
};
