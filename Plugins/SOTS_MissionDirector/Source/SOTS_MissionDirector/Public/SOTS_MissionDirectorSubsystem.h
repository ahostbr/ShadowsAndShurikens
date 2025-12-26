#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_MissionDirectorTypes.h"
#include "GameplayTagContainer.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_ProfileSnapshotProvider.h"
#include "SOTS_ProfileTypes.h"
#include "TimerManager.h"
#include "SOTS_MissionDirectorSubsystem.generated.h"

class AActor;
class USOTS_GlobalStealthManagerSubsystem;
class USOTS_ShaderWarmupSubsystem;
struct FSOTS_MissionProfileData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSOTS_MissionScoreChangedSignature, float, NewScore, float, Delta, FGameplayTagContainer, ContextTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_MissionEventLoggedSignature, const FSOTS_MissionEventLogEntry&, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_MissionEndedSignature, const FSOTS_MissionRunSummary&, Summary);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSOTS_MissionSimpleEventSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnObjectiveUpdatedSignature, FName, ObjectiveId);

/**
 * Central mission orchestrator and scoring/logging subsystem for SOTS.
 *
 * Current architecture:
 * - Data: USOTS_MissionDefinition assets describe mission id, objectives,
 *   stealth constraints, failure conditions, FX tags, and rewards. They are
 *   content-only and reference maps via soft object pointers.
 * - Runtime: USOTS_MissionDirectorSubsystem tracks the active mission state,
 *   objective completion, stealth/alert metrics (via GSM), and a scored event
 *   log for debrief UI and profile saves.
 * - Integration: other systems (GSM, AI perception, KEM, Ability/SkillTree,
 *   FX, Tag manager) report high-level events via tags or explicit calls;
 *   the director reacts by updating objectives, failing/finishing missions,
 *   and triggering FX/rewards, without owning level scripts or AI behavior.
 */
UCLASS()
class SOTS_MISSIONDIRECTOR_API USOTS_MissionDirectorSubsystem : public UGameInstanceSubsystem, public ISOTS_ProfileSnapshotProvider
{
    GENERATED_BODY()

public:
    USOTS_MissionDirectorSubsystem();

    // Convenience accessor.
    UFUNCTION(BlueprintCallable, Category="Mission", meta=(WorldContext="WorldContextObject"))
    static USOTS_MissionDirectorSubsystem* Get(const UObject* WorldContextObject);

    // Is a mission currently active?
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
    bool bMissionActive;

    // Current mission id.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
    FName CurrentMissionId;

    // Difficulty tag used for this run.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
    FGameplayTag CurrentDifficultyTag;

    // World time when the mission started.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
    float MissionStartTimeSeconds;

    // Running score.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
    float CurrentScore;

    // Log entries for the active mission.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
    TArray<FSOTS_MissionEventLogEntry> EventLog;

    // Objective counters.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
    int32 PrimaryObjectivesCompleted;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission")
    int32 OptionalObjectivesCompleted;

    // Delegates
    UPROPERTY(BlueprintAssignable, Category="Mission")
    FSOTS_MissionScoreChangedSignature OnScoreChanged;

    UPROPERTY(BlueprintAssignable, Category="Mission")
    FSOTS_MissionEventLoggedSignature OnEventLogged;

    UPROPERTY(BlueprintAssignable, Category="Mission")
    FSOTS_MissionEndedSignature OnMissionEnded;

public:
    /** Starts a new mission run. Resets all state and begins logging. */
    UFUNCTION(BlueprintCallable, Category="Mission")
    void StartMissionRun(FName MissionId, FGameplayTag InDifficultyTag);

    // Mission travel entrypoint that waits for shader warmup completion before opening the level.
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void RequestMissionTravelWithWarmup(FName TargetLevelPackageName);

    // Optional abort path if travel is cancelled by UI or back-out flow.
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void CancelWarmupTravel();

    /** Records an objective completion. */
    UFUNCTION(BlueprintCallable, Category="Mission")
    void RegisterObjectiveCompleted(bool bIsPrimaryObjective, float ScoreDelta, FText Title, FText Description, const FGameplayTagContainer& ContextTags);

    /**
     * Generic scoring hook for anything: stealth bonuses, detections, loot, time penalties, etc.
     * Systems can call this directly instead of building their own scoring.
     */
    UFUNCTION(BlueprintCallable, Category="Mission")
    void RegisterScoreEvent(ESOTSMissionEventCategory Category, float ScoreDelta, FText Title, FText Description, const FGameplayTagContainer& ContextTags);

