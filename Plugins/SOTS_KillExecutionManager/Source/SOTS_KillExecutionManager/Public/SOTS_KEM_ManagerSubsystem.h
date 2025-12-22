
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_LooseTagHandle.h"
#include "SOTS_KEM_OmniTraceTuning.h"
#include "SOTS_OmniTraceKEMPresetLibrary.h"
#include "TimerManager.h"
#include "SOTS_KEM_ManagerSubsystem.generated.h"

class USOTS_AbilityRequirementLibraryAsset;
struct FSOTS_AbilityRequirementCheckResult;
class UContextualAnimSceneAsset;
class ASOTS_KEMExecutionAnchor;
class UAnimMontage;
class USOTS_KEM_ExecutionCatalog;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
struct FStreamableHandle;
class USOTS_InteractionSubsystem;
struct FSOTS_InteractionActionRequest;

USTRUCT(BlueprintType)
struct FSOTS_KEMAnchorDebugInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    FString AnchorName;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    FGameplayTag PositionTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    ESOTS_KEM_ExecutionFamily ExecutionFamily = ESOTS_KEM_ExecutionFamily::Unknown;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    float UseRadius = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    FVector Forward = FVector::ForwardVector;
};

DECLARE_LOG_CATEGORY_EXTERN(LogSOTS_KEM, Log, All);

UENUM(BlueprintType)
enum class EKEMDebugVerbosity : uint8
{
    None    UMETA(DisplayName="None"),
    Basic   UMETA(DisplayName="Basic"),
    Verbose UMETA(DisplayName="Verbose")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnExecutionEvent, const FSOTS_KEM_ExecutionEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnKEMExecutionCompleted, const FSOTS_KEMCompletionPayload&, Payload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FSOTS_OnKEMCameraRequest,
                                              FGameplayTag, ExecutionTag,
                                              AActor*, Instigator,
                                              AActor*, Target,
                                              FGameplayTag, PrimaryCameraTag,
                                              FGameplayTag, SecondaryCameraTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSOTS_OnKEMQTERequested,
                                              FGameplayTag, ExecutionTag,
                                              AActor*, Instigator,
                                              AActor*, Target,
                                              AActor*, Witness);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSOTS_OnKEMQTEResolved,
                                              FGameplayTag, ExecutionTag,
                                              AActor*, Instigator,
                                              AActor*, Target,
                                              ESOTS_KEMQTEOutcome, Outcome);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnKEMStartGateFailed, const FSOTS_KEMStartGateResult&, Result);


DECLARE_DYNAMIC_MULTICAST_DELEGATE_EightParams(FSOTS_KEM_CASExecutionChosenSignature,
                                               USOTS_KEM_ExecutionDefinition*, ExecutionDef,
                                               UContextualAnimSceneAsset*, Scene,
                                               AActor*, Instigator,
                                               AActor*, Target,
                                               FName, SectionName,
                                               FName, InstigatorRole,
                                               FName, TargetRole,
                                               FTransform, InstigatorWarpTarget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSOTS_KEM_LevelSequenceExecutionChosenSignature,
                                              USOTS_KEM_ExecutionDefinition*, ExecutionDef,
                                              AActor*, Instigator,
                                              AActor*, Target,
                                              FSOTS_ExecutionContext, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSOTS_KEM_AISExecutionChosenSignature,
                                              USOTS_KEM_ExecutionDefinition*, ExecutionDef,
                                              AActor*, Instigator,
                                              AActor*, Target,
                                              FSOTS_ExecutionContext, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FSOTS_OnKEMLevelSequenceFinished,
                                              FGuid, RequestId,
                                              FGameplayTag, ExecutionTag,
                                              AActor*, Instigator,
                                              AActor*, Target,
                                              bool, bSucceeded);

DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnKEMExecutionTelemetry, const FSOTS_KEMExecutionTelemetry&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnKEMExecutionTelemetryBP, const FSOTS_KEMExecutionTelemetry&, Telemetry);

