
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "SOTS_GlobalStealthTypes.h"
#include "Data/SOTS_AbilityTypes.h"
#include "SOTS_OmniTraceKEMPresetLibrary.h"
#include "MovieSceneSequencePlayer.h"
#include "LevelSequence.h"
#include "ContextualAnimSceneAsset.h"

#include "SOTS_KEMAuthoringTypes.h"
#include "SOTS_KEM_Types.generated.h"

class UContextualAnimSceneAsset;
class ULevelSequence;

// ExecutionFamily is design language that complements PositionTag without replacing it.
UENUM(BlueprintType)
enum class ESOTS_KEM_BackendType : uint8
{
    CAS             UMETA(DisplayName="CAS"),
    LevelSequence   UMETA(DisplayName="Level Sequence"),
    AIS             UMETA(DisplayName="AIS (Deprecated)", Hidden, DeprecationMessage="AIS backend retired"),
    SpawnActor      UMETA(DisplayName="Spawn Actor")
};

UENUM(BlueprintType)
enum class ESOTS_KEM_HeightMode : uint8
{
    SamePlaneOnly   UMETA(DisplayName="Same Plane Only"),
    VerticalOnly    UMETA(DisplayName="Vertical Only"),
    Any             UMETA(DisplayName="Any")
};

UENUM(BlueprintType)
enum class ESOTS_KEMState : uint8
{
    None            UMETA(DisplayName="None"),
    Ready           UMETA(DisplayName="Ready"),
    Executing       UMETA(DisplayName="Executing"),
    SuccessCooldown UMETA(DisplayName="Success Cooldown"),
    FailureCooldown UMETA(DisplayName="Failure Cooldown")
};

UENUM(BlueprintType)
enum class ESOTS_KEM_ExecutionResult : uint8
{
    Started     UMETA(DisplayName="Started"),
    Succeeded   UMETA(DisplayName="Succeeded"),
    Failed      UMETA(DisplayName="Failed"),
};

UENUM(BlueprintType)
enum class ESOTS_KEM_ExecutionFamily : uint8
{
    Unknown      UMETA(DisplayName="Unknown"),

    GroundRear   UMETA(DisplayName="Ground Rear Basic"),
    GroundFront  UMETA(DisplayName="Ground Front"),
    GroundLeft   UMETA(DisplayName="Ground Left"),
    GroundRight  UMETA(DisplayName="Ground Right"),

    VerticalAbove UMETA(DisplayName="Vertical Above (Drop)"),
    VerticalBelow UMETA(DisplayName="Vertical Below (Pull)"),

    CornerLeft   UMETA(DisplayName="Corner Left"),
    CornerRight  UMETA(DisplayName="Corner Right"),

    // Extend later with weapon/stance variations if needed.
    Special      UMETA(DisplayName="Special / Cinematic")
};

// Shared helper for logging and debug strings without repeating enum plumbing.
inline FString SOTS_KEM_GetExecutionFamilyName(ESOTS_KEM_ExecutionFamily Family)
{
    if (const UEnum* Enum = StaticEnum<ESOTS_KEM_ExecutionFamily>())
    {
        return Enum->GetNameStringByValue(static_cast<int64>(Family));
    }
    return TEXT("Unknown");
}

UENUM(BlueprintType)
enum class ESOTS_KEMRejectReason : uint8
{
    None            UMETA(DisplayName="None"),
    MissingDefinition UMETA(DisplayName="Missing Definition"),
    AbilityRequirementFailed UMETA(DisplayName="Ability Requirement Failed"),
    StealthBlocked  UMETA(DisplayName="Stealth Blocked"),
    MissionTagMismatch UMETA(DisplayName="Mission Tag Mismatch"),
    DistanceOutOfRange UMETA(DisplayName="Distance Out Of Range"),
    AngleOutOfRange UMETA(DisplayName="Angle Out Of Range"),
    HeightModeMismatch UMETA(DisplayName="Height Mode Mismatch"),
    OmniTraceFailed UMETA(DisplayName="OmniTrace Failed"),
    WarpPointMissing UMETA(DisplayName="Warp Point Missing"),
    DataIncomplete  UMETA(DisplayName="Data Incomplete"),
    Other           UMETA(DisplayName="Other")
};

