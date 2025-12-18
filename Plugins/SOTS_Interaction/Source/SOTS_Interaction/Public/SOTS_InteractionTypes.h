#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "SOTS_InteractionTypes.generated.h"

class AActor;
class APawn;
class APlayerController;

UENUM(BlueprintType)
enum class ESOTS_InteractionNoCandidateReason : uint8
{
    None            UMETA(DisplayName="None"),
    NoHits          UMETA(DisplayName="No Hits"),
    NotInteractable UMETA(DisplayName="Not Interactable"),
    BlockedByTags   UMETA(DisplayName="Blocked By Tags"),
    OutOfRange      UMETA(DisplayName="Out Of Range"),
    BlockedByLOS    UMETA(DisplayName="Blocked By LOS"),
    MissingInterface UMETA(DisplayName="Missing Interface"),
    Unknown         UMETA(DisplayName="Unknown")
};

UENUM(BlueprintType)
enum class ESOTS_InteractionResultCode : uint8
{
    Success             UMETA(DisplayName="Success"),
    PromptedForOptions  UMETA(DisplayName="Prompted For Options"),
    NoCandidate         UMETA(DisplayName="No Candidate"),
    CandidateInvalid    UMETA(DisplayName="Candidate Invalid"),
    OptionNotFound      UMETA(DisplayName="Option Not Found"),
    NotInRange          UMETA(DisplayName="Not In Range"),
    BlockedByLOS        UMETA(DisplayName="Blocked By LOS"),
    MissingInterface    UMETA(DisplayName="Missing Interface"),
    MissingComponent    UMETA(DisplayName="Missing Component")
};

UENUM(BlueprintType)
enum class ESOTS_InteractionExecuteResult : uint8
{
    Success                     UMETA(DisplayName="Success"),
    NoCandidate                 UMETA(DisplayName="No Candidate"),
    CandidateInvalid            UMETA(DisplayName="Candidate Invalid"),
    CandidateOutOfRange         UMETA(DisplayName="Candidate Out Of Range"),
    CandidateNoLineOfSight      UMETA(DisplayName="Candidate No Line Of Sight"),
    CandidateBlocked            UMETA(DisplayName="Candidate Blocked By Tags"),
    MissingInteractableInterface UMETA(DisplayName="Missing Interactable Interface"),
    MissingInteractableComponent UMETA(DisplayName="Missing Interactable Component"),
    OptionNotFound              UMETA(DisplayName="Option Not Found"),
    OptionNotAvailable          UMETA(DisplayName="Option Not Available"),
    ExecutionRejectedByTarget   UMETA(DisplayName="Execution Rejected By Target"),
    InternalError               UMETA(DisplayName="Internal Error")
};

/**
 * A single selectable interaction option for a target (e.g., "Open", "Loot", "Drag Body").
 */
USTRUCT(BlueprintType)
struct FSOTS_InteractionOption
{
    GENERATED_BODY()

    /** Option identifier (stable tag, drives behavior). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTag OptionTag;

    /** Player-facing label to display in UI (UI is routed elsewhere). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FText DisplayText;

    /** Optional: reason this option is blocked (empty = available). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTag BlockedReasonTag;

    /** Optional: arbitrary extra tags describing this option (e.g. "RequiresKey"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTagContainer MetaTags;

    /** Optional: player must have all of these tags. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tags")
    FGameplayTagContainer RequiredPlayerTags;

    /** Optional: interaction blocked if player has any of these tags. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tags")
    FGameplayTagContainer BlockedPlayerTags;

    /** Optional: target must have all of these tags. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tags")
    FGameplayTagContainer RequiredTargetTags;

    /** Optional: interaction blocked if target has any of these tags. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tags")
    FGameplayTagContainer BlockedTargetTags;

    /** Optional: per-option LOS override. Ignored unless bOverrideRequiresLineOfSight is true. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bRequiresLineOfSight;

    /** When true, bRequiresLineOfSight is respected; otherwise component/global defaults apply. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    bool bOverrideRequiresLineOfSight;

    /** Optional: per-option range override (<= 0 uses component/global defaults). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning", meta=(ClampMin="0.0"))
    float MaxDistanceOverride;

    /** Optional: priority hint for selection/UI sorting (higher = preferred). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Tuning")
    float Priority;

    FSOTS_InteractionOption()
        : OptionTag()
        , DisplayText(FText::GetEmpty())
        , BlockedReasonTag()
        , MetaTags()
        , RequiredPlayerTags()
        , BlockedPlayerTags()
        , RequiredTargetTags()
        , BlockedTargetTags()
        , bRequiresLineOfSight(false)
        , bOverrideRequiresLineOfSight(false)
        , MaxDistanceOverride(0.f)
        , Priority(0.f)
    {}
};

/**
 * The context passed around for interaction validation and execution.
 * This is the canonical payload type; UI can be driven from this.
 */
USTRUCT(BlueprintType)
struct FSOTS_InteractionContext
{
    GENERATED_BODY()

