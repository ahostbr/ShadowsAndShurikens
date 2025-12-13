#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_AIPerceptionTypes.h"
#include "SOTS_AIGuardPerceptionDataAsset.h"
#include "SOTS_AIPerceptionComponent.generated.h"

class USOTS_AIPerceptionConfig;
class USkeletalMeshComponent;

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

    // --- SCRIPTED CONTROL ---

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void SuppressPerceptionForDuration(float Seconds);

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void ForceAlertToLocation(FVector Location);

    UFUNCTION(BlueprintCallable, Category="SOTS|Perception")
    void ForceForgetTarget(AActor* Target);

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

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnPerceptionStateChanged OnPerceptionStateChanged;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Perception")
    FSOTS_OnTargetPerceptionChanged OnTargetPerceptionChanged;

    // Internal: called by subsystem to apply noise stim.
    void HandleReportedNoise(const FVector& Location, float Loudness);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
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

    // Internal hook used to translate high-level perception state changes
    // into optional FX cues (and any future side-effects).
    void HandlePerceptionStateChanged(ESOTS_PerceptionState OldState, ESOTS_PerceptionState NewState);

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> WatchedActors;

    UPROPERTY()
    TMap<TWeakObjectPtr<AActor>, FSOTS_PerceivedTargetState> TargetStates;

    FTimerHandle PerceptionTimerHandle;
    FTimerHandle SuppressionTimerHandle;

    bool bPerceptionSuppressed;

    // Suspicion values for this guard, driven by GuardConfig.
    float CurrentSuspicion;
    float PreviousSuspicion;

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
};
