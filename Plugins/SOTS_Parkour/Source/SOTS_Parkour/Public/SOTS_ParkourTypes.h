// SOTS_ParkourTypes.h
// Core enums and result/trace structs for SOTS parkour.
//
// SPINE V3_15:
// - Canonical, unified FSOTS_ParkourResult that supports BOTH:
//     * The original v0.2 usage (bHasResult, Action, ClimbStyle, WorldLocation, WorldNormal, Hit)
//     * The newer CGF-style trace-driven path (bIsValid, ResultType, TargetLocation, SurfaceNormal, HeightDelta)
// - ESOTS_ParkourResultType restored for coarse classification.
// - FSOTS_ParkourCapsuleTraceSettings retained for all capsule trace helpers.
//
// This header is now the single source of truth for parkour result/trace
// structs and is expected by all SPINE V3_* passes that touch parkour.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "SOTS_ParkourTypes.generated.h"

/** Data-driven trace profile (per Parkour.Action.*) matching CGF probe numbers. */
USTRUCT(BlueprintType)
struct SOTS_PARKOUR_API FSOTS_ParkourTraceProfile
{
	GENERATED_BODY()

	/** Optional row/tag identifier used for cross-checking against CGF/OutParkour. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FName SourceId;

	/** Core capsule dimensions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float CapsuleRadius = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float CapsuleHalfHeight = 120.0f;

	/** Forward sweep range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float ForwardDistanceMin = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float ForwardDistanceMax = 200.0f;

	/** Start / vertical offsets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float StartZOffset = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float VerticalOffsetUp = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float VerticalOffsetDown = 0.0f;

	/** Ledge window constraints. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float MinLedgeHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float MaxLedgeHeight = 0.0f;
};

/** High-level state of the parkour component. */
UENUM(BlueprintType)
enum class ESOTS_ParkourState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	Entering    UMETA(DisplayName = "Entering"),
	Active      UMETA(DisplayName = "Active"),
	Exiting     UMETA(DisplayName = "Exiting")
};

/** Logical parkour context mirroring CGF Parkour.State.* tags. */
UENUM(BlueprintType)
enum class ESOTS_ParkourLogicalState : uint8
{
	NotBusy    UMETA(DisplayName = "Not Busy"),
	Mantle     UMETA(DisplayName = "Mantle"),
	Vault      UMETA(DisplayName = "Vault"),
	Climb      UMETA(DisplayName = "Climb"),
	ReachLedge UMETA(DisplayName = "Reach Ledge"),
	TicTac     UMETA(DisplayName = "Tic Tac"),
	CornerMove UMETA(DisplayName = "Corner Move"),
	BeamHidden UMETA(DisplayName = "Beam Hidden")
};

/** Style of climbing the player is currently using. */
UENUM(BlueprintType)
enum class ESOTS_ClimbStyle : uint8
{
	None        UMETA(DisplayName = "None"),
	FreeHang    UMETA(DisplayName = "Free Hang"),
	Braced      UMETA(DisplayName = "Braced")
};

/** The type of parkour action selected from the detection/classification step. */
UENUM(BlueprintType)
enum class ESOTS_ParkourAction : uint8
{
	None            UMETA(DisplayName = "None"),
	Mantle          UMETA(DisplayName = "Mantle"),
	LowMantle       UMETA(DisplayName = "Low Mantle"),
	DistanceMantle  UMETA(DisplayName = "Distance Mantle"),
	Vault           UMETA(DisplayName = "Vault"),
	ThinVault       UMETA(DisplayName = "Thin Vault"),
	HighVault       UMETA(DisplayName = "High Vault"),
	VaultToBraced   UMETA(DisplayName = "Vault To Braced"),
	LedgeMove       UMETA(DisplayName = "Ledge Move"),
	Drop            UMETA(DisplayName = "Drop"),
	TicTac          UMETA(DisplayName = "Tic Tac"),
	BackHop         UMETA(DisplayName = "Back Hop"),
	PredictJump     UMETA(DisplayName = "Predictive Jump"),
	CornerMove      UMETA(DisplayName = "Corner Move"),
	AirHang         UMETA(DisplayName = "Air Hang")
};

/**
 * Coarse classification for detection results.
 * This is intentionally simpler than ESOTS_ParkourAction and can be used
 * when the classification step only knows "Mantle vs Drop" (early passes),
 * while ESOTS_ParkourAction can capture the more detailed final action type.
 */