UENUM(BlueprintType)
enum class ESOTS_KEMExecutionOutcome : uint8
{
    Success UMETA(DisplayName="Success"),
    Failed_Gate UMETA(DisplayName="Failed Gate"),
    Failed_Position UMETA(DisplayName="Failed Position"),
    Failed_OmniTrace UMETA(DisplayName="Failed OmniTrace"),
    Failed_AnchorUnavailable UMETA(DisplayName="Failed Anchor Unavailable"),
    Failed_TargetInvalid UMETA(DisplayName="Failed Target Invalid"),
    Failed_StealthTier UMETA(DisplayName="Failed Stealth Tier"),
    Failed_InternalError UMETA(DisplayName="Failed Internal Error")
};

USTRUCT(BlueprintType)
struct FSOTS_ExecutionContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> Instigator;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> Target;

    UPROPERTY(BlueprintReadOnly)
    FGameplayTagContainer ContextTags;

    UPROPERTY(BlueprintReadOnly)
    FVector InstigatorLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector InstigatorForward = FVector::ForwardVector;

    UPROPERTY(BlueprintReadOnly)
    FVector TargetForward = FVector::ForwardVector;

    UPROPERTY(BlueprintReadOnly)
    float HeightDelta = 0.f; // Target.Z - Instigator.Z

    // Global stealth snapshot (threaded in from the global stealth manager).
    UPROPERTY(BlueprintReadOnly)
    float GlobalStealthScore01 = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float ShadowLevel01 = 0.f;

    UPROPERTY(BlueprintReadOnly)
    ESOTS_StealthTier StealthTier = ESOTS_StealthTier::Hidden;
};

USTRUCT(BlueprintType)
struct FSOTS_KEM_ExecutionEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    ESOTS_KEM_ExecutionResult Result = ESOTS_KEM_ExecutionResult::Started;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    TWeakObjectPtr<AActor> Instigator;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    TWeakObjectPtr<AActor> Target;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    FGameplayTag ExecutionTag;

    // Non-persistent pointer to the definition used for this execution.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    TWeakObjectPtr<const USOTS_KEM_ExecutionDefinition> Definition;
};

/**
 * Lightweight per-candidate debug information for the last KEM selection.
 */
USTRUCT(BlueprintType)
struct FSOTS_KEMCandidateDebugRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    FString ExecutionName;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    float Score = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    bool bSelected = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    ESOTS_KEMRejectReason RejectReason = ESOTS_KEMRejectReason::None;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    FString FailureReason;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    float DistanceToTarget = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    float FacingAngleDeg = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    float HeightDelta = 0.f;
};

USTRUCT(BlueprintType)
struct FSOTS_KEMDecisionStep
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    FName StepName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    FString Detail;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    bool bPassed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    float NumericValue = 0.f;
};

USTRUCT(BlueprintType)
struct FSOTS_KEMExecutionTelemetry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    FGameplayTag ExecutionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    FGameplayTag ExecutionFamilyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    FGameplayTag ExecutionPositionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    ESOTS_KEMExecutionOutcome Outcome = ESOTS_KEMExecutionOutcome::Failed_InternalError;

    UPROPERTY()
    TWeakObjectPtr<AActor> Instigator;

    UPROPERTY()
    TWeakObjectPtr<AActor> Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    float DistanceToTarget = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    int32 StealthTierAtRequest = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    bool bDragonControlled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    bool bFromCutscene = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    bool bUsedAnchor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    bool bUsedOmniTrace = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    FString SourceLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|KEM|Telemetry")
    TArray<FSOTS_KEMDecisionStep> DecisionSteps;
};

USTRUCT(BlueprintType)
struct FSOTS_KEMDecisionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    TWeakObjectPtr<AActor> Instigator;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    TWeakObjectPtr<AActor> Target;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    FGameplayTag RequestedTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    FString SourceLabel;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    FGameplayTagContainer ContextTags;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Debug")
    TArray<FSOTS_KEMCandidateDebugRecord> CandidateRecords;
};

/**
 * High-level summary record for a single execution request, used by debug UI.
 */
USTRUCT(BlueprintType)
struct FSOTS_KEMDebugRecord
{
    GENERATED_BODY()

    // Execution tag that was requested / chosen.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    FGameplayTag RequestedTag;

    // Friendly execution name (ExecutionTag or asset name).
    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    FString ExecutionName;

    // Score of the chosen execution at selection time.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    float Score = 0.f;

    // Final result once the execution completes.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    ESOTS_KEM_ExecutionResult Result = ESOTS_KEM_ExecutionResult::Started;

