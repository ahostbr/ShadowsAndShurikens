#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_InteractionTypes.h"
#include "SOTS_InteractionUIIntentPayload.h"
#include "SOTS_InteractionSubsystem.generated.h"

class USOTS_InteractableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_OnInteractionCandidateChanged, APlayerController*, PlayerController, const FSOTS_InteractionContext&, NewCandidate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FSOTS_OnInteractionUIIntentPayload,
    APlayerController*, PlayerController,
    const FSOTS_InteractionUIIntentPayload&, Payload
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FSOTS_OnInteractionUIIntent,
    APlayerController*, PlayerController,
    FGameplayTag, IntentTag,
    const FSOTS_InteractionContext&, Context,
    const TArray<FSOTS_InteractionOption>&, Options
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnInteractionExecuted, const FSOTS_InteractionExecuteReport&, Report);

USTRUCT()
struct FSOTS_InteractionCandidateState
{
    GENERATED_BODY()

    /** Current chosen candidate context */
    UPROPERTY()
    FSOTS_InteractionContext Current;

    /** Whether we currently have a valid candidate */
    UPROPERTY()
    bool bHasCandidate = false;

    /** Throttle timestamp (world seconds) */
    UPROPERTY()
    double NextAllowedUpdateTimeSeconds = 0.0;

    /** Last evaluated view for throttle bypass checks */
    UPROPERTY()
    FVector LastViewLocation = FVector::ZeroVector;

    UPROPERTY()
    FRotator LastViewRotation = FRotator::ZeroRotator;

    /** Diagnostic tracking of why no candidate was chosen */
    UPROPERTY()
    ESOTS_InteractionNoCandidateReason LastNoCandidateReason = ESOTS_InteractionNoCandidateReason::None;

    /** Whether the last query used OmniTrace */
    UPROPERTY()
    bool bLastUsedOmniTrace = false;
};

/**
 * Global interaction brain: finds and tracks the best interactable candidate per player.
 * No UI calls. UI routing happens in later pass.
 */