    /** The controller initiating the interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    TWeakObjectPtr<APlayerController> PlayerController;

    /** The pawn initiating the interaction (usually the player's pawn). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    TWeakObjectPtr<APawn> PlayerPawn;

    /** The target actor being interacted with. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    TWeakObjectPtr<AActor> TargetActor;

    /** The primary interaction type tag for this target (e.g., Pickup/Door/Talk). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTag InteractionTypeTag;

    /** Distance from player to target at query time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    float Distance = 0.f;

    /** Trace result used to resolve the candidate (if any). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FHitResult HitResult;

    /** Score assigned during candidate selection (higher is better). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    float Score = 0.f;

    FSOTS_InteractionContext()
        : PlayerController(nullptr)
        , PlayerPawn(nullptr)
        , TargetActor(nullptr)
        , InteractionTypeTag()
        , Distance(0.f)
        , HitResult()
        , Score(0.f)
    {}
};

USTRUCT(BlueprintType)
struct FSOTS_InteractionResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    ESOTS_InteractionResultCode Result = ESOTS_InteractionResultCode::NoCandidate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTag FailReasonTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTag ExecutedOptionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FSOTS_InteractionContext Context;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    bool bUsedOmniTrace = false;
};

USTRUCT(BlueprintType)
struct FSOTS_InteractionExecuteReport
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    ESOTS_InteractionExecuteResult Result = ESOTS_InteractionExecuteResult::NoCandidate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    int32 SequenceId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    TWeakObjectPtr<AActor> Instigator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    TWeakObjectPtr<AActor> Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTag OptionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    float Distance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FString DebugReason;
};

UENUM(BlueprintType)
enum class ESOTS_InteractionTraceShape : uint8
{
    Sphere UMETA(DisplayName="Sphere"),
    Line   UMETA(DisplayName="Line")
};

USTRUCT(BlueprintType)
struct FSOTS_InteractionTraceConfig
{
    GENERATED_BODY()

    /** Max distance for search/LOS traces. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Trace", meta=(ClampMin="0.0"))
    float MaxDistance;

    /** Whether LOS is globally required (component/option may add stricter rules). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Trace")
    bool bRequireLineOfSight;

    /** Trace channel used for search and LOS. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Trace")
    TEnumAsByte<ECollisionChannel> TraceChannel;

    /** Shape used for candidate search. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Trace")
    ESOTS_InteractionTraceShape TraceShape;

    /** Radius used when TraceShape is Sphere. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Trace", meta=(ClampMin="0.0"))
    float SphereRadius;

    /** Optional socket/bone to use as target aim point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Trace")
    FName TargetSocketName;

    FSOTS_InteractionTraceConfig()
        : MaxDistance(400.f)
        , bRequireLineOfSight(true)
        , TraceChannel(ECC_Visibility)
        , TraceShape(ESOTS_InteractionTraceShape::Sphere)
        , SphereRadius(30.f)
        , TargetSocketName(NAME_None)
    {}
};

/** Canonical interaction data package (provider + options + score). */
USTRUCT(BlueprintType)
struct FSOTS_InteractionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FSOTS_InteractionContext Context;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    TArray<FSOTS_InteractionOption> Options;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    bool bFromComponent = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    bool bFromInterface = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    float Score = 0.f;
};