UCLASS(Config=Game)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEMManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    USOTS_KEMManagerSubsystem();
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM", meta=(WorldContext="WorldContextObject"))
    static USOTS_KEMManagerSubsystem* Get(const UObject* WorldContextObject);

    // Gameplay-friendly entry points keep the BP API surface small.
    bool RequestExecution(const UObject* WorldContextObject,
                          AActor* Instigator,
                          AActor* Target,
                          const FGameplayTagContainer& ContextTags,
                          const USOTS_KEM_ExecutionDefinition* ExecutionOverride = nullptr,
                          const FString& SourceLabel = TEXT("Blueprint"));

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM", meta=(WorldContext="WorldContextObject"))
    bool RequestExecution_WithResult(const UObject* WorldContextObject,
                                     AActor* Instigator,
                                     AActor* Target,
                                     const FGameplayTagContainer& ContextTags,
                                     const USOTS_KEM_ExecutionDefinition* ExecutionOverride,
                                     const FString& SourceLabel,
                                     FSOTS_KEMStartGateResult& OutGateResult);

    /** Unified, blessed entrypoint for executions from player/dragon/cutscene/Sequencer. */
    UFUNCTION(BlueprintCallable, Category="SOTS|KEM", meta=(WorldContext="WorldContextObject"))
    bool RequestExecution_Blessed(UObject* WorldContextObject,
                                  AActor* InstigatorActor,
                                  AActor* TargetActor,
                                  FGameplayTag ExecutionTag,
                                  FName ContextReason = NAME_None,
                                  bool bAllowFallback = true);

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM")
    bool RequestExecution_FromPlayer(AActor* Instigator, AActor* Target);

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM")
    bool RequestExecution_FromDragon(AActor* Instigator, AActor* Target);

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM")
    bool RequestExecution_FromCinematic(AActor* Instigator, AActor* Target, UObject* ExecutionDefinitionOverride);

    // Input wiring reminder (player input blueprints): Interact triggers RequestExecution_FromPlayer, Dragon input triggers RequestExecution_FromDragon so the "3 allowed entrypoints" rule stays visible.

    // Debug helper, logs why definitions passed/failed
    UFUNCTION(BlueprintCallable, Category="KEM|Debug", meta=(WorldContext="WorldContextObject"))
    void RunKEMDebug(const UObject* WorldContextObject,
                     AActor* Instigator,
                     AActor* Target,
                     const FGameplayTagContainer& ContextTags) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Validation")
    FSOTS_KEMValidationResult ValidateExecutionDefinition(const USOTS_KEM_ExecutionDefinition* Def) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM", meta=(WorldContext="WorldContextObject"))
    FSOTS_KEMStartGateResult EvaluateStartGate(const UObject* WorldContextObject,
                                               AActor* Instigator,
                                               AActor* Target,
                                               const FGameplayTagContainer& ContextTags) const;

    UFUNCTION(Exec)
    void KEM_SelfTest();

    UFUNCTION(Exec)
    void KEM_DumpCoverage();

    UFUNCTION(Exec)
    void KEM_ToggleAnchorDebug();

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Debug")
    void GetNearbyAnchorsForDebug(AActor* CenterActor,
                                  float Radius,
                                  TArray<FSOTS_KEMAnchorDebugInfo>& OutAnchors) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Debug")
    void DrawAnchorDebugVisualization(AActor* CenterActor, float Radius) const;

    UFUNCTION(BlueprintPure, Category="KEM")
    ESOTS_KEMState GetCurrentState() const { return CurrentState; }

    UFUNCTION(BlueprintPure, Category="KEM")
    bool IsSaveBlocked() const { return bSaveBlocked; }

    // Returns a copy of the recent debug records (most recent first).
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="KEM|Debug")
    void GetRecentKEMDebugRecords(TArray<FSOTS_KEMDebugRecord>& OutRecords) const;

    // Returns the candidate breakdown for the most recent selection.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="KEM|Debug")
    void GetLastKEMCandidateDebug(TArray<FSOTS_KEMCandidateDebugRecord>& OutCandidates) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="KEM|Debug")
    void GetLastDecisionSnapshot(FSOTS_KEMDecisionSnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Debug")
    void GetLastKEMDecisionSummary(FString& OutSummary) const;

    // Candidate entries encode ExecutionName, status, score, and reject/failure reason for the last selection.
    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Debug")
    void GetLastKEMCandidates(TArray<FString>& OutCandidates) const;

    // Called externally when an execution fully ends (e.g. CAS montage finished, helper destroyed).
    // This drives the SuccessCooldown / FailureCooldown state before returning to Ready.
    UFUNCTION(BlueprintCallable, Category="KEM", meta=(BlueprintInternalUseOnly="true"))
    void NotifyExecutionEnded(bool bSuccess);

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM")
    void NotifyExecutionWitnessed(AActor* WitnessActor);

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM")
    void NotifyExecutionQTEResult(bool bSuccess);

    // Backend-agnostic lifecycle hook that includes the execution context
    // and definition so listeners can react (FX, analytics, etc.).
    // Note: this is a pure C++ helper (not exposed as a UFUNCTION) to avoid
    // name clashes with the simpler Blueprint-facing NotifyExecutionEnded(bool).
    void NotifyExecutionEnded(const FSOTS_ExecutionContext& Context,
                              const USOTS_KEM_ExecutionDefinition* Def,
                              bool bWasSuccessful);

    // Emergency escape hatch to reset the state machine back to Ready (clears any cooldown).
    UFUNCTION(BlueprintCallable, Category="KEM", meta=(BlueprintInternalUseOnly="true"))
    void ForceResetState();

    // Optional runtime setter for the shared ability requirement library.
    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Ability", meta=(BlueprintInternalUseOnly="true"))
    void SetAbilityRequirementLibrary(USOTS_AbilityRequirementLibraryAsset* InLibrary);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Gameplay")
    FGameplayTagContainer PlayerContextTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Gameplay")
    FGameplayTagContainer DragonContextTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Gameplay")
    FGameplayTagContainer CinematicContextTags;

    // Global start gate: required tags that must be present on the request context.
    UPROPERTY(EditAnywhere, Config, Category="SOTS|KEM|Gate")
    FGameplayTagContainer StartGate_RequiredContextTags;

    // Global start gate: tags that hard-block the request context.
    UPROPERTY(EditAnywhere, Config, Category="SOTS|KEM|Gate")
    FGameplayTagContainer StartGate_BlockedContextTags;

    // Global start gate: tag blocks evaluated on the instigator's union tag view.
    UPROPERTY(EditAnywhere, Config, Category="SOTS|KEM|Gate")
    FGameplayTagContainer StartGate_BlockingTags_Instigator;

    // Global start gate: tag blocks evaluated on the target's union tag view.
    UPROPERTY(EditAnywhere, Config, Category="SOTS|KEM|Gate")
    FGameplayTagContainer StartGate_BlockingTags_Target;

    // Optional override: tags that indicate the target is alerted (execution hard-block).
    UPROPERTY(EditAnywhere, Config, Category="SOTS|KEM|Gate")
    FGameplayTagContainer StartGate_TargetAlertedTags;

public:
    // Execution definitions to evaluate. Soft so they can live in Content.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    TArray<TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>> ExecutionDefinitions;

    UPROPERTY(EditAnywhere, Category="KEM")
    TSoftObjectPtr<USOTS_KEM_ExecutionCatalog> ExecutionCatalog;

    // Optional ability requirement library used to resolve requirements by
    // AbilityRequirementTag. If null, definitions that opt into ability
    // requirements but do not provide inline data will treat gating as
    // unconfigured and pass by default.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Ability")
    TObjectPtr<USOTS_AbilityRequirementLibraryAsset> AbilityRequirementLibrary;

    // Optional default registry config. If set, you can call InitializeFromDefaultRegistryConfig()
    // (e.g. from your GameInstance) to populate ExecutionDefinitions and the internal ID map.

    UPROPERTY(EditAnywhere, Config, Category="KEM|Debug")
    bool bEnableKEMDebug = true;

    UPROPERTY(EditAnywhere, Config, Category="KEM|Telemetry")
    bool bEnableTelemetryLogging = false;

    UPROPERTY(EditAnywhere, Config, Category="KEM|Debug")
    EKEMDebugVerbosity DebugVerbosity = EKEMDebugVerbosity::Basic;

    /** Optional dev-only warning when dispatch delegates have no listeners. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="KEM|Debug")
    bool bWarnOnUnboundDispatchDelegates = false;

    /** Optional dev-only log for height rejections. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="KEM|Debug")
    bool bDebugLogHeightRejections = false;

    // Tag broadcast when a KEM execution completes; emitted via TagManager.
    UPROPERTY(EditAnywhere, Config, Category="SOTS|KEM|Events")
    FName KEMFinishedEventTagName = TEXT("SAS.MissionEvent.KEM.Execution");

    // Duration to hold the completion tag before auto-removing it.
    UPROPERTY(EditAnywhere, Config, Category="SOTS|KEM|Events", meta=(ClampMin="0.0"))
    float KEMFinishedEventTagHoldSeconds = 0.1f;

    UPROPERTY(EditAnywhere, Config, Category="KEM|Debug", meta=(ClampMin="0.0"))
    float AnchorSearchRadius = 600.f;

    /** Optional: apply anchor EnvContextTags to the request ContextTags before evaluation. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="KEM|Anchor")
    bool bUseAnchorEnvContextTags = false;

    /** Optional: bias scoring toward an anchor's preferred executions. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="KEM|Anchor")
    bool bUseAnchorPreferredExecutions = false;

    /** Score bonus added when an anchor's PreferredExecutions influences selection. */
    UPROPERTY(EditAnywhere, Config, Category="KEM|Anchor", meta=(ClampMin="0.0"))
    float AnchorPreferredExecutionBonus = 25000.0f;

    UPROPERTY(EditAnywhere, Category="KEM|Fallback")
    TObjectPtr<UAnimMontage> FallbackMontage = nullptr;

    UPROPERTY(EditAnywhere, Config, Category="KEM|Fallback", meta=(ClampMin="0.0"))
    float FallbackTriggerDistance = 250.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KEM|Debug")
    bool bAnchorDebugDraw = false;

    bool IsDebugAtLeast(EKEMDebugVerbosity Level) const
    {
        return bEnableKEMDebug && static_cast<uint8>(DebugVerbosity) >= static_cast<uint8>(Level);
    }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    TSoftObjectPtr<USOTS_KEM_ExecutionRegistryConfig> DefaultRegistryConfig;

    /** Optional: auto-compute position tags when none are present. Default OFF. */
    UPROPERTY(EditAnywhere, Config, Category="KEM|Selection")
    bool bAutoComputePositionTags = false;

    /** Optional: allow corner quadrants when auto computing. Default true. */
    UPROPERTY(EditAnywhere, Config, Category="KEM|Selection")
    bool bAutoComputeCornerTags = true;

    /** Vertical threshold for auto-computed position tags (Above/Below). */
    UPROPERTY(EditAnywhere, Config, Category="KEM|Selection", meta=(ClampMin="0.0"))
    float AutoPositionVerticalThreshold = 30.f;

    /** Dot threshold for corner classification when auto computing (lower = more permissive). */
    UPROPERTY(EditAnywhere, Config, Category="KEM|Selection", meta=(ClampMin="0.0", ClampMax="1.0"))
    float AutoPositionCornerDotThreshold = 0.35f;

    /** Optional: also broadcast the legacy LevelSequence chosen delegate. Default OFF to avoid double-play. */
    UPROPERTY(EditAnywhere, Config, Category="KEM|LevelSequence")
    bool bBroadcastLegacyLevelSequenceChosen = false;

    // Map of ExecutionId -> ExecutionDefinition for direct lookups. Populated by the registry helpers.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KEM")
    TMap<FName, TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>> ExecutionDefinitionsById;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|OmniTrace")
    TObjectPtr<USOTS_OmniTraceKEMPresetLibrary> OmniTracePresetLibrary;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|OmniTrace")
    TSoftObjectPtr<USOTS_KEM_OmniTraceTuningConfig> OmniTraceTuningConfig;


    // Current high-level state
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KEM")
    ESOTS_KEMState CurrentState = ESOTS_KEMState::Ready;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|KEM")
    FSOTS_KEMStartGateResult LastStartGateResult;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|KEM")
    ESOTS_KEMQTEOutcome ActiveQTEOutcome = ESOTS_KEMQTEOutcome::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|KEM")
    bool bExecutionWitnessed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|KEM")
    bool bSaveBlocked = false;

    // Rolling history of recent executions for debug HUD (most recent first).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KEM|Debug")
    TArray<FSOTS_KEMDebugRecord> RecentDebugRecords;

    // Candidate breakdown for the most recent selection.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KEM|Debug")
    TArray<FSOTS_KEMCandidateDebugRecord> LastCandidateDebug;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="KEM|Debug")
    FSOTS_KEMDecisionSnapshot LastDecisionSnapshot;

    // Index into RecentDebugRecords for the currently active execution, or INDEX_NONE.
    int32 ActiveDebugRecordIndex = INDEX_NONE;

    // Optional cooldowns applied after an execution finishes before returning to Ready.
    UPROPERTY(EditAnywhere, Config, Category="KEM|Timing", meta=(ClampMin="0.0"))
    float SuccessCooldownDuration = 0.25f;

    UPROPERTY(EditAnywhere, Config, Category="KEM|Timing", meta=(ClampMin="0.0"))
    float FailureCooldownDuration = 0.0f;

    // CAS execution picked and ready to start: all info needed for MotionWarping + CAS Start node.
    UPROPERTY(BlueprintAssignable, Category="KEM")
    FSOTS_KEM_CASExecutionChosenSignature OnCASExecutionChosen;

    // LevelSequence-style execution picked. Caller is responsible for actually playing the sequence.
    UPROPERTY(BlueprintAssignable, Category="KEM")
    FSOTS_KEM_LevelSequenceExecutionChosenSignature OnLevelSequenceExecutionChosen;

    // AIS / ability-style execution picked. Caller is responsible for driving AI / gameplay reactions.
    UPROPERTY(BlueprintAssignable, Category="KEM")
    FSOTS_KEM_AISExecutionChosenSignature OnAISExecutionChosen;

    /** Raised when a LevelSequence backend finishes (success or abort). */
    UPROPERTY(BlueprintAssignable, Category="KEM|LevelSequence")
    FSOTS_OnKEMLevelSequenceFinished OnKEMLevelSequenceFinished;

    // Unified lifecycle event surface for executions (started / succeeded / failed).
    UPROPERTY(BlueprintAssignable, Category="SOTS|KEM")
    FSOTS_OnExecutionEvent OnExecutionEvent;

    // Structured outcome payload for suite consumption (MissionDirector/Stats/SkillTree/etc).
    UPROPERTY(BlueprintAssignable, Category="SOTS|KEM")
    FSOTS_OnKEMExecutionCompleted OnExecutionCompleted;

    // Camera request/cleanup surface (no UI ownership).
    UPROPERTY(BlueprintAssignable, Category="SOTS|KEM|Camera")
    FSOTS_OnKEMCameraRequest OnExecutionCameraRequested;

    UPROPERTY(BlueprintAssignable, Category="SOTS|KEM|Camera")
    FSOTS_OnKEMCameraRequest OnExecutionCameraReleased;

    // Witness escalation + QTE request surface.
    UPROPERTY(BlueprintAssignable, Category="SOTS|KEM|QTE")
    FSOTS_OnKEMQTERequested OnExecutionQTERequested;

    UPROPERTY(BlueprintAssignable, Category="SOTS|KEM|QTE")
    FSOTS_OnKEMQTEResolved OnExecutionQTEResolved;

    // Start gate feedback surface for callers that want structured failure info.
    UPROPERTY(BlueprintAssignable, Category="SOTS|KEM|Gate")
    FSOTS_OnKEMStartGateFailed OnStartGateFailed;

    UPROPERTY(BlueprintAssignable, Category="SOTS|KEM|Telemetry")
    FSOTS_OnKEMExecutionTelemetryBP OnExecutionTelemetryBP;