UCLASS()
class USOTS_InteractionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    USOTS_InteractionSubsystem();

    // --- Tunables (defaults are safe; can be moved to config later) ---
    /** How often UpdateCandidateThrottled is allowed to run for a given player. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    float UpdateIntervalSeconds;

    /** Sphere sweep radius for candidate search. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    float SearchRadius;

    /** Maximum sweep distance forward from viewpoint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    float SearchDistance;

    /** Weight for distance score. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    float DistanceScoreWeight;

    /** Weight for facing score (dot product). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    float FacingScoreWeight;

    /** Debug (dev only later). Keep as data only for now. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bEnableDebug;

    /** Force use of legacy engine traces instead of OmniTrace (default false). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bForceLegacyTraces;

    /** Allow bypassing throttle when view moves enough between frames. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    float ThrottleBypassViewDotThreshold;

    /** Allow bypassing throttle when camera teleports significantly. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    float ThrottleBypassLocationThreshold;

    /** Dev-only verbose scoring log toggle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bLogScoreDebug;

    /** Dev-only trace debug draw. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bDebugDrawInteractionTraces;

    /** Dev-only trace hit logging. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bDebugLogInteractionTraceHits;

    /** Dev-only execution failure logs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bDebugLogInteractionExecutionFailures;

    /** Dev-only warning when UI intents have no listeners. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bWarnOnUnboundInteractionIntents;

    /** Dev-only warning when legacy UI intent delegate is used. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bWarnOnLegacyIntentUsage;

    /** Dev-only payload logging for UI intents. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bDebugLogIntentPayloads;

    /** Cooldown for repeated intent warnings (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    float WarnIntentCooldownSeconds;

    /** Dev-only warning when no UI bindings are present. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bWarnOnUnboundUIIntent;

    // --- Events ---
    /** Fired whenever the best candidate changes (including clearing to none). */
    UPROPERTY(BlueprintAssignable, Category="SOTS|Interaction")
    FSOTS_OnInteractionCandidateChanged OnCandidateChanged;

    /** UI intent router: emit prompt/options/fail intents to SOTS_UI listeners. */
    UPROPERTY(BlueprintAssignable, Category="SOTS|Interaction|UI")
    FSOTS_OnInteractionUIIntentPayload OnUIIntentPayload;

    /** DEPRECATED: legacy multicast with split params. Remove after SOTS_UI glue updates. */
    UPROPERTY(BlueprintAssignable, Category="SOTS|Interaction|UI")
    FSOTS_OnInteractionUIIntent OnUIIntent;

    /** Fired once per interaction attempt (success or failure). */
    UPROPERTY(BlueprintAssignable, Category="SOTS|Interaction")
    FSOTS_OnInteractionExecuted OnInteractionExecuted;

    // --- Public API (call from PlayerController / Pawn BP) ---
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    void UpdateCandidateNow(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    void UpdateCandidateThrottled(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    void ClearCandidate(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    bool GetCurrentCandidate(APlayerController* PlayerController, FSOTS_InteractionContext& OutContext) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    FSOTS_InteractionResult RequestInteraction(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    FSOTS_InteractionResult ExecuteInteractionOption(APlayerController* PlayerController, FGameplayTag OptionTag);

    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    FSOTS_InteractionExecuteReport RequestInteraction_WithResult(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    FSOTS_InteractionExecuteReport ExecuteInteractionOption_WithResult(APlayerController* PlayerController, FGameplayTag OptionTag);

private:
    UPROPERTY()
    TMap<TWeakObjectPtr<APlayerController>, FSOTS_InteractionCandidateState> CandidateStates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI", meta=(AllowPrivateAccess="true"))
    FGameplayTag UIIntent_InteractionPrompt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI", meta=(AllowPrivateAccess="true"))
    FGameplayTag UIIntent_InteractionOptions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI", meta=(AllowPrivateAccess="true"))
    FGameplayTag UIIntent_InteractionFail;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI", meta=(AllowPrivateAccess="true"))
    FGameplayTag UIFailReason_NoCandidate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI", meta=(AllowPrivateAccess="true"))
    FGameplayTag UIFailReason_BlockedByTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|UI", meta=(AllowPrivateAccess="true"))
    FGameplayTag UIFailReason_InterfaceDenied;

    bool BuildViewContext(APlayerController* PC, FVector& OutViewLoc, FRotator& OutViewRot) const;

    void UpdateCandidateInternal(APlayerController* PlayerController, const FVector& ViewLoc, const FRotator& ViewRot);

    void FindBestCandidate(APlayerController* PC, const FVector& ViewLoc, const FRotator& ViewRot, FSOTS_InteractionContext& OutBest, bool& bOutHasBest, ESOTS_InteractionNoCandidateReason& OutNoCandidateReason, bool& bOutUsedOmniTrace) const;

    bool MakeContextForActor(APlayerController* PC, APawn* Pawn, AActor* Target, const FVector& ViewLoc, const FRotator& ViewRot, const FHitResult& Hit, FSOTS_InteractionContext& OutContext) const;

    bool PassesLOS(APlayerController* PC, const FVector& ViewLoc, AActor* Target, const FHitResult& CandidateHit, bool& bOutUsedOmniTrace) const;

    bool PassesTagGates(APlayerController* PC, const USOTS_InteractableComponent* Interactable, FGameplayTag& OutFailReason) const;

    float ScoreCandidate(const FVector& ViewLoc, const FVector& ViewDir, const FSOTS_InteractionContext& Ctx) const;

    static bool AreSameTarget(const FSOTS_InteractionContext& A, const FSOTS_InteractionContext& B);

    bool ValidateCanInteract(const FSOTS_InteractionContext& Context, FGameplayTag& OutFailReason) const;

    void GatherOptions(const FSOTS_InteractionContext& Context, TArray<FSOTS_InteractionOption>& OutOptions) const;

    bool ExecuteOptionInternal(const FSOTS_InteractionContext& Context, FGameplayTag OptionTag);

    FSOTS_InteractionExecuteReport BuildExecuteReport(int32 SequenceId, ESOTS_InteractionExecuteResult Result, const FSOTS_InteractionContext& Context, const FGameplayTag& OptionTag, const FString& DebugReason) const;

    FSOTS_InteractionExecuteReport ExecuteInteractionInternal(const FSOTS_InteractionContext& Context, const FGameplayTag& OptionTag);

    void BroadcastUIIntent(APlayerController* PlayerController, const FSOTS_InteractionUIIntentPayload& Payload);

    UObject* ResolveInteractableImplementer(const FSOTS_InteractionContext& Context) const;

    bool ShouldBypassThrottle(const FSOTS_InteractionCandidateState& State, const FVector& ViewLoc, const FRotator& ViewRot) const;

    void LogCandidateScoreDebug(const FSOTS_InteractionContext& Context, float Score) const;

    UPROPERTY()
    int32 InteractionSequenceCounter;

    UPROPERTY()
    double LastWarnUnboundIntentTime;

    UPROPERTY()
    double LastWarnLegacyIntentTime;
};
