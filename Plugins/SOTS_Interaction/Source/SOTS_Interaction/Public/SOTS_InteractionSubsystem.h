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
    void RequestInteraction(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    bool ExecuteInteractionOption(APlayerController* PlayerController, FGameplayTag OptionTag);

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

    void FindBestCandidate(APlayerController* PC, const FVector& ViewLoc, const FRotator& ViewRot, FSOTS_InteractionContext& OutBest, bool& bOutHasBest) const;

    bool MakeContextForActor(APlayerController* PC, APawn* Pawn, AActor* Target, const FVector& ViewLoc, const FRotator& ViewRot, const FHitResult& Hit, FSOTS_InteractionContext& OutContext) const;

    bool PassesLOS(APlayerController* PC, const FVector& ViewLoc, AActor* Target, const FHitResult& CandidateHit) const;

    bool PassesTagGates(APlayerController* PC, const USOTS_InteractableComponent* Interactable, FGameplayTag& OutFailReason) const;

    float ScoreCandidate(const FVector& ViewLoc, const FVector& ViewDir, const FSOTS_InteractionContext& Ctx) const;

    static bool AreSameTarget(const FSOTS_InteractionContext& A, const FSOTS_InteractionContext& B);

    bool ValidateCanInteract(const FSOTS_InteractionContext& Context, FGameplayTag& OutFailReason) const;

    void GatherOptions(const FSOTS_InteractionContext& Context, TArray<FSOTS_InteractionOption>& OutOptions) const;

    bool ExecuteOptionInternal(const FSOTS_InteractionContext& Context, FGameplayTag OptionTag);

    void BroadcastUIIntent(APlayerController* PlayerController, const FSOTS_InteractionUIIntentPayload& Payload);

    UObject* ResolveInteractableImplementer(const FSOTS_InteractionContext& Context) const;
};