protected:
    // --- Core evaluation helpers ---
    void BuildExecutionContext(AActor* Instigator,
                               AActor* Target,
                               const FGameplayTagContainer& ContextTags,
                               FSOTS_ExecutionContext& OutContext) const;

    bool EvaluateDefinition(const USOTS_KEM_ExecutionDefinition* Def,
                            const FSOTS_ExecutionContext& Context,
                            float& OutScore,
                            FSOTS_CASQueryResult& OutCASResult,
                            ESOTS_KEMRejectReason& OutRejectReason,
                            FString& OutFailReason);

    bool EvaluateTagsAndHeight(const USOTS_KEM_ExecutionDefinition* Def,
                               const FSOTS_ExecutionContext& Context,
                               ESOTS_KEMRejectReason& OutRejectReason,
                               FString& OutFailReason) const;

    bool EvaluateCASDefinition(const USOTS_KEM_ExecutionDefinition* Def,
                               const FSOTS_ExecutionContext& Context,
                               FSOTS_CASQueryResult& OutResult,
                               ESOTS_KEMRejectReason& OutRejectReason,
                               FString& OutFailReason) const;

    // v0.40: Non-CAS backends are stubbed out intentionally
    bool EvaluateSequenceDefinition(const USOTS_KEM_ExecutionDefinition* Def,
                                    const FSOTS_ExecutionContext& Context,
                                    ESOTS_KEMRejectReason& OutRejectReason,
                                    FString& OutFailReason) const;

    bool EvaluateAISDefinition(const USOTS_KEM_ExecutionDefinition* Def,
                               const FSOTS_ExecutionContext& Context,
                               ESOTS_KEMRejectReason& OutRejectReason,
                               FString& OutFailReason) const;

    bool EvaluateSpawnDefinition(const USOTS_KEM_ExecutionDefinition* Def,
                                 const FSOTS_ExecutionContext& Context,
                                 ESOTS_KEMRejectReason& OutRejectReason,
                                 FString& OutFailReason) const;

    bool EvaluateAbilityRequirementsForExecution(const UObject* WorldContextObject,
                                                 const USOTS_KEM_ExecutionDefinition* ExecutionDef,
                                                 FSOTS_AbilityRequirementCheckResult& OutResult) const;

    bool EvaluateStartGateInternal(const UObject* WorldContextObject,
                                   AActor* Instigator,
                                   AActor* Target,
                                   const FGameplayTagContainer& ContextTags,
                                   FSOTS_KEMStartGateResult& OutResult) const;

    bool RequestExecutionInternal(const UObject* WorldContextObject,
                                  AActor* Instigator,
                                  AActor* Target,
                                  const FGameplayTagContainer& ContextTags,
                                  const USOTS_KEM_ExecutionDefinition* ExecutionOverride,
                                  const FString& SourceLabel,
                                  bool bAllowFallback = true,
                                  FSOTS_KEMStartGateResult* OutGateResult = nullptr);


    void FindNearbyExecutionAnchors(AActor* Instigator,
                                    AActor* Target,
                                    float SearchRadius,
                                    TArray<ASOTS_KEMExecutionAnchor*>& OutAnchors) const;

    float ComputeAnchorScoreBonus(const USOTS_KEM_ExecutionDefinition* Def,
                                  const TArray<ASOTS_KEMExecutionAnchor*>& Anchors,
                                  ASOTS_KEMExecutionAnchor*& OutMatchingAnchor) const;

    const FSOTS_KEM_OmniTraceTuning* FindOmniTraceTuning(ESOTS_OmniTraceKEMPreset Preset) const;


    // --- Registry helpers ---

    // Register a single execution definition under a given ID (usually the ExecutionTag's name).
    UFUNCTION(BlueprintCallable, Category="KEM|Registry", meta=(BlueprintInternalUseOnly="true"))
    void RegisterExecutionDefinition(FName ExecutionId, USOTS_KEM_ExecutionDefinition* Definition);

    // Register all definitions from a registry config asset. This will rebuild both the array
    // used by RequestExecution and the internal ID map.
    UFUNCTION(BlueprintCallable, Category="KEM|Registry", meta=(BlueprintInternalUseOnly="true"))
    void RegisterExecutionDefinitionsFromConfig(USOTS_KEM_ExecutionRegistryConfig* Config);

    // Convenience helper that loads DefaultRegistryConfig and calls RegisterExecutionDefinitionsFromConfig.
    UFUNCTION(BlueprintCallable, Category="KEM|Registry", meta=(BlueprintInternalUseOnly="true"))
    void InitializeFromDefaultRegistryConfig();

    // Direct lookup of a definition by its registered ID.
    UFUNCTION(BlueprintCallable, Category="KEM|Registry", meta=(BlueprintInternalUseOnly="true"))
    USOTS_KEM_ExecutionDefinition* FindExecutionDefinitionById(FName ExecutionId);