    // Textual phase / state ("Requested", "Executing", "Finished", "Failed").
    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    FString Phase;

    // Optional failure reason if the execution was rejected or failed.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    FString FailureReason;

    // Instigator / target actors involved in the request.
    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    TWeakObjectPtr<AActor> Instigator;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM")
    TWeakObjectPtr<AActor> Target;
};

USTRUCT(BlueprintType)
struct FSOTS_KEMValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Validation")
    bool bIsValid = true;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Validation")
    TArray<FString> Errors;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|KEM|Validation")
    TArray<FString> Warnings;

    void AddError(const FString& InMsg)
    {
        bIsValid = false;
        Errors.Add(InMsg);
    }

    void AddWarning(const FString& InMsg)
    {
        Warnings.Add(InMsg);
    }
};

USTRUCT(BlueprintType)
struct FSOTS_KEM_CASOffsetConfig
{
    GENERATED_BODY()

    // Instigator local offset relative to target, authored at (0,0,0) in CAS
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS")
    FVector InstigatorLocalOffsetFromTarget = FVector(-80.f, 0.f, 0.f);

    // Optional local rotation offset
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS")
    FRotator InstigatorLocalRotationOffset = FRotator::ZeroRotator;

    // Maximum distance the instigator is allowed to warp to reach this offset
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS", meta=(ClampMin="0.0"))
    float MaxWarpDistance = 100.f;
};

USTRUCT(BlueprintType)
struct FSOTS_KEM_CASConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS")
    TSoftObjectPtr<UContextualAnimSceneAsset> Scene;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS")
    FName SectionName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS")
    FName InstigatorRoleName = TEXT("Ninja");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS")
    FName TargetRoleName = TEXT("Victim");

    // Same-plane law tolerance
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS", meta=(ClampMin="0.0"))
    float MaxSamePlaneHeightDelta = 15.f;

    // Minimum vertical delta required for Vertical-only executions. Backward compatible default mirrors MaxSamePlaneHeightDelta.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS", meta=(ClampMin="0.0"))
    float MinVerticalHeightDelta = 15.f;

    // Optional ceiling for vertical checks. 0 = no ceiling.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS", meta=(ClampMin="0.0"))
    float MaxVerticalHeightDelta = 0.f;

    // Distance window
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS", meta=(ClampMin="0.0"))
    float MinDistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS", meta=(ClampMin="0.0"))
    float MaxDistance = 200.f;

    // Facing window
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS", meta=(ClampMin="0.0", ClampMax="180.0"))
    float MaxFacingAngleDegrees = 60.f;

    // Per-execution authored offset / warp tolerance
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CAS")
    FSOTS_KEM_CASOffsetConfig OffsetConfig;
};


UENUM(BlueprintType)
enum class ESOTS_KEM_WarpRef : uint8
{
    Instigator      UMETA(DisplayName="Instigator Actor"),
    Target          UMETA(DisplayName="Target Actor"),
    InstigatorBone  UMETA(DisplayName="Instigator Bone/Socket"),
    TargetBone      UMETA(DisplayName="Target Bone/Socket")
};

USTRUCT(BlueprintType)
struct FSOTS_KEM_WarpPointDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warp")
    FName WarpTargetName = TEXT("KEM_Execution");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warp")
    ESOTS_KEM_WarpRef Reference = ESOTS_KEM_WarpRef::Target;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warp")
    FName BoneOrSocketName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warp")
    FVector LocalOffset = FVector(-80.f, 0.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warp")
    FRotator LocalRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warp", meta=(ClampMin="0.0"))
    float MaxWarpDistance = 150.f;
};

UCLASS(BlueprintType)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_SpawnExecutionData : public UDataAsset
{
    GENERATED_BODY()

public:
    // Montages to play for this spawn-based execution. TargetMontage can be left null
    // for one-sided executions.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Montages")
    TObjectPtr<UAnimMontage> InstigatorMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Montages")
    TObjectPtr<UAnimMontage> TargetMontage = nullptr;

    // Candidate warp point names for the instigator. These should correspond to
    // WarpPoints authored on the owning ExecutionDefinition. OmniTrace or other
    // logic can decide which one is actually used at runtime.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warp")
    TArray<FName> InstigatorWarpPointNames;

    // Candidate warp point names for the target, if required.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warp")
    TArray<FName> TargetWarpPointNames;
};

USTRUCT(BlueprintType)
struct FSOTS_KEM_SpawnActorConfig
{
    GENERATED_BODY()