UENUM(BlueprintType)
enum class ESOTS_ParkourResultType : uint8
{
	None    UMETA(DisplayName = "None"),
	Mantle  UMETA(DisplayName = "Mantle"),
	Drop    UMETA(DisplayName = "Drop"),
};

/** Precomputed runtime warp target for MotionWarping registration. */
USTRUCT(BlueprintType)
struct SOTS_PARKOUR_API FSOTS_ParkourWarpRuntimeTarget
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FName TargetName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FTransform TargetTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float StartTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float EndTime = 0.0f;

	/** Motion-warp point provider to mirror OutParkour settings. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FName WarpPointAnimProvider;

	/** Bone used by the provider, if any. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FName WarpPointAnimBoneName;

	/** Whether translation is warped during this window. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bWarpTranslation = true;

	/** Whether Z is ignored while warping translation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bIgnoreZAxis = false;

	/** Whether rotation is warped during this window. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bWarpRotation = true;

	/** Rotation type string (mirrors OutParkour Rotation Type). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FName RotationType;

	/** Whether to flip the facing direction for this window. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bReversedRotation = false;

	/** Whether to adjust Z using capsule height vs. authoring height. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bAdjustForCharacterHeight = false;

	/** Transform type tag captured from the action definition. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FGameplayTag WarpTransformType;
};

/**
 * Unified result container for a single parkour opportunity evaluation.
 *
 * This struct is designed to support BOTH:
 *
 *  - The original SPINE 1–6 result usage from the v0.2 plugin:
 *      - bHasResult
 *      - Action
 *      - ClimbStyle
 *      - WorldLocation
 *      - WorldNormal
 *      - Hit
 *
 *  - The newer capsule-trace–driven detection passes:
 *      - bIsValid
 *      - ResultType
 *      - TargetLocation (raw impact/warp location)
 *      - SurfaceNormal (raw surface normal)
 *      - HeightDelta (ledge Z vs character Z)
 *
 * Keeping all fields here avoids type churn across the SPINE and lets later
 * passes choose which subset is authoritative for their stage.
 */
USTRUCT(BlueprintType)
struct SOTS_PARKOUR_API FSOTS_ParkourResult
{
	GENERATED_BODY()

public:
	/** Whether this result contains a valid parkour opportunity (legacy v0.2 flag). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bHasResult = false;

	/** Whether this result is valid for the newer trace-driven path. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bIsValid = false;

	/** The parkour action chosen by classification (full-fidelity type). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	ESOTS_ParkourAction Action = ESOTS_ParkourAction::None;

	/** The climb style selected for this opportunity (where applicable). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	ESOTS_ClimbStyle ClimbStyle = ESOTS_ClimbStyle::None;

	/**
	 * Coarse result type, typically used by simpler detection code that only
	 * knows "Mantle vs Drop" before we wire in the full CGF classification.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	ESOTS_ParkourResultType ResultType = ESOTS_ParkourResultType::None;

	/** Representative world-space location for this result (e.g. ledge point). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FVector WorldLocation = FVector::ZeroVector;

	/** Representative world-space normal (e.g. wall or ledge normal). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FVector WorldNormal = FVector::UpVector;

	/**
	 * Raw target location coming from the detection trace (e.g. ImpactPoint
	 * or a warp anchor). In many cases this will match WorldLocation but is
	 * kept separate so future passes can distinguish “raw hit” vs “final warp”.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FVector TargetLocation = FVector::ZeroVector;

	/**
	 * Raw surface normal from the detection trace (ImpactNormal). Like
	 * TargetLocation, this may be used for detailed warp calculations while
	 * WorldNormal stays as a more generic “representative” normal.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FVector SurfaceNormal = FVector::UpVector;

	/**
	 * Height delta between the detected ledge/target and the character.
	 * Positive values typically indicate “above feet” (mantle), negative
	 * values indicate “below feet” (drop/down).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float HeightDelta = 0.0f;

	/** Raw hit information used to derive this result (first/primary hit). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FHitResult Hit;

	/** Optional warp targets derived from action data; supports multi-window warps. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	TArray<FSOTS_ParkourWarpRuntimeTarget> WarpTargets;

	/** Optional selected warp target name derived from action data. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FName WarpTargetName = NAME_None;

	/** Optional direction tag (Parkour.Direction.*) chosen during detection/hops. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Tags")
	FGameplayTag DirectionTag;

	/** Logical parkour state as a gameplay tag (Parkour.State.*). */
	UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Tags")
	FGameplayTag StateTag;

