#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_MissionDirectorTypes.h"
#include "GameplayTagContainer.h"
#include "SOTS_MissionDirectorSubsystem.generated.h"

class AActor;
class USOTS_GlobalStealthManagerSubsystem;
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
class SOTS_MISSIONDIRECTOR_API USOTS_MissionDirectorSubsystem : public UGameInstanceSubsystem
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

    /** Returns the high-level mission state driven by the mission definition. */
    UFUNCTION(BlueprintPure, Category="SOTS|Mission")
    ESOTS_MissionState GetMissionState() const { return MissionState; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    USOTS_MissionDefinition* GetActiveMissionDefinition() const { return ActiveMission; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    TArray<FSOTS_MissionObjectiveRuntimeState> GetCurrentMissionObjectives() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Mission")
    bool IsObjectiveCompleted(FName ObjectiveId) const;

    // Generic mission event hook. Currently this forwards to CompleteObjectiveByTag
    // so that objectives can be completed by broadcasting tags rather than
    // calling the subsystem directly.
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission")
    void NotifyMissionEvent(const FGameplayTag& EventTag);

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

    // Mission-definition-driven helpers.
    void EvaluateMissionCompletion();

    UFUNCTION()
    void HandleStealthLevelChanged(ESOTSStealthLevel OldLevel, ESOTSStealthLevel NewLevel, float NewScore);

    // KEM integration: invoked when the KillExecutionManager broadcasts an
    // execution event so objectives with RequiredExecutionTag / RequiredTargetTag
    // can be evaluated.
    UFUNCTION()
    void HandleExecutionEvent(const struct FSOTS_KEM_ExecutionEvent& Event);

private:
    // Active authored mission (if any).
    UPROPERTY(Transient)
    TObjectPtr<USOTS_MissionDefinition> ActiveMission;

    // High-level mission lifecycle state.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|Mission", meta=(AllowPrivateAccess="true"))
    ESOTS_MissionState MissionState = ESOTS_MissionState::Inactive;

    // Objective completion flags keyed by ObjectiveId.
    UPROPERTY()
    TMap<FName, bool> ObjectiveCompletion;

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

public:
    // Mission events (separate from scoring/debrief).
    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_MissionSimpleEventSignature OnMissionStarted;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_MissionSimpleEventSignature OnMissionCompleted;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_MissionSimpleEventSignature OnMissionFailed;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Mission")
    FSOTS_OnObjectiveUpdatedSignature OnObjectiveUpdated;
};