    /** Helper actor class to spawn for this execution. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnActor")
    TSubclassOf<AActor> HelperClass;

    /** DataAsset describing the spawn-based execution (montages, warp point names, etc). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnActor")
    TSoftObjectPtr<USOTS_SpawnExecutionData> ExecutionData;

    /** If true, KEM will call into OmniTrace to refine the spawn transform / warp targets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnActor|OmniTrace")
    bool bUseOmniTraceForWarp = false;

    /** KEM preset that determines which OmniTrace pattern is used. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnActor|OmniTrace")
    ESOTS_OmniTraceKEMPreset OmniTracePreset = ESOTS_OmniTraceKEMPreset::Unknown;

    /** High-level pattern tag that selects which OmniTrace logic to run (e.g. SOTS.OmniTrace.KEM.Ground.Rear.Single). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SpawnActor|OmniTrace", meta=(Categories="SOTS.OmniTrace"))
    FGameplayTag OmniTracePatternTag;
};

// LevelSequence backend config (kept generic so callers can decide how to play sequences).
USTRUCT(BlueprintType)
struct FSOTS_KEM_LevelSequenceConfig
{
    GENERATED_BODY()

    /** Sequence asset reference to drive cinematic executions. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LevelSequence")
    TSoftObjectPtr<ULevelSequence> SequenceAsset;

    /** Optional binding tag/name for the instigator within the sequence. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LevelSequence")
    FName InstigatorBindingName;

    /** Optional binding tag/name for the target within the sequence. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LevelSequence")
    FName TargetBindingName;

    /** Playback settings forwarded to ULevelSequencePlayer creation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LevelSequence")
    FMovieSceneSequencePlaybackSettings PlaybackSettings;

    /** Destroy the spawned LevelSequenceActor when playback ends. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LevelSequence")
    bool bDestroySequenceActorOnFinish = true;
};

// AIS / ability backend config (legacy placeholder; AIS backend retired).
USTRUCT(BlueprintType)
struct FSOTS_KEM_AISConfig
{
    GENERATED_BODY()

    /** Legacy placeholder for retired AIS backend. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AIS")
    FGameplayTag BehaviorTag;
};


UENUM()
enum class ESOTS_KEM_PositionKind : uint8
{
    Unknown,
    GroundRear,
    GroundFront,
    GroundLeft,
    GroundRight,
    VerticalAbove,
    VerticalBelow,
    CornerLeft,
    CornerRight
};

// Main execution definition DA
UCLASS(BlueprintType)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_ExecutionDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ID")
    FGameplayTag ExecutionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ID")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Authoring")
    FSOTS_KEMAuthoringMetadata Authoring;

    // Tags used for selection
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection")
    FGameplayTagContainer RequiredContextTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection")
    FGameplayTagContainer BlockedContextTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection")
    FGameplayTag ExecutionFamilyTag;

    // PositionTag indicates the canonical relative position of the instigator
    // to the target for this execution (Ground.Rear, Ground.Left, Vertical.Above,
    // etc.). OmniTrace patterns, CAS offsets, and DevTools reports rely on the
    // SOTS.KEM.Position.* taxonomy staying stable.
    // DevTools: KEM execution reports treat PositionTag as the primary key for
    // grouping and validating kill positions.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection")
    FGameplayTag PositionTag;

    // ExecutionFamily groups executions into design archetypes (e.g. GroundRear,
    // VerticalAbove, CornerLeft). This is used for coverage checks and balancing.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Execution|Design")
    ESOTS_KEM_ExecutionFamily ExecutionFamily = ESOTS_KEM_ExecutionFamily::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection")
    TArray<FGameplayTag> AdditionalPositionTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection")
    ESOTS_KEM_HeightMode HeightMode = ESOTS_KEM_HeightMode::SamePlaneOnly;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection", meta=(ClampMin="0.0"))
    float BaseScore = 1.f;

    // Backend type
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backend")
    ESOTS_KEM_BackendType BackendType = ESOTS_KEM_BackendType::CAS;

    // Optional stealth gating: maximum allowed global stealth score for this
    // execution to be considered. 1.0 = no restriction.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection|Stealth", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaxGlobalStealthScore01 = 1.0f;

    // Optional minimum shadow level required for this execution to be preferred.
    // 0.0 = no preference. Higher values bias selection towards deeper shadows.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection|Stealth", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MinShadowLevel01 = 0.0f;

    // Optional ability requirements. When enabled, KEM will consult the GAS
    // ability requirement system before treating this execution as a valid
    // candidate for the current context.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Ability")
    FGameplayTag AbilityRequirementTag;

    // Optional inline ability requirements. If authored, these are evaluated
    // directly instead of (or in addition to) looking up a library entry.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Ability")
    FSOTS_AbilityRequirements InlineAbilityRequirements;

    // If true, ability requirements are evaluated for this execution; if false,
    // the fields above are ignored and KEM behaves exactly as before.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Ability")
    bool bUseAbilityRequirements = false;

    // Optional authored warp points
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warp")
    TArray<FSOTS_KEM_WarpPointDef> WarpPoints;

    // Optional FX tags that can be fired when this execution transitions
    // through its lifecycle. These are purely descriptive; FX mapping is
    // handled via the global FX manager + definition libraries.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|FX")
    FGameplayTag FXTag_OnExecutionStarted;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|FX")
    FGameplayTag FXTag_OnExecutionSucceeded;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|FX")
    FGameplayTag FXTag_OnExecutionFailed;

    // SpawnActor backend config (used when BackendType == SpawnActor)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backend|SpawnActor")
    FSOTS_KEM_SpawnActorConfig SpawnActorConfig;

    // LevelSequence backend config (used when BackendType == LevelSequence)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backend|LevelSequence")
    FSOTS_KEM_LevelSequenceConfig LevelSequenceConfig;

    // AIS backend config (used when BackendType == AIS)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backend|AIS")
    FSOTS_KEM_AISConfig AISConfig;

    // CAS backend config (used when BackendType == CAS)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backend|CAS")
    FSOTS_KEM_CASConfig CASConfig;

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Validation")
    FSOTS_KEMValidationResult ValidateDefinition() const;

    ESOTS_KEM_PositionKind GetPositionKind() const;
};

// Internal CAS evaluation result (C++ only)

// Registry entry used by the central ExecutionDefinition registry config
USTRUCT(BlueprintType)
struct FSOTS_KEM_ExecutionRegistryEntry
{
    GENERATED_BODY()

    // Optional explicit ID. If left None, the ExecutionDefinition's ExecutionTag name will be used.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    FName ExecutionId = NAME_None;

    // The execution definition to register. Soft so it can live in Content and be cooked normally.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    TSoftObjectPtr<USOTS_KEM_ExecutionDefinition> ExecutionDefinition;
};

// DataAsset that holds the list of all execution definitions for this project.
// In SOTS this is intended to be authored as a single asset like:
//   'DA_KEMExecutionDefinitionRegistryConfig'
UCLASS(BlueprintType)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_ExecutionRegistryConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    // List of execution definitions to register at startup.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="KEM")
    TArray<FSOTS_KEM_ExecutionRegistryEntry> Entries;
};

USTRUCT()
struct FSOTS_CASQueryResult
{
    GENERATED_BODY()

    bool bIsValid = false;

    UContextualAnimSceneAsset* Scene = nullptr;
    FName SectionName = NAME_None;
    FName InstigatorRole = NAME_None;
    FName TargetRole = NAME_None;

    FTransform InstigatorEntryTransform;
    FTransform TargetEntryTransform;
    FTransform InstigatorWarpTarget;
};


USTRUCT(BlueprintType)
struct FSOTS_KEM_WarpRuntimeTarget
{
    GENERATED_BODY()

    /** Semantic name for this target ("Rear", "Left", etc.). */
    UPROPERTY(BlueprintReadOnly, Category="Warp")
    FName TargetName = NAME_None;

    /** World-space transform the instigator should warp towards. */
    UPROPERTY(BlueprintReadOnly, Category="Warp")
    FTransform TargetTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct FSOTS_KEM_OmniTraceWarpResult
{
    GENERATED_BODY()

    /** Whether a helper spawn transform was computed. */
    UPROPERTY(BlueprintReadOnly, Category="Warp")
    bool bHasHelperSpawn = false;

    /** Suggested helper spawn transform (usually aligned behind the target). */
    UPROPERTY(BlueprintReadOnly, Category="Warp")
    FTransform HelperSpawnTransform = FTransform::Identity;

    /** Per-role warp targets (currently typically one entry named "Rear"). */
    UPROPERTY(BlueprintReadOnly, Category="Warp")
    TArray<FSOTS_KEM_WarpRuntimeTarget> WarpTargets;
};