	/** Parkour action as a gameplay tag (Parkour.Action.*). */
	UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Tags")
	FGameplayTag ActionTag;

	/** Climb style as a gameplay tag (Parkour.ClimbStyle.*). */
	UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Tags")
	FGameplayTag ClimbStyleTag;

	/** Optional per-hand contact validity for IK (left/right). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bHasLeftHandSurface = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bHasRightHandSurface = false;

	/** Raw hits captured for hand IK anchoring. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FHitResult LeftHandHit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FHitResult RightHandHit;

	/** Base hand targets derived from hits (pre-curve offsets, BP parity forward*3 and Z-9). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FVector LeftHandBaseLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FRotator LeftHandBaseRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FVector RightHandBaseLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FRotator RightHandBaseRotation = FRotator::ZeroRotator;

	/** Optional per-foot contact validity for IK (left/right). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bHasLeftFootSurface = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bHasRightFootSurface = false;

	/** Raw hits captured for foot IK anchoring. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FHitResult LeftFootHit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FHitResult RightFootHit;

	/** Optional climb ledge anchor (e.g., SecondClimb Ledge Result in BP). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bHasClimbLedgeResult = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	FHitResult ClimbLedgeResult;

	/** Optional per-hand climb offsets (e.g., curve-driven) applied in addition to IK settings. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float ClimbLeftHandOffsetZ = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float ClimbRightHandOffsetZ = 0.0f;

	/** Whether climb IK should be enabled for left/right hands. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bEnableLeftClimbIK = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	bool bEnableRightClimbIK = false;

	/** Optional warp start time (montage seconds) from the chosen warp window. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float WarpStartTime = 0.0f;

	/** Optional warp end time (montage seconds) from the chosen warp window. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour")
	float WarpEndTime = 0.0f;

	FSOTS_ParkourResult() = default;

	/** Reset all fields to a clean, invalid state. */
	void Reset()
	{
		bHasResult    = false;
		bIsValid      = false;
		Action        = ESOTS_ParkourAction::None;
		ClimbStyle    = ESOTS_ClimbStyle::None;
		ResultType    = ESOTS_ParkourResultType::None;
		WorldLocation = FVector::ZeroVector;
		WorldNormal   = FVector::UpVector;
		TargetLocation = FVector::ZeroVector;
		SurfaceNormal  = FVector::UpVector;
		HeightDelta    = 0.0f;
		Hit            = FHitResult();
		WarpTargets.Empty();
		WarpTargetName = NAME_None;
		WarpStartTime  = 0.0f;
		WarpEndTime    = 0.0f;
		DirectionTag   = FGameplayTag();
		bHasLeftHandSurface = false;
		bHasRightHandSurface = false;
		LeftHandHit = FHitResult();
		RightHandHit = FHitResult();
		LeftHandBaseLocation = FVector::ZeroVector;
		RightHandBaseLocation = FVector::ZeroVector;
		LeftHandBaseRotation = FRotator::ZeroRotator;
		RightHandBaseRotation = FRotator::ZeroRotator;
		bHasLeftFootSurface = false;
		bHasRightFootSurface = false;
		LeftFootHit = FHitResult();
		RightFootHit = FHitResult();
		bHasClimbLedgeResult = false;
		ClimbLedgeResult = FHitResult();
		ClimbLeftHandOffsetZ = 0.0f;
		ClimbRightHandOffsetZ = 0.0f;
		bEnableLeftClimbIK = false;
		bEnableRightClimbIK = false;
	}
};

/**
 * Shared shape for all capsule traces used by the parkour system.
 * This is the C++ mirror of the “First/Second Capsule Trace Settings”
 * style graphs in CGF, allowing us to pipe fully computed trace data
 * through a single helper function.
 */
USTRUCT(BlueprintType)
struct SOTS_PARKOUR_API FSOTS_ParkourCapsuleTraceSettings
{
	GENERATED_BODY()

public:
	/** World-space start of the capsule sweep. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Trace")
	FVector Start = FVector::ZeroVector;

	/** World-space end of the capsule sweep. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Trace")
	FVector End = FVector::ZeroVector;

	/** Capsule radius used for the sweep. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Trace")
	float Radius = 0.0f;

	/** Capsule half-height used for the sweep. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Trace")
	float HalfHeight = 0.0f;

	void Reset()
	{
		Start      = FVector::ZeroVector;
		End        = FVector::ZeroVector;
		Radius     = 0.0f;
		HalfHeight = 0.0f;
	}
};