    /** Ends the mission and builds a summary, which you can pass to your debrief UI. */
    UFUNCTION(BlueprintCallable, Category="Mission")
    void EndMissionRun(bool bSuccess, FSOTS_MissionRunSummary& OutSummary);

    /** Lightweight score-only accessor (no summary build). */
    UFUNCTION(BlueprintPure, Category="Mission")
    float GetCurrentScore() const { return CurrentScore; }

    UFUNCTION(BlueprintPure, Category="Mission")
    bool IsMissionActive() const { return bMissionActive; }

    // Total play seconds accumulated by MissionDirector (authoritative).
    UFUNCTION(BlueprintPure, Category="Mission")
    float GetTotalPlaySeconds() const;

    // --- SOTS_Core bridge hooks (state-only; called by optional Core bridge listener) ---
    void HandleCoreWorldStartPlay(UWorld* World);
    void HandleCorePrimaryPlayerReady(APlayerController* PC, APawn* Pawn);
    void HandleCorePreLoadMap(const FString& MapName);
    void HandleCorePostLoadMap(UWorld* World);

    // --- Mission definition / objective tracking API ---

    /** Starts a mission based on a USOTS_MissionDefinition (objectives + stealth rules). */
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void StartMission(USOTS_MissionDefinition* MissionDef);

    /** Marks any objectives whose CompletionTag matches CompletedTag as complete. */
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void CompleteObjectiveByTag(const FGameplayTag& CompletedTag);

    /** Explicitly fails the current mission (e.g., scripted failure). */
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void FailMission(const FGameplayTag& FailReasonTag);

    /** Explicitly aborts the current mission (e.g., back-out flow). */
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void AbortMission(const FGameplayTag& AbortReasonTag);

    /** Returns the high-level mission state driven by the mission definition. */
    UFUNCTION(BlueprintPure, Category="SOTS|Mission")
    ESOTS_MissionState GetMissionState() const { return MissionState; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    USOTS_MissionDefinition* GetActiveMissionDefinition() const { return ActiveMission; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    TArray<FSOTS_MissionObjectiveRuntimeState> GetCurrentMissionObjectives() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    bool IsObjectiveCompleted(FName ObjectiveId) const;

    // Generic mission event hook. Routes through the progress intake and legacy
    // completion-by-tag so objectives can react without direct subsystem calls.
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void NotifyMissionEvent(const FGameplayTag& EventTag);

    // Normalized ingress point for external systems to feed mission progress events.
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void PushMissionProgressEvent(const FSOTS_MissionProgressEvent& Event);

    // Explicitly fails a specific objective, optionally with a debug reason.
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void ForceFailObjective(FName ObjectiveId, const FString& Reason);

    // Simple alert counter hook (e.g. when AI enters fully alerted state).
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void NotifyAlertTriggered();

    // High-level runtime state snapshot for UI/save systems.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    FSOTS_MissionRuntimeState GetCurrentMissionState() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    FSOTS_MissionRuntimeState GetCurrentMissionSaveState() const { return GetCurrentMissionState(); }

    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void RestoreMissionFromSave(const FSOTS_MissionRuntimeState& SavedState);

    void BuildProfileData(FSOTS_MissionProfileData& OutData) const;
    void ApplyProfileData(const FSOTS_MissionProfileData& InData);
    virtual void BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot) override;
    virtual void ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot) override;

    // Reads the next mission id configured for the current outcome tag, if any.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    bool GetNextMissionIdByOutcome(FName& OutMissionId) const;

    // --- Save/Profile helpers ---

    // Returns a lightweight result struct for the currently active or last
    // mission run. This is suitable for per-mission profile persistence.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    FSOTS_MissionRunSummary GetCurrentMissionResult() const;

    // Serializes mission runtime state (objectives, outcome, metrics) for
    // a specific mission id. Currently this mirrors GetCurrentMissionState
    // while allowing profile systems to store multiple missions.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    void ExportMissionState(FName MissionId, FSOTS_MissionRuntimeState& OutState) const;

