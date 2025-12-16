#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SOTS_StealthConfigDataAsset.h"
#include "SOTS_MissionDirectorTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ESOTSMissionEventCategory : uint8
{
    Objective   UMETA(DisplayName="Objective"),
    Stealth     UMETA(DisplayName="Stealth"),
    Combat      UMETA(DisplayName="Combat"),
    Time        UMETA(DisplayName="Time"),
    Loot        UMETA(DisplayName="Loot"),
    Exploration UMETA(DisplayName="Exploration"),
    Misc        UMETA(DisplayName="Misc"),
    Custom      UMETA(DisplayName="Custom")
};

// High-level mission lifecycle state used by the mission director.
UENUM(BlueprintType)
enum class ESOTS_MissionState : uint8
{
    NotStarted  UMETA(DisplayName="Not Started"),
    InProgress  UMETA(DisplayName="In Progress"),
    Completed   UMETA(DisplayName="Completed"),
    Failed      UMETA(DisplayName="Failed"),
    Aborted     UMETA(DisplayName="Aborted")
};

UENUM(BlueprintType)
enum class ESOTS_ObjectiveState : uint8
{
    Inactive    UMETA(DisplayName="Inactive"),
    Active      UMETA(DisplayName="Active"),
    Completed   UMETA(DisplayName="Completed"),
    Failed      UMETA(DisplayName="Failed"),
    Abandoned   UMETA(DisplayName="Abandoned")
};

// Simple type flag so designers can mark which objectives are mandatory.
UENUM(BlueprintType)
enum class ESOTS_ObjectiveType : uint8
{
    Mandatory   UMETA(DisplayName="Mandatory"),
    Optional    UMETA(DisplayName="Optional")
};

USTRUCT(BlueprintType)
struct FSOTS_MissionId
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Mission")
    FName Id = NAME_None;
};

USTRUCT(BlueprintType)
struct FSOTS_RouteId
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Mission")
    FName Id = NAME_None;
};

USTRUCT(BlueprintType)
struct FSOTS_ObjectiveId
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Mission")
    FName Id = NAME_None;
};

// Normalized transient event consumed by MissionDirector to drive objective progress.
USTRUCT(BlueprintType)
struct FSOTS_MissionProgressEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Progress")
    FGameplayTag EventTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Progress")
    TWeakObjectPtr<AActor> InstigatorActor;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Progress")
    bool bHasLocation = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Progress")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Progress")
    double TimestampSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Progress")
    float Value01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Progress")
    FName NameId = NAME_None;
};

// Lightweight snapshot written to Stats/Profile at mission milestones.
USTRUCT(BlueprintType)
struct FSOTS_MissionMilestoneSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Milestone")
    FSOTS_MissionId MissionId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Milestone")
    ESOTS_MissionState MissionState = ESOTS_MissionState::NotStarted;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Milestone")
    FSOTS_RouteId ActiveRouteId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Milestone")
    TArray<FSOTS_ObjectiveId> CompletedObjectives;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Milestone")
    TArray<FSOTS_ObjectiveId> FailedObjectives;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission|Milestone")
    double TimestampSeconds = 0.0;
};

// High-level mission completion state used for save/profile systems.
UENUM(BlueprintType)
enum class ESOTS_MissionCompletionState : uint8
{
    NotStarted  UMETA(DisplayName="Not Started"),
    InProgress  UMETA(DisplayName="In Progress"),
    Completed   UMETA(DisplayName="Completed"),
    Failed      UMETA(DisplayName="Failed")
};

/**
 * Single line item in the mission run log.
 * This is what you will show to the player at mission end.
 */
USTRUCT(BlueprintType)
struct FSOTS_MissionEventLogEntry
{
    GENERATED_BODY()

    // Seconds since the mission started when this event occurred.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    float TimeSinceStart = 0.0f;

    // Category for grouping/filtering in UI.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    ESOTSMissionEventCategory Category = ESOTSMissionEventCategory::Misc;

    // Short, UI-friendly title (e.g. "Primary Objective Completed").
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    FText Title;

    // Optional longer description for tooltips/details.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    FText Description;

    // Positive or negative score delta applied by this event.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    float ScoreDelta = 0.0f;

    // Score after applying this delta (for running total display).
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    float CumulativeScore = 0.0f;

    // True if this was a penalty event (for red text in UI).
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    bool bIsPenalty = false;