public:

    bool ExecuteSpawnActorBackend(const USOTS_KEM_ExecutionDefinition* Def,
                                  const FSOTS_ExecutionContext& Context,
                                  const ASOTS_KEMExecutionAnchor* Anchor = nullptr) const;

    bool ResolveWarpPointByName(const USOTS_KEM_ExecutionDefinition* Def,
                                FName WarpTargetName,
                                const FSOTS_ExecutionContext& Context,
                                FTransform& OutTransform,
                                bool bIgnoreDistanceCheck = false) const;

    void LogDebugForDefinition(const USOTS_KEM_ExecutionDefinition* Def,
                               const FSOTS_ExecutionContext& Context,
                               bool bPassed,
                               const FString& Reason) const;

    // --- State / cooldown helpers ---

    void EnterCooldownState(bool bWasSuccessful);
    void OnCooldownExpired();

    FTimerHandle CooldownTimerHandle;

    bool TryPlayFallbackMontage(AActor* Instigator) const;

    void BroadcastExecutionEvent(ESOTS_KEM_ExecutionResult Result,
                                 const FSOTS_ExecutionContext& Context,
                                 const USOTS_KEM_ExecutionDefinition* Def) const;

    void BroadcastExecutionCompleted(const FSOTS_ExecutionContext& Context,
                                     const USOTS_KEM_ExecutionDefinition* Def,
                                     ESOTS_KEM_ExecutionResult Result,
                                     ESOTS_KEMExecutionOutcome Outcome);

    void BroadcastCompletionTag(const FSOTS_ExecutionContext& Context);
    void ClearCompletionEventTag(FSOTS_LooseTagHandle Handle);

    void BroadcastCameraRequest(const FSOTS_ExecutionContext& Context,
                                const USOTS_KEM_ExecutionDefinition* Def);
    void BroadcastCameraRelease();

    void SetSaveBlocked(bool bBlocked);

    FSOTS_OnKEMExecutionTelemetry& GetTelemetryDelegate();
    void BroadcastExecutionTelemetry(const FSOTS_KEMExecutionTelemetry& Telemetry);

    void TriggerExecutionFX(ESOTS_KEM_ExecutionResult Result,
                            const FSOTS_ExecutionContext& Context,
                            const USOTS_KEM_ExecutionDefinition* Def) const;