    // Restores mission runtime state previously exported for a mission id.
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void ImportMissionState(const FSOTS_MissionRuntimeState& InState);

private:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    float GetTimeSinceStart(const UObject* WorldContextObject) const;
    void AppendEventInternal(const FSOTS_MissionEventLogEntry& Entry);
    FName EvaluateRankFromScore(float FinalScore) const;
    bool SetMissionState(ESOTS_MissionState NewState, double TimestampSeconds);
    void DispatchMissionRewards(const FSOTS_MissionRewards& Rewards, double TimestampSeconds);
    void StartTotalPlaySecondsTimer();
    void StopTotalPlaySecondsTimer();
    void HandleTotalPlaySecondsTick();

    // Mission-definition-driven helpers.
    void EvaluateMissionCompletion();
    void InitializeMissionRuntimeState(const USOTS_MissionDefinition* MissionDef);
    void ResetConditionTracking();
    void HandleProgressEvent(const FSOTS_MissionProgressEvent& Event);
    void ApplyProgressToObjective(const USOTS_ObjectiveDefinition* ObjectiveDef, bool bIsGlobalObjective, const FSOTS_RouteId& RouteId, const FSOTS_MissionProgressEvent& Event, double NowSeconds);
    bool EvaluateObjectiveConditions(const USOTS_ObjectiveDefinition* ObjectiveDef, const FSOTS_RouteId& RouteId, bool bIsGlobalObjective, double NowSeconds, bool& bOutFailed, bool& bOutCompleted) const;
    bool IsConditionSatisfied(const FSOTS_ObjectiveCondition& Condition, const FName& ConditionKey, double NowSeconds) const;
    FName BuildConditionKey(const FSOTS_ObjectiveId& ObjectiveId, const FSOTS_ObjectiveCondition& Condition) const;
    bool SetObjectiveState(const FSOTS_ObjectiveId& ObjectiveId, const FSOTS_RouteId& RouteId, bool bIsGlobalObjective, ESOTS_ObjectiveState NewState, double TimestampSeconds);
    bool ValidateMissionDefinition(const USOTS_MissionDefinition* MissionDef, FString& OutError) const;
    bool AreObjectiveRequirementsSatisfied(const TArray<FSOTS_ObjectiveId>& RequiresCompleted) const;
    bool IsObjectiveIdCompleted(const FSOTS_ObjectiveId& ObjectiveId) const;
    void ClearConditionTrackingForObjective(const FSOTS_ObjectiveId& ObjectiveId);
    void HandleMissionTerminalState();
    USOTS_RouteDefinition* GetActiveRouteDefinition() const;
    void ActivateDefaultRoute();
    void OnConditionDurationElapsed(FName ConditionKey, FSOTS_ObjectiveId ObjectiveId, bool bIsGlobalObjective, FSOTS_RouteId RouteId);
    const USOTS_ObjectiveDefinition* FindObjectiveDefinition(const FSOTS_ObjectiveId& ObjectiveId, const FSOTS_RouteId& RouteId, bool bIsGlobalObjective) const;
    FSOTS_MissionMilestoneSnapshot BuildMilestoneSnapshot(double NowSeconds) const;
    void TryWriteMilestoneToStatsAndProfile(const FSOTS_MissionMilestoneSnapshot& Snapshot);
    void EmitMissionUIIntent(const FText& Message, FGameplayTag CategoryTag) const;
    void EmitObjectiveUIIntent(const USOTS_ObjectiveDefinition* ObjectiveDef, bool bIsGlobalObjective, const FSOTS_RouteId& RouteId, ESOTS_ObjectiveState NewState) const;
    void EmitMissionTerminalUIIntent(ESOTS_MissionState NewState) const;

    UFUNCTION()
    void HandleStealthLevelChanged(ESOTSStealthLevel OldLevel, ESOTSStealthLevel NewLevel, float NewScore);

    UFUNCTION()
    void HandleAIAwarenessStateChanged(AActor* SubjectAI, ESOTS_AIAwarenessState OldState, ESOTS_AIAwarenessState NewState, const FSOTS_GSM_AIRecord& Record);

    UFUNCTION()
    void HandleGlobalAlertnessChanged(float NewValue, float OldValue);

    // KEM integration: invoked when the KillExecutionManager broadcasts an
    // execution event so objectives with RequiredExecutionTag / RequiredTargetTag
    // can be evaluated.
    UFUNCTION()
    void HandleExecutionEvent(const struct FSOTS_KEM_ExecutionEvent& Event);

    UFUNCTION()
    void HandleWarmupReadyToTravel();

    UFUNCTION()
    void HandleWarmupCancelled(FText Reason);