    // Optional tags so other systems can filter or post-process (e.g. Mission.Modifier.NoKills).
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    FGameplayTagContainer ContextTags;
};

/**
 * Final snapshot of a mission run. This is what you save or pass to your
 * mission debrief UI.
 */
USTRUCT(BlueprintType)
struct FSOTS_MissionRunSummary
{
    GENERATED_BODY()

    // Mission identifier (e.g. "M01_CastleInfiltration").
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    FName MissionId;

    // Difficulty tag used for this run (e.g. Difficulty.Medium).
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    FGameplayTag DifficultyTag;

    // How long the mission took in seconds.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    float DurationSeconds = 0.0f;

    // Final accumulated score after all modifiers.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    float FinalScore = 0.0f;

    // Rank name (e.g. "S", "A", "B", etc.). You can map this to localized text in UI.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    FName FinalRank;

    // True if mission was completed successfully.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    bool bSuccess = false;

    // Number of primary objectives completed.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    int32 PrimaryObjectivesCompleted = 0;

    // Number of optional objectives completed.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    int32 OptionalObjectivesCompleted = 0;

    // Full chronological event log for this run.
    UPROPERTY(BlueprintReadOnly, Category="Mission")
    TArray<FSOTS_MissionEventLogEntry> EventLog;
};

/**
 * Design-time description of a single mission objective.
 */
USTRUCT(BlueprintType)
struct FSOTS_MissionObjective
{
    GENERATED_BODY()

    // Local identifier for this objective within the mission.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FName ObjectiveId;

    // Whether this objective must be completed to finish the mission.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    ESOTS_ObjectiveType Type = ESOTS_ObjectiveType::Mandatory;

    // Legacy single completion tag (kept for backwards-compatibility).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FGameplayTag CompletionTag;

    // Tags/flags used by MissionDirector to detect completion (e.g. Mission.Objective.CollectedIntel).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FGameplayTagContainer CompletionTags;

    // Optional objectives that must be completed before this one activates.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    TArray<FName> PrerequisiteObjectiveIds;

    // Optional descriptive text for UI.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FText Description;

    // Whether this objective is optional from the player's perspective.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    bool bOptional = false;

    // If true and this objective fails, the mission fails immediately.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    bool bMissionCritical = false;

    // Optional required execution tag (from KEM) for objectives such as
    // "Assassinate target X with execution Y". When set, the mission director
    // will listen for matching KEM execution events and mark this objective
    // complete when a successful execution with this tag occurs.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|KEM")
    FGameplayTag RequiredExecutionTag;

    // Optional tag that must be present on the KEM target actor for this
    // objective to be considered satisfied (e.g. Target.Assassination.Boss).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|KEM")
    FGameplayTag RequiredTargetTag;
};

// Data-driven condition entry for objective progress/failure.
USTRUCT(BlueprintType)
struct FSOTS_ObjectiveCondition
{
    GENERATED_BODY()

    // Canonical event tag required to advance this condition.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Condition")
    FGameplayTag RequiredEventTag;

    // Optional id-like discriminator (item id, execution id, stat key, etc.).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Condition")
    FName RequiredNameId = NAME_None;

    // Minimum number of matching events required.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Condition")
    int32 RequiredCount = 1;

    // If > 0, requires this condition to remain valid for the duration window after the first match.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Condition")
    float DurationSeconds = 0.0f;

    // If true, satisfying this condition fails the objective instead of completing it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Condition")
    bool bIsFailureCondition = false;
};

// Golden-path objective DataAsset (Prompt 2). Existing FSOTS_MissionObjective remains for legacy flows.
UCLASS(BlueprintType)
class SOTS_MISSIONDIRECTOR_API USOTS_ObjectiveDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_ObjectiveId ObjectiveId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|UI")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|UI")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    bool bIsOptional = false;

    // If empty, objective is valid for all routes.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Routing")
    TArray<FSOTS_RouteId> AllowedRoutes;

    // Simple gating: requires these objectives completed first.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Gating")
    TArray<FSOTS_ObjectiveId> RequiresCompleted;

    // Placeholder for failability; evaluation is implemented in later prompts.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Failure")
    bool bCanFail = true;

    // Data-driven event conditions required to complete/fail this objective.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Conditions")
    TArray<FSOTS_ObjectiveCondition> Conditions;

    // When true, all non-failure conditions must be satisfied; otherwise any single non-failure condition completes the objective.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Conditions")
    bool bAllConditionsRequired = true;
};