private:
    struct FActiveLevelSequenceRun
    {
        FGuid RequestId;
        FGameplayTag ExecutionTag;
        TWeakObjectPtr<const USOTS_KEM_ExecutionDefinition> ExecutionDefinition;
        FSOTS_ExecutionContext ExecContext;
        TWeakObjectPtr<AActor> Instigator;
        TWeakObjectPtr<AActor> Target;
        TSoftObjectPtr<ULevelSequence> SequenceAsset;
        TWeakObjectPtr<ULevelSequencePlayer> Player;
        TWeakObjectPtr<ALevelSequenceActor> SequenceActor;
        TSharedPtr<FStreamableHandle> StreamableHandle;
        bool bDestroySequenceActorOnFinish = true;
    };

    bool IsTelemetryLoggingEnabled() const;
    void ResetPendingTelemetry();
    void PrepareExecutionTelemetryForSelection(const USOTS_KEM_ExecutionDefinition* Def,
                                               const FSOTS_ExecutionContext& Context,
                                               const FString& SourceLabel,
                                               float DistanceToTarget,
                                               float FacingAngleDeg,
                                               float HeightDelta,
                                               float Score,
                                               ASOTS_KEMExecutionAnchor* Anchor,
                                               bool bUsedOmniTrace);
    FSOTS_KEMExecutionTelemetry BuildTelemetryRecord(const USOTS_KEM_ExecutionDefinition* Def,
                                                    const FSOTS_ExecutionContext& Context,
                                                    const FString& SourceLabel,
                                                    float DistanceToTarget,
                                                    float FacingAngleDeg,
                                                    float HeightDelta,
                                                    float Score,
                                                    ASOTS_KEMExecutionAnchor* Anchor,
                                                    bool bUsedOmniTrace) const;

    FSOTS_OnKEMExecutionTelemetry OnExecutionTelemetry;
    FSOTS_KEMExecutionTelemetry PendingExecutionTelemetry;
    bool bHasPendingExecutionTelemetry = false;

    FSOTS_ExecutionContext ActiveExecutionContext;
    TWeakObjectPtr<const USOTS_KEM_ExecutionDefinition> ActiveExecutionDefinition;
    FGameplayTag ActiveExecutionTag;
    FGameplayTag ActiveCameraPrimaryTag;
    FGameplayTag ActiveCameraSecondaryTag;
    TWeakObjectPtr<AActor> ActiveWitnessActor;
    bool bCameraRequested = false;
    bool bLoggedMissingCompletionTagOnce = false;
    bool bLoggedMissingDefinitionsOnce = false;
    bool bLoggedMissingFallbackOnce = false;
    FTimerHandle CompletionTagTimerHandle;

    void ComputeAndInjectPositionTags(AActor* Instigator,
                                      AActor* Target,
                                      FGameplayTagContainer& InOutTags) const;

    void ApplyAnchorEnvContextTags(const ASOTS_KEMExecutionAnchor* Anchor,
                                   FGameplayTagContainer& InOutTags) const;

    float ComputePreferredExecutionBonus(const ASOTS_KEMExecutionAnchor* Anchor,
                                         const USOTS_KEM_ExecutionDefinition* Def) const;

    bool StartLevelSequenceExecution(const USOTS_KEM_ExecutionDefinition* Def, const FSOTS_ExecutionContext& Context);
    void OnLevelSequenceAssetLoaded(FGuid RequestId);
    bool PlayLevelSequenceRun(FGuid RequestId, ULevelSequence* Sequence);
    FGuid FindRequestIdForPlayer(const ULevelSequencePlayer* Player) const;
    void HandleLevelSequenceFinished();
    void HandleLevelSequenceStopped();
    void OnLevelSequenceFinishedInternal(FGuid RequestId, bool bSucceeded);
    void CleanupLevelSequenceRun(FGuid RequestId, bool bSucceeded);
    void StopAllActiveLevelSequences();

    TMap<FGuid, FActiveLevelSequenceRun> ActiveLevelSequenceRuns;

    UFUNCTION()
    void HandleInteractionActionRequested(const FSOTS_InteractionActionRequest& Request);

    TWeakObjectPtr<USOTS_InteractionSubsystem> CachedInteractionSubsystem;
    bool bBoundToInteraction = false;
    bool bLoggedMissingInteractionExecutionTagOnce = false;
};