    void ClearMissionStealthConfigOverride();

private:
    // Active authored mission (if any).
    UPROPERTY(Transient)
    TObjectPtr<USOTS_MissionDefinition> ActiveMission;

    // High-level mission lifecycle state.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|Mission", meta=(AllowPrivateAccess="true"))
    ESOTS_MissionState MissionState = ESOTS_MissionState::NotStarted;

    // Objective completion flags keyed by ObjectiveId.
    UPROPERTY()
    TMap<FName, bool> ObjectiveCompletion;

    UPROPERTY()
    FSOTS_GSM_Handle MissionStealthConfigHandle;

    // Objective failure flags keyed by ObjectiveId.
    UPROPERTY()
    TMap<FName, bool> ObjectiveFailed;

    // Cached pointer so we can unbind safely on shutdown.
    UPROPERTY()
    USOTS_GlobalStealthManagerSubsystem* CachedStealthSubsystem = nullptr;

    // Outcome + analytics fields used for branching and debrief.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|Mission", meta=(AllowPrivateAccess="true"))
    FGameplayTag CurrentOutcomeTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|Mission", meta=(AllowPrivateAccess="true"))
    int32 StealthScore = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|Mission", meta=(AllowPrivateAccess="true"))
    int32 AlertsTriggered = 0;

    // Condition/runtime tracking for prompt 4 triggers.
    UPROPERTY()
    TMap<FName, int32> ConditionCountsByKey;

    UPROPERTY()
    TMap<FName, double> ConditionStartTimeByKey;

    UPROPERTY()
    TMap<FName, FTimerHandle> ConditionTimerHandles;

    UPROPERTY()
    TMap<FName, FSOTS_ObjectiveRuntimeState> GlobalObjectiveStatesById;

    UPROPERTY()
    TMap<FName, FSOTS_RouteRuntimeState> RouteStatesById;

    UPROPERTY()
    FSOTS_RouteId ActiveRouteId;

    // Persistence/UI bookkeeping.
    UPROPERTY()
    TArray<FSOTS_MissionMilestoneSnapshot> MilestoneHistory;

    bool bLoggedMissingStatsOnce = false;
    bool bLoggedMissingUIRouterOnce = false;

    UPROPERTY()
    bool bWarmupTravelPending = false;

    UPROPERTY()
    FName PendingWarmupTargetLevelPackage = NAME_None;

    UPROPERTY()
    TWeakObjectPtr<USOTS_ShaderWarmupSubsystem> ActiveWarmupSubsystem;

    // Profile-oriented mirrors of mission progression.
    UPROPERTY()
    FName ActiveMissionIdForProfile;

    UPROPERTY()
    TArray<FName> CompletedMissionIds;

    UPROPERTY()
    TArray<FName> FailedMissionIds;

    UPROPERTY()
    FName LastMissionIdForProfile;

    UPROPERTY()
    float LastFinalScoreForProfile = 0.0f;

    UPROPERTY()
    float LastDurationSecondsForProfile = 0.0f;

    UPROPERTY()
    bool bLastMissionCompletedForProfile = false;

    UPROPERTY()
    bool bLastMissionFailedForProfile = false;

    // Core bridge cached state (state-only; no behavior owned here).
    UPROPERTY()
    TWeakObjectPtr<UWorld> CoreBridgeWorld;

    UPROPERTY()
    TWeakObjectPtr<APlayerController> CoreBridgePrimaryPC;

    UPROPERTY()
    TWeakObjectPtr<APawn> CoreBridgePrimaryPawn;

    UPROPERTY()
    FString CoreBridgeLastPreLoadMapName;

    float TotalPlaySeconds = 0.0f;
    double LastPlaySecondsSampleTime = 0.0;
    FTimerHandle TotalPlaySecondsTimerHandle;
    bool bMissionRunEnded = false;
    bool bRewardsDispatched = false;

public:
    // Mission events (separate from scoring/debrief).
    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_MissionSimpleEventSignature OnMissionStarted;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_MissionSimpleEventSignature OnMissionCompleted;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_MissionSimpleEventSignature OnMissionFailed;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_MissionSimpleEventSignature OnMissionAborted;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_OnObjectiveUpdatedSignature OnObjectiveUpdated;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_OnMissionStateChanged OnMissionStateChanged;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_OnObjectiveStateChanged OnObjectiveStateChanged;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_OnRouteActivated OnRouteActivated;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_OnMissionRewardIntent OnMissionRewardIntent;
};