// Golden-path route DataAsset (Prompt 2).
UCLASS(BlueprintType)
class SOTS_MISSIONDIRECTOR_API USOTS_RouteDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_RouteId RouteId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|UI")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|UI")
    FText Description;

    // Objectives belonging to this route.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Objectives")
    TArray<TObjectPtr<USOTS_ObjectiveDefinition>> Objectives;
};

// Lightweight runtime view of a mission objective for UI/query code.
USTRUCT(BlueprintType)
struct FSOTS_MissionObjectiveRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FName ObjectiveId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FText Description;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    ESOTS_ObjectiveType Type = ESOTS_ObjectiveType::Mandatory;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    bool bCompleted = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    bool bFailed = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    bool bActive = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    bool bOptional = false;
};

// Golden-path runtime objective state (Prompt 2). Keeps timestamps for analytics/debug.
USTRUCT(BlueprintType)
struct FSOTS_ObjectiveRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_ObjectiveId ObjectiveId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    ESOTS_ObjectiveState State = ESOTS_ObjectiveState::Inactive;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    double ActivatedTimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    double CompletedTimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    double FailedTimeSeconds = 0.0;
};

// Golden-path runtime route state (Prompt 2).
USTRUCT(BlueprintType)
struct FSOTS_RouteRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_RouteId RouteId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    bool bIsActive = false;

    // Map keyed by ObjectiveId.Id for BP friendliness.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    TMap<FName, FSOTS_ObjectiveRuntimeState> ObjectiveStatesById;
};

// High-level snapshot of the current mission runtime state.
USTRUCT(BlueprintType)
struct FSOTS_MissionRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_MissionId MissionId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    TArray<FSOTS_MissionObjectiveRuntimeState> Objectives;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    bool bMissionCompleted = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    bool bMissionFailed = false;

    // Outcome tag used for branching (e.g. MissionOutcome.Perfect / Failed).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FGameplayTag OutcomeTag;

    // Simple aggregate metrics for analytics / UI.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    int32 StealthScore = 0;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    int32 OptionalObjectivesCompleted = 0;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    int32 AlertsTriggered = 0;

    // Golden-path mission state fields (Prompt 2).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    ESOTS_MissionState State = ESOTS_MissionState::NotStarted;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_RouteId ActiveRouteId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    TMap<FName, FSOTS_ObjectiveRuntimeState> GlobalObjectiveStatesById;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    TMap<FName, FSOTS_RouteRuntimeState> RouteStatesById;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    double StartTimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    double EndTimeSeconds = 0.0;
};

/**
 * Optional per-mission failure condition overrides beyond the basic
 * stealth tier checks.
 */
USTRUCT(BlueprintType)
struct FSOTS_MissionFailureConditions
{
    GENERATED_BODY()

    // If > 0, the mission fails once AlertsTriggered exceeds this value.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Failure")
    int32 MaxAllowedAlerts = 0;

    // If true, any civilian death (surfaced as a mission event with a matching
    // tag) should be treated as an immediate failure. MissionDirector itself
    // only sees tags; actual kill classification is done by KEM/AI systems.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Failure")
    bool bFailOnCivilianKilled = false;

    // If true, any non-quiet execution (tagged by KEM) will fail the mission
    // when the associated event reaches the director.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Failure")
    bool bFailOnLoudExecution = false;

    // Tags that, when present in a mission event's ContextTags, should cause
    // the mission to fail (e.g. Mission.Fail.NoKillRunBroken).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Failure")
    FGameplayTagContainer FailOnEventTags;
};

/**
 * Optional mission rewards applied when the mission completes successfully.
 * These are data-only; MissionDirector coordinates with other subsystems
 * (Tag manager, SkillTree, Ability system, FX) to interpret them.
 */
USTRUCT(BlueprintType)
struct FSOTS_MissionRewards
{
    GENERATED_BODY()

    // Global tags to grant on successful completion (e.g. Mission.M01.Completed,
    // meta progression flags, modifiers for later runs).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Rewards")
    FGameplayTagContainer GrantedTags;

    // Skill tags to apply via the skill tree/tag systems. These typically map
    // to FSOTS_SkillNodeEffects.GrantedTags or ability prerequisites.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Rewards")
    FGameplayTagContainer GrantedSkillTags;

    // Ability tags to grant via the ability system. The MissionDirector does
    // not know about concrete ability classes; it only passes these tags on
    // to higher-level game logic that can talk to UAC_SOTS_Abilitys.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Rewards")
    FGameplayTagContainer GrantedAbilityTags;

    // Optional skill tree nodes to unlock as rewards. These are authored as
    // (TreeId, NodeId) pairs so the skill tree subsystem can process them.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Rewards")
    TArray<FName> RewardSkillTreeIds;

    // Optional FX tag that can be used for reward-specific FX (distinct from
    // the mission completed FX tag). Mapped via the FX manager.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Rewards")
    FGameplayTag FXTag_OnRewardsGranted;
};

// Stable delegate payloads (Prompt 2)
USTRUCT(BlueprintType)
struct FSOTS_MissionStateChangedPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_MissionId MissionId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    ESOTS_MissionState OldState = ESOTS_MissionState::NotStarted;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    ESOTS_MissionState NewState = ESOTS_MissionState::NotStarted;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    double TimestampSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct FSOTS_ObjectiveStateChangedPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_MissionId MissionId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_RouteId RouteId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    bool bIsGlobalObjective = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_ObjectiveId ObjectiveId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    ESOTS_ObjectiveState OldState = ESOTS_ObjectiveState::Inactive;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    ESOTS_ObjectiveState NewState = ESOTS_ObjectiveState::Inactive;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    double TimestampSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct FSOTS_RouteActivatedPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_MissionId MissionId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_RouteId RouteId;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Mission")
    double TimestampSeconds = 0.0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnMissionStateChanged, const FSOTS_MissionStateChangedPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnObjectiveStateChanged, const FSOTS_ObjectiveStateChangedPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnRouteActivated, const FSOTS_RouteActivatedPayload&, Payload);
/**
 * DataAsset describing mission-level rules, objectives, and stealth constraints.
 */
UCLASS(BlueprintType)
class SOTS_MISSIONDIRECTOR_API USOTS_MissionDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    // Identifier for this mission (golden-path id). Legacy MissionId (FName) retained for compatibility.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_MissionId MissionIdentifier;

    // Legacy mission id for existing content.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FName MissionId;

    // Display name used in UI.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FText MissionName;

    // Optional long description for briefing / UI.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FText MissionDescription;

    // Soft reference to the primary map/level associated with this mission.
    // This is data-only; the mission director does not perform level loading.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    TSoftObjectPtr<UWorld> MissionMap;

    // Ordered list of authored objectives (legacy inline authoring).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    TArray<FSOTS_MissionObjective> Objectives;

    // Golden-path routes (first-class mission structure).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Routing")
    TArray<TObjectPtr<USOTS_RouteDefinition>> Routes;

    // Global objectives that apply regardless of route.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Objectives")
    TArray<TObjectPtr<USOTS_ObjectiveDefinition>> GlobalObjectives;

    // Max allowed global stealth tier (inclusive). If >= 0 and the GSM tier exceeds this, the mission fails.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Stealth")
    int32 MaxAllowedStealthTier = 0;

    // If true, entering the highest stealth level (FullyDetected) fails the mission immediately.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Stealth")
    bool bFailOnMaxTier = true;

    // If true, all mandatory objectives must be completed to finish the mission.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Stealth")
    bool bRequireAllMandatoryObjectives = true;

    // Optional override for the global stealth scoring config while this mission is active.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Stealth")
    TObjectPtr<USOTS_StealthConfigDataAsset> OverrideStealthConfig = nullptr;

    // Optional FX tags to fire when mission state changes. These are purely
    // descriptive; the FX manager maps them to concrete systems via data.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|FX")
    FGameplayTag FXTag_OnMissionStart;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|FX")
    FGameplayTag FXTag_OnMissionCompleted;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|FX")
    FGameplayTag FXTag_OnMissionFailed;

    // Optional branching configuration: which missions can follow this one,
    // keyed by an outcome tag (e.g. MissionOutcome.Perfect, MissionOutcome.Failed).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    TMap<FGameplayTag, FName> NextMissionByOutcome;

    // Optional, data-only failure conditions in addition to the basic stealth
    // tier checks. These are evaluated by the mission director when relevant
    // events are reported.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_MissionFailureConditions FailureConditions;

    // Optional rewards applied when the mission completes successfully.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission")
    FSOTS_MissionRewards Rewards;

    // Placeholder for mission-level failure enable/disable (logic in later prompts).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Mission|Failure")
    bool bMissionCanFail = true;
};
