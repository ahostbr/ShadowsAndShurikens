#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "SOTS_ParkourActionData.h"
#include "SOTS_ParkourInterface.h"
#include "SOTS_ParkourStatsWidgetInterface.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UCapsuleComponent;
class USceneComponent;
class USOTS_ParkourConfig;
class USOTS_ParkourActionSet;
class USpringArmComponent;
class UCurveFloat;
class UDataTable;
class UUserWidget;
class UAnimMontage;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTSParkourStateChangedSignature, ESOTS_ParkourState, PreviousState, ESOTS_ParkourState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTSParkourActionStartedSignature, ESOTS_ParkourAction, ActionType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTSParkourActionFinishedSignature, ESOTS_ParkourAction, ActionType, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSOTSParkourMontageRequestSignature, UAnimMontage*, Montage, float, PlayRate, float, StartingPosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTSParkourIKToggleSignature, FName, HandName, bool, bEnable);

USTRUCT()
struct FSOTSParkourTraceStats
{
	GENERATED_BODY()

	UPROPERTY()
	int32 TracesThisFrame = 0;

	UPROPERTY()
	int32 TracesThisAction = 0;
};

/**
 * USOTS_ParkourComponent
 *
 * Lightweight C++ port of the CGF ParkourComp Blueprint. This component owns
 * the local parkour state for a character and provides:
 *
 * - A detection step that probes for ledges/walls in front of the character.
 * - A classification step that turns hits into "mantle", "drop", etc.
 * - An execution step that drives the character movement/warp for the action.
 *
 * The long-term goal is 1:1 parity with the original Blueprint graphs, with
 * this component acting as the central runtime for parkour behavior.
 */
UCLASS(ClassGroup = (SOTS), meta = (BlueprintSpawnableComponent))
class SOTS_PARKOUR_API USOTS_ParkourComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USOTS_ParkourComponent();

	// ~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// ~UActorComponent interface

	/** Enable or disable verbose debug drawing/logging for this component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Debug")
	bool bEnableDebugLogging = false;

	/** Additional trace logging (start/end/hit) for parity audits; off by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Debug")
	bool bEnableTraceDebug = false;

	/** When true, bypasses gates and forces a ledge trace each detect pass (debug only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Debug")
	bool bDebugForceLedgeTrace = false;

	/** Duration (seconds) for generic trace debug draw helpers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Debug")
	float TraceDebugDrawTime = 2.0f; // CGF parity: per-trace draw duration

	/** Enable MotionWarping when a component is available; falls back to teleport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Execution")
	bool bUseMotionWarping = true;

	/** Warp target name consumed by montages/nodes (e.g., Default or numbered). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Execution")
	FName MotionWarpTargetName = TEXT("ParkourTarget");

	/** Collision channel used for all parkour traces (capsule, sphere, line). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	TEnumAsByte<ECollisionChannel> ParkourTraceChannel = ECollisionChannel::ECC_Visibility;

	/** When true, runs parkour detection every tick (still respects the per-frame trace budget). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	bool bContinuousTraceMode = false;

	/** Maximum number of parkour traces allowed per frame (<=0 disables budgeting). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	int32 MaxTracesPerFrame = 3;

	/** Minimum height delta that will be treated as a vault instead of a mantle (mirrors CGF low obstacle vault). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float VaultMinHeight = 15.0f;

	/** Maximum height delta for a vault before we consider mantle (mirrors CGF low obstacle vault). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float VaultMaxHeight = 55.0f;

	/** Forward offset for the secondary/confirm traces that look back into the wall (negative = into wall). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float SecondTraceForwardOffset = -40.0f;

	/** Vertical offset for secondary/confirm traces (negative drops below hit point). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float SecondTraceVerticalOffset = -80.0f;

	/** Minimum 2D speed required before running parkour detection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|LOD")
	float MinSpeedForDetection = 5.0f;

	/** Maximum camera distance where parkour detection remains active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|LOD")
	float MaxCameraDistanceForDetection = 3000.0f;
    
	/** Height delta between the authored CGF pawn and this character (applied to IK and trace starts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Character")
	float CharacterHeightDiff = 17.0f;
    
	/** Additional height adjustment used by warp/retarget helpers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Character")
	float CharacterHeightAdjustment = -15.0f;

	/** Randomized cooldown (seconds) applied while in climb to reduce trace load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|LOD")
	float ClimbTraceCooldownMin = 0.2f;

	/** Randomized cooldown (seconds) applied while in climb to reduce trace load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|LOD")
	float ClimbTraceCooldownMax = 0.35f;

	/** Horizontal sweep distance used for ledge-move and hop probes (CGF uses ~140 units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float LedgeMoveHorizontalDistance = 140.0f;

	/** CGF HorizontalWallDetectTraceHalfQuantity for NotBusy state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	int32 HorizontalWallTraceHalfQuantity_NotBusy = 2;

	/** CGF HorizontalWallDetectTraceHalfQuantity for Climb state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	int32 HorizontalWallTraceHalfQuantity_Climb = 1;

	/** CGF HorizontalWallDetectTraceRange (units per lateral step). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HorizontalWallTraceRange = 20.0f;

	/** CGF VerticalWallDetectTraceQuantity for NotBusy state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	int32 VerticalWallTraceQuantity_NotBusy = 30;

	/** CGF VerticalWallDetectTraceQuantity for Climb state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	int32 VerticalWallTraceQuantity_Climb = 4;

	/** CGF VerticalWallDetectTraceRange (units per vertical step). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float VerticalWallTraceRange = 20.0f;

	/** Optional overlap-only wall grid probes (CGF parity toggle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	bool bUseWallGridOverlap = false;

	/** Distance-from-ledge XY cutoff used for DistanceMantle-like branching (CGF uses 122uu). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float DistanceFromLedgeXYCutoff = 122.0f;

	/** Upwards offset applied when firing lateral ledge-move probes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float LedgeMoveVerticalOffset = 25.0f;

	/** Radius for ledge-move/hop sphere sweeps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float LedgeMoveProbeRadius = 5.0f;

	/** Back-hop distance mirrored from CGF hop traces (horizontal component). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopDistance = 140.0f;

	/** Optional closer-range back-hop probe used by the old CGF graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopInnerDistance = 55.0f; // CGF parity: short inner hop scalar

	/** Acceptable deviation (in degrees) from a vertical wall for back-hop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopAngleToleranceDegrees = 8.0f; // CGF parity: 90° ± 8°

	/** Back-hop probe radius (smaller than generic ledge-move probe for CGF parity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopProbeRadius = 2.5f;

	/** Upwards offset from the impact point when starting back-hop traces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopTraceStartZOffset = 7.0f;

	/** Back-hop yaw window upper bound (positive side, control vs actor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopYawMaxDegrees = 120.0f;

	/** Back-hop yaw window lower bound (negative side, control vs actor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopYawMinDegrees = -120.0f;

	/** Deadzone around forward-facing direction excluded from strafe back-hop windows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopYawCenterDeadzoneDegrees = 30.0f;

	/** Require |DeltaYaw| above this to accept forward-input back hop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopForwardYawRejectDegrees = 150.0f;

	/** Minimum right/left input magnitude for strafe-based back hops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopRightInputThreshold = 0.25f;

	/** Minimum forward input magnitude for the straight-back back hop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopForwardInputThreshold = 0.25f;

	/** Capsule radius used to validate back-hop landing clearance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopSurfaceCapsuleRadius = 25.0f;

	/** Capsule half-height used to validate back-hop landing clearance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopSurfaceCapsuleHalfHeight = 82.0f;

	/** Forward offset (away from wall) for the landing clearance capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float BackHopSurfaceForwardOffset = 55.0f;

	/** Whether to require a clearance capsule check before accepting back-hop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	bool bValidateBackHopSurface = true;

	/** Vertical lift used for hop style traces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HopVerticalLift = 25.0f;

	/** Sphere probe radius for per-hand IK ledge checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HandIKProbeRadius = 5.0f;

	/** Horizontal offset from the ledge point used when searching for hand contacts (fallback when side-specific offsets are unset). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HandIKHorizontalOffset = 8.0f;

	/** Left hand horizontal offset from the ledge point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HandIKHorizontalOffsetLeft = 0.0f;

	/** Right hand horizontal offset from the ledge point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HandIKHorizontalOffsetRight = 0.0f;

	/** Vertical lift applied before sweeping down for hand contacts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HandIKVerticalLift = 10.0f;

	/** Downward trace depth for hand contact validation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HandIKTraceDepth = 60.0f;

	/** Minimum normal alignment (dot) to accept a hand surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HandIKNormalDotThreshold = 0.3f;

	/** Interp speed used when blending hand IK targets (climb / ledge case). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK")
	float HandIKInterpSpeed = 0.2f;

	/** Interp speed used when blending hand IK targets while not climbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK")
	float HandIKInterpSpeedNotClimb = 0.2f;

	/** Maximum consecutive hand IK failures before applying fallback offsets (0 disables). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	int32 HandIKMaxConsecutiveFailures = 0;

	/** Vertical offset applied when hand IK repeatedly fails (used with the failure counter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float HandIKFailureOffsetZ = 0.0f;

	/** Max consecutive frames with no hand IK hit before skipping traces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	int32 HandIKMaxConsecutiveMissFrames = 2;

	/** Cooldown frames to skip hand IK after exceeding the miss budget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	int32 HandIKMissCooldownFrames = 1;

	/** Horizontal spacing used when deriving climb hand targets from the climb ledge hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK")
	float ClimbIKHandSpacing = 8.0f;

	/** Forward/back offset from the ledge when placing climb IK targets (negative pushes into wall). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK")
	float ClimbIKForwardOffset = -20.0f;

	/** Vertical offset applied to climb IK targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|IK")
	float ClimbIKUpOffset = 0.0f;

	/** Optional debug draw duration for climb IK placement traces/markers (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Debug")
	float ClimbIKDebugDrawTime = 5.0f;

	/** Collision channel used for foot IK traces (mirrors BP Foot IK trace channel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	TEnumAsByte<ECollisionChannel> FootIKTraceChannel = ECollisionChannel::ECC_Visibility;

	/** Sphere probe radius for per-foot IK checks (default use). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float FootIKProbeRadius = 6.0f;

	/** Corner move variant of the foot IK probe radius (Parkour.Action.CornerMove). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float FootIKCornerMoveProbeRadius = 15.0f;

	/** Forward offset from the foot socket used when sweeping for contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float FootIKForwardOffset = 40.0f;

	/** Right-hand offset from the foot socket (signed by foot side). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float FootIKRightOffset = 7.0f;

	/** Upwards lift applied before sweeping down to find the foot surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float FootIKVerticalLift = 10.0f;

	/** Downward sweep depth when searching for foot placement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	float FootIKTraceDepth = 50.0f;

	/** Optional debug draw duration for hand IK traces (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Debug")
	float HandIKDebugDrawTime = 1.0f;

	/** Optional debug draw duration for foot IK traces (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Debug")
	float FootIKDebugDrawTime = 0.1f;

	/** Optional climb arrow/probe component used for climb-specific traces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Trace")
	TObjectPtr<USceneComponent> ClimbArrowComponent = nullptr;

	/** Debug draw duration for capsule/sweep traces (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Debug")
	float CapsuleDebugDrawTime = 1.0f;

	/** Optional config asset that drives all tunable parkour settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Config")
	TObjectPtr<USOTS_ParkourConfig> ParkourConfig;

	/** Optional action set DataAsset (OutParkour parity) to drive warp target names/timings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Config")
	TObjectPtr<USOTS_ParkourActionSet> ParkourActionSet;

	/** Optional DataTable source for parkour actions (preferred over JSON/DataAsset when provided). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Config")
	TSoftObjectPtr<UDataTable> ParkourActionDefinitionsTable;

	/** Optional stats/debug widget class (must implement ISOTS_ParkourStatsWidgetInterface). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|UI")
	TSoftClassPtr<UUserWidget> ParkourStatsWidgetClass;

	/** Optional camera boom (spring arm) used for BP camera timeline parity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;

	/** Optional camera move curve (mirrors BP ParkourCameraMove timeline). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Camera")
	TSoftObjectPtr<UCurveFloat> CameraMoveCurve;

	/** Default duration for the camera move timeline (seconds, BP curve ends around 0.4s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Camera")
	float CameraTimelineDuration = 0.4f;

	/** Desired boom arm length used as the target for the camera timeline (falls back to current). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Camera")
	float CameraTargetArmLength = 0.0f;

	/** Blueprint-consumable parkour lifecycle events (CGF parity). */
	UPROPERTY(BlueprintAssignable, Category = "SOTS|Parkour|Events")
	FSOTSParkourStateChangedSignature OnParkourStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "SOTS|Parkour|Events")
	FSOTSParkourActionStartedSignature OnParkourActionStarted;

	UPROPERTY(BlueprintAssignable, Category = "SOTS|Parkour|Events")
	FSOTSParkourActionFinishedSignature OnParkourActionFinished;

	UPROPERTY(BlueprintAssignable, Category = "SOTS|Parkour|Bridge")
	FSOTSParkourMontageRequestSignature OnRequestPlayMontage;

	UPROPERTY(BlueprintAssignable, Category = "SOTS|Parkour|Bridge")
	FSOTSParkourIKToggleSignature OnToggleLeftClimbIK;

	UPROPERTY(BlueprintAssignable, Category = "SOTS|Parkour|Bridge")
	FSOTSParkourIKToggleSignature OnToggleRightClimbIK;

	/** Authoring half-height used by OutParkour data for height adjustments (UE mannequin ~88). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Warp")
	float MotionWarpAuthoringHalfHeight = 88.0f;

	/** CGF parity: permission gates for auto/manual climb requests. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|State")
	bool bCanAutoClimb = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|State")
	bool bCanManualClimb = true;

	/** Current high-level parkour state (Idle / Entering / Active / Exiting). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ParkourState, Category = "SOTS|Parkour|State", meta = (AllowPrivateAccess = "true"))
	ESOTS_ParkourState ParkourState = ESOTS_ParkourState::Idle;

	/** Logical parkour state mirrored to Parkour.State.* gameplay tags. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_LogicalState, Category = "SOTS|Parkour|State", meta = (AllowPrivateAccess = "true"))
	ESOTS_ParkourLogicalState ParkourLogicalState = ESOTS_ParkourLogicalState::NotBusy;

	/** Last computed parkour result from the most recent detection step. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_LastResult, Category = "SOTS|Parkour|Runtime")
	FSOTS_ParkourResult LastResult;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Public Blueprint API
	/**
	 * Entry point for "try to perform parkour now" from input or higher-level
	 * gameplay logic. This will:
	 *
	 *  - Re-run detection using the current character location/velocity.
	 *  - If a valid opportunity is found, store it in LastResult.
	 *  - Execute the resulting action (e.g., mantle / drop) if allowed.
	 */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour")
	void TryPerformParkour();

	/**
	 * Simple helper that just runs the detection step and returns the result,
	 * without executing anything. Useful for debugging or prediction.
	 */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour")
	bool TryDetectParkourOnce(FSOTS_ParkourResult& OutResult);

	/** Abort the current parkour attempt and return to an idle/not-busy state. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|State")
	void AbortParkour(const FString& Reason = TEXT("Abort"));

	/** BP parity: trigger the auto-climb fallback jump (mirrors Auto Climb graph). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|State")
	bool AutoClimb(const FString& Reason = TEXT("AutoClimb"));

	/** BP parity: Check Back Hop graph. Evaluates yaw/input gates for back-hop eligibility. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	bool CheckBackHop(const FVector& InInputDirection, float DeltaYawDegrees) const;

	/** BP parity: Check Back Hop Surface graph. Uses BackHop surface capsule to validate clearance. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	bool CheckBackHopSurface(const FVector& WarpPoint, const FVector& Forward) const;

	/** BP parity: Check Mantle Surface graph (reuses confirm trace overlap). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug")
	bool CheckMantleSurface(const FHitResult& FirstHit, float CapsuleHalfHeight, float HeightDelta, FSOTS_ParkourResult& OutResult) const;

	/** BP parity: Check Vault Surface graph (reuses confirm trace overlap). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug")
	bool CheckVaultSurface(const FHitResult& FirstHit, float CapsuleHalfHeight, float HeightDelta, FSOTS_ParkourResult& OutResult) const;

	/** BP parity: Predict Jump graph. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug")
	bool CheckPredictJump(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& OutResult) const;

	/** BP parity: Predict Jump Surface graph. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug")
	bool CheckPredictJumpSurface(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& OutResult) const;

	/** BP parity: Check ClimbStyle graph. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	ESOTS_ClimbStyle CheckClimbStyle() const { return LastResult.ClimbStyle; }

	/** BP parity: Check Climb Surface graph. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	bool CheckClimbSurface() const { return LastResult.bHasClimbLedgeResult; }

	/** BP parity: Check Parkour Condition graph. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	bool CheckParkourCondition() const { return CanAttemptParkourInternal(); }

	/** BP parity: CheckAirHang graph. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	bool CheckAirHang() const { return LastResult.Action == ESOTS_ParkourAction::AirHang; }

	/** BP parity: CheckTicTac graph. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	bool CheckTicTac() const { return LastResult.Action == ESOTS_ParkourAction::TicTac; }

	/** BP parity: Check Corner Hop graph. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	bool CheckCornerHop() const { return LastResult.Action == ESOTS_ParkourAction::BackHop || LastResult.Action == ESOTS_ParkourAction::LedgeMove; }

	/** BP parity: Check Out Corner graph. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	bool CheckOutCorner() const { return ParkourLogicalState == ESOTS_ParkourLogicalState::CornerMove; }

	/** BP parity: Get Climb Forward Value (input along actor forward). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	float GetClimbForwardValue() const;

	/** BP parity: Get Climb Right Value (input along actor right). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	float GetClimbRightValue() const;

	/** BP parity: Get Horizontal Axis (input X). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	float GetHorizontalAxis() const;

	/** BP parity: Get Vertical Axis (input Z). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	float GetVerticalAxis() const;

	/** BP parity: Get Parkour Action (enum). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	ESOTS_ParkourAction GetParkourAction() const { return LastResult.Action; }

	/** BP parity: Is Parkour Action Equal To Any. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	bool IsParkourActionEqualToAny(const TArray<ESOTS_ParkourAction>& Actions) const;

	/** BP parity: Is Parkour State Equal To Any. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	bool IsParkourStateEqualToAny(const TArray<ESOTS_ParkourState>& States) const;

	/** BP parity: Get Delta Second With Time Dilation. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	float GetDeltaSecondWithTimeDilation() const;

	/** BP parity: Get Warp Rotation (facing into surface). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	FRotator GetWarpRotation() const;


	/** BP parity: Get Climb Style enum. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	ESOTS_ClimbStyle GetClimbStyleEnum() const { return LastResult.ClimbStyle; }

	/** BP parity: Reset Movement (alias to ResetMovementInput). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|State")
	void ResetMovement() { ResetMovementInput(); }


	/** BP parity: Is Ledge Valid. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	bool IsLedgeValid() const;

	/** BP parity: Get Hop Direction (input dir normalized). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	FVector GetHopDirection() const;

	/** Reset transient movement inputs and climb direction (BP Reset Movement parity). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|State")
	void ResetMovementInput();

	/** Reset the cached parkour result (BP Reset Parkour Result parity). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|State")
	void ResetParkourResultState();

	// Debug/QA Blueprint API
	/**
	 * Run detection while bypassing movement/camera gates. Intended for debug tools/console only.
	 */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug")
	bool DebugRunDetectionBypass(FSOTS_ParkourResult& OutResult);

	/** Current high-level parkour state as a gameplay tag (Parkour.State.*). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Tags")
	FGameplayTag GetCurrentParkourStateTag() const;

	/** Current action as a gameplay tag (Parkour.Action.*). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Tags")
	FGameplayTag GetCurrentParkourActionTag() const;

	/** Current climb style as a gameplay tag (Parkour.ClimbStyle.*). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Tags")
	FGameplayTag GetCurrentClimbStyleTag() const;

	/** Current direction as a gameplay tag (Parkour.Direction.*). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Tags")
	FGameplayTag GetCurrentDirectionTag() const;

	/** Debug: trace counts for the current frame. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	int32 GetTracesThisFrame() const { return TraceStats.TracesThisFrame; }

	/** Debug: max traces seen in a single frame since BeginPlay. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Debug")
	int32 GetMaxTracesInSingleFrame() const { return MaxTracesInSingleFrame; }

	/** Read the current parkour state (used by SOTS_ParkourStateLibrary). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	ESOTS_ParkourState GetParkourState() const;

	// Input bridging (Blueprint)
	// Input setters to allow external (BP) systems to drive locomotion data without hard AnimBP coupling.
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Input")
	void SetIsGrounded(bool bInIsGrounded);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Input")
	void SetIsInAir(bool bInIsInAir);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Input")
	void SetMovementSpeed2D(float InSpeed2D);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Input")
	void SetDesiredInputDirection(const FVector& InInputDirection);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Input")
	void SetIsSprinting(bool bInIsSprinting);

	/**
	 * Blueprint-friendly accessor for the last detection/classification result.
	 * This is the canonical way for other systems (BridgeLibrary, DebugLibrary,
	 * UI, etc.) to inspect what the parkour component most recently decided.
	 */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	FSOTS_ParkourResult GetLastResult() const { return LastResult; }

	/** Base IK targets exposed for AnimBPs that prefer pull-model bindings. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	FTransform GetLeftHandBaseTransform() const;

	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	FTransform GetRightHandBaseTransform() const;

	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	FVector GetLeftFootLocation() const;

	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	FVector GetRightFootLocation() const;

	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	bool IsLeftClimbIKEnabled() const { return LastResult.bEnableLeftClimbIK; }

	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	bool IsRightClimbIKEnabled() const { return LastResult.bEnableRightClimbIK; }

	/** Toggle Parkour trace/log debug flags for quick in-editor validation. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug")
	void SetParkourDebugEnabled(bool bEnabled);

	/** CGF parity helper: logs active config values so QA can validate live tuning. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug")
	void DebugPrintConfig() const;

	/** Convenience accessors for gameplay tags consumed by animation. */
	FGameplayTag GetParkourStateTag() const;
	FGameplayTag GetParkourActionTag() const;
	FGameplayTag GetClimbStyleTag() const;
	FGameplayTag GetDirectionTag() const { return LastResult.DirectionTag; }

	/**
	 * Explicitly set the parkour state.
	 * Normally only internal logic should call this, but it is exposed in case
	 * BP needs to drive forced transitions (cutscenes, scripted behavior).
	 */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|State")
	void SetParkourState(ESOTS_ParkourState NewState);

	/** Whether the component is currently performing continuous ledge shimmy movement. */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|State")
	bool IsOnLedgeShimmy() const { return bIsLedgeShimmyActive; }

	UFUNCTION()
	void OnRep_ParkourState();

	UFUNCTION()
	void OnRep_LogicalState();

	UFUNCTION()
	void OnRep_LastResult();

	UFUNCTION()
	void OnOwnerLanded(const FHitResult& Hit);

private:
	/** Find an AnimInstance that implements the parkour ABP interface. */
	UObject* GetParkourAnimInstanceObject() const;

	void PushParkourDataToAnimInstance(const FSOTS_ParkourResult& Result);

	void PushParkourStateToAnimInstance(ESOTS_ParkourState NewState);

protected:
	/** Cached owning character (if any). */
	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> CachedOwnerCharacter;

	/** Cached movement component for the owning character (if any). */
	UPROPERTY(Transient)
	UCharacterMovementComponent* CachedMovementComponent = nullptr;

	/** Simple internal helper: can the owner currently attempt parkour at all? */
	bool CanAttemptParkourInternal() const;
	bool TryConsumeTraceSlot() const;

	/** Look up the per-action trace profile (CGF parity) from the config. */
	const FSOTS_ParkourTraceProfile* FindTraceProfileForAction(const FGameplayTag& ActionTag) const;

	/** CGF ParkourComp "First Capsule Trace" (NotBusy) math. */
	bool BuildGroundForwardTrace(
		const FSOTS_ParkourTraceProfile& Profile,
		const FVector& InInputDirection,
		const FVector& ActorLocation,
		const FVector& Velocity,
		bool bIsFalling,
		FSOTS_ParkourCapsuleTraceSettings& OutSettings) const;

	/** CGF ParkourComp climb/ReachLedge forward trace math. */
	bool BuildClimbForwardTrace(
		const FSOTS_ParkourTraceProfile& Profile,
		FSOTS_ParkourCapsuleTraceSettings& OutSettings) const;

	void BuildMantleTrace(
		const FSOTS_ParkourTraceProfile& Profile,
		const FVector& WarpTopPoint,
		float CapsuleHalfHeight,
		FSOTS_ParkourCapsuleTraceSettings& OutSettings) const;

	void BuildVaultTrace(
		const FSOTS_ParkourTraceProfile& Profile,
		const FVector& WarpTopPoint,
		float CapsuleHalfHeight,
		FSOTS_ParkourCapsuleTraceSettings& OutSettings) const;

	/** BP parity helper: expose the first capsule trace settings (NotBusy/Climb). */
	UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Trace")
	bool GetFirstCapsuleTraceSettings(FSOTS_ParkourCapsuleTraceSettings& OutSettings) const;

	// First capsule trace settings – CGF parity helpers.
	bool BuildFirstCapsuleTrace_NotBusy(FSOTS_ParkourCapsuleTraceSettings& OutSettings) const;
	bool BuildFirstCapsuleTrace_Climb(FSOTS_ParkourCapsuleTraceSettings& OutSettings) const;
	bool BuildFirstCapsuleTrace(FSOTS_ParkourCapsuleTraceSettings& OutSettings) const;

	// Generic capsule sweep helper shared by all parkour capsule traces.
	bool RunCapsuleTrace(
		const FSOTS_ParkourCapsuleTraceSettings& Settings,
		FHitResult& OutHit,
		ECollisionChannel TraceChannel,
		const FName& DebugName
	) const;

	/**
	 * Detection – first capsule trace + CGF-style height classification.
	 *
	 * Returns true and fills OutResult if a valid opportunity exists.
	 */
	bool TryDetectSimpleParkourOpportunity(FSOTS_ParkourResult& OutResult) const;

	/**
	 * Execution – simple Mantle/Drop warp using config offsets.
	 * More advanced actions will be added in later SPINE passes.
	 */
	void ExecuteCurrentParkour();
	void ExecuteCornerMove(class ACharacter* Character, class UCapsuleComponent* Capsule, float CapsuleHalfHeight, float CapsuleRadius);

	UFUNCTION()
	void OnCornerMoveFinished();

	/** Internal helper: validate the current config and log any obvious issues. */
	void ValidateCurrentConfig() const;

	/** Compute the vertical wall detect start height using the shared library. */
	float GetVerticalDetectStartHeight() const;

	/** Confirm mantle/vault surface integrity using the dedicated probe helpers. */
	bool ConfirmSurfaceForAction(
		const FHitResult& FirstHit,
		float CapsuleHalfHeight,
		ESOTS_ParkourAction Action,
		FSOTS_ParkourResult& InOutResult) const;

	/** Run drop-hang primary/secondary probes and update the result if valid. */
	bool RunDropHangProbes(
		const FHitResult& FirstHit,
		float CapsuleHalfHeight,
		FSOTS_ParkourResult& InOutResult) const;

	/** Attempt side tic-tac probes as a fallback when mantle/drop are invalid. */
	bool TryTicTacSideProbes(
		const FHitResult& FirstHit,
		float CapsuleHalfHeight,
		FSOTS_ParkourResult& InOutResult) const;

	/** Run the CGF-style wall grid probes to refine ledge height/XY distance. */
	bool RunWallGridProbes(
		const FHitResult& FirstHit,
		float CapsuleHalfHeight,
		FHitResult& OutBestHit,
		float& OutHeightDelta,
		float& OutXYDistance,
		float& OutWallDepth) const;

	/** Optional advanced classification hook (Blueprint overridable) to pick variants/tags. */
	UFUNCTION(BlueprintNativeEvent, Category = "SOTS|Parkour|Classification")
	bool ClassifyAdvancedParkour(const FSOTS_ParkourResult& BasicResult, FSOTS_ParkourResult& OutResult) const;
	virtual bool ClassifyAdvancedParkour_Implementation(const FSOTS_ParkourResult& BasicResult, FSOTS_ParkourResult& OutResult) const;

	/** Placeholder for motion-warp integration once assets/configs are ready. */
	bool ApplyMotionWarp(const FVector& TargetLocation, const FVector& SurfaceNormal) const;

	/** Select warp settings from the action set and write into LastResult. */
	void ApplyWarpSettingsFromAction(ESOTS_ParkourAction Action, FSOTS_ParkourResult& InOutResult);

	/** Anim-facing hooks (Blueprint can forward to ABP/IK interfaces). */
	UFUNCTION(BlueprintImplementableEvent, Category = "SOTS|Parkour|Anim")
	void BP_OnParkourResultUpdated(const FSOTS_ParkourResult& Result) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "SOTS|Parkour|Anim")
	void BP_OnParkourStateChanged(ESOTS_ParkourState NewState) const;

	/** Optional hook to register root-motion modifiers (e.g., SkewWarp) per action/window. */
	UFUNCTION(BlueprintImplementableEvent, Category = "SOTS|Parkour|Anim")
	void BP_RegisterRootMotionModifiers(class UMotionWarpingComponent* MotionWarping, const FSOTS_ParkourActionDefinition& ActionDef, const TArray<FSOTS_ParkourWarpRuntimeTarget>& RuntimeTargets, class UAnimMontage* Montage) const;

	/** Notify C++ interfaces and BP hooks about result changes. */
	void NotifyParkourResultUpdated(const FSOTS_ParkourResult& Result);

	/** Notify C++ interfaces and BP hooks about state changes. */
	void NotifyParkourStateChanged(ESOTS_ParkourState PreviousState, ESOTS_ParkourState NewState);

	/** Blueprint-callable hook from ReachLedgeIK notifies to toggle climb IK. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|IK")
	void LeftClimbIK(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|IK")
	void RightClimbIK(bool bEnable);

	/** Blueprint-callable hook from OutParkour notify to reset state/action. */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|State")
	void HandleOutParkourNotify();

	/** Internal camera timeline driver (BP parity for ParkourCameraMove). */
	void AddCameraTimeline(float TimeOverride = -1.0f);
	void CameraTimelineTick();
	void FinishCameraTimeline();
	void HandleCameraTimelineForStateChange(ESOTS_ParkourLogicalState PreviousState, ESOTS_ParkourLogicalState NewState);

	/** Manually assign a stats widget instance (must implement ISOTS_ParkourStatsWidgetInterface). */
	UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|UI")
	void SetParkourStatsWidget(UUserWidget* Widget);

	bool PlayActionMontage(const FSOTS_ParkourActionDefinition& ActionDef) const;

	/** Evaluate and tick the logical parkour state machine (CGF parity). */
	void UpdateStateMachine(float DeltaTime);

	/** Derive a logical parkour state from the current lifecycle state/result. */
	ESOTS_ParkourLogicalState EvaluateLogicalState() const;

	/** Set the logical state and update gameplay tags. */
	void SetParkourLogicalState(ESOTS_ParkourLogicalState NewLogicalState);

	// Trace builders mirroring CGF probe tree (mantle, vault, tic-tac, ledge move, hops).
	bool BuildSecondCapsuleTrace(const FHitResult& FirstHit, FSOTS_ParkourCapsuleTraceSettings& OutSettings) const;
	bool TryDetectVaultPath(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& InOutResult) const;
	bool TryDetectLedgeMoveAndHops(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& InOutResult) const;
	bool TryDetectCornerMove(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& InOutResult) const;
	bool PredictJumpLanding(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& OutResult) const;
	bool SelectParkourActionFromHit(const FHitResult& Hit, float HeightDelta, float XYDistance, float WallDepth, float Speed2DForClassification, float MaxSpeed, float CharacterHalfHeight, FSOTS_ParkourResult& OutResult) const;
	ESOTS_ParkourLogicalState GetLogicalStateForAction(ESOTS_ParkourAction Action) const;
	bool IsBackHopInputAllowed(const FVector& InputDirection, float DeltaYawDegrees) const;
	bool CheckBackHopSurfaceClear(const FVector& WarpPoint, const FVector& Forward, const ACharacter* Character) const;
	 bool TriggerAutoClimbJump(const FString& Reason) const;
	 float GetControlToActorDeltaYawDegrees(const ACharacter* Character) const;

	void TickLedgeShimmy(float DeltaTime);
	void LogTraceResult(const TCHAR* Label, const FVector& Start, const FVector& End, float Radius, float HalfHeight, const FHitResult& Hit) const;
	void BroadcastParkourActionStarted(ESOTS_ParkourAction Action);
	void BroadcastParkourActionFinished(bool bSuccess);

	// Debug draw helpers (centralized CGF parity visuals)
	void DrawDebugLineIfEnabled(const FVector& Start, const FVector& End, bool bFeatureEnabled, const FColor& Color) const;
	void DrawDebugCapsuleIfEnabled(const FVector& Center, float HalfHeight, float Radius, const FQuat& Rotation, bool bFeatureEnabled, const FColor& Color) const;
	void DrawDebugSphereIfEnabled(const FVector& Center, float Radius, bool bFeatureEnabled, const FColor& Color) const;
	void DrawDebugPointIfEnabled(const FVector& Location, float Size, bool bFeatureEnabled, const FColor& Color) const;

	/** Per-hand ledge probes to feed IK targets (mirrors CGF hand loops). */
	void ComputeHandIKProbes(const FVector& SurfacePoint, const FVector& SurfaceNormal, float CapsuleHalfHeight, FSOTS_ParkourResult& InOutResult) const;

	/** Derive climb hand IK targets from the climb ledge result (no extra traces). */
	void ComputeClimbIKTargets(FSOTS_ParkourResult& InOutResult) const;

	/** Per-foot probes to feed IK targets (mirrors BP Parkour Foot IK). */
	void ComputeFootIKProbes(FSOTS_ParkourResult& InOutResult) const;

	/** Build a warp transform using facing mode, offsets, and optional rotation/height tweaks. */
	FTransform BuildWarpTransform(const FVector& BaseLocation, const FVector& SurfaceNormal, const FVector& Velocity, float XOffset, float ZOffset, ESOTS_ParkourWarpFacingMode FacingMode, bool bReverseRotation = false, bool bAdjustForCharacterHeight = false, float AuthoringHalfHeight = 88.0f) const;

	// Resolved input helpers (prefer BP-provided values, fall back to movement component when unset).
	FVector GetResolvedInputDirection() const;
	float GetResolvedSpeed2D() const;
	bool GetResolvedIsGrounded() const;
	bool GetResolvedIsInAir() const;
	bool GetResolvedIsSprinting() const;
	FName GetResolvedStateName() const;
	FName GetResolvedActionName() const;

private:
	/** Saved camera boom relative location at begin play (BP: First Camera Relative Location). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Camera", meta = (AllowPrivateAccess = "true"))
	FVector FirstCameraRelativeLocation = FVector::ZeroVector;

	/** Runtime camera boom relative target used by the timeline. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Camera", meta = (AllowPrivateAccess = "true"))
	FVector TargetRelativeCameraLocation = FVector::ZeroVector;

	/** Runtime camera move alpha sampled from the curve. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Camera", meta = (AllowPrivateAccess = "true"))
	float CameraCurveAlpha = 0.0f;

	/** Runtime cached difference between desired and current arm length. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Camera", meta = (AllowPrivateAccess = "true"))
	float LocalTargetLengthDiff = 0.0f;

	/** Active timer and interpolation state for the camera timeline. */
	FTimerHandle CameraTimelineHandle;
	bool bCameraTimelineActive = false;
	float CameraTimelineElapsed = 0.0f;
	float CameraTimelineActiveDuration = 0.0f;
	FVector CameraTimelineStartLocation = FVector::ZeroVector;
	float CameraTimelineStartArmLength = 0.0f;

	ESOTS_ParkourState LastBroadcastParkourState = ESOTS_ParkourState::Idle;
	ESOTS_ParkourAction CurrentActionForEvents = ESOTS_ParkourAction::None;

	/** Tag cache to avoid redundant tag add/remove churn. */
	FGameplayTag ActiveStateTag;
	FGameplayTag ActiveActionTag;
	FGameplayTag ActiveClimbStyleTag;
	FGameplayTag ActiveDirectionTag;

	// BP-driven locomotion inputs (optional overrides).
	bool bHasIsGroundedInput = false;
	bool bIsGroundedInput = false;
	bool bHasIsInAirInput = false;
	bool bIsInAirInput = false;
	bool bHasSpeed2DInput = false;
	float MovementSpeed2DInput = 0.0f;
	bool bHasInputDirection = false;
	FVector InputDirection = FVector::ZeroVector;
	bool bHasIsSprintingInput = false;
	bool bIsSprintingInput = false;

	/** Internal flag for continuous ledge shimmy (OnLedgeMovement parity). */
	bool bIsLedgeShimmyActive = false;

	/** Skip the first foot IK sweep right after entering climb to mirror BP First flag. */
	mutable bool bSkipFootIKTraceOnNextCall = false;

	/** Skip a hand IK sweep after repeated misses to mirror BP "no hit for N frames" behavior. */
	mutable bool bSkipHandIKTraceOnNextCall = false;

	/** Cooldown frame counter after we intentionally skip hand IK traces. */
	mutable int32 HandIKCooldownFramesRemaining = 0;

	/** Consecutive hand IK failure counters (left/right). */
	mutable int32 ConsecutiveLeftHandIKFailures = 0;
	mutable int32 ConsecutiveRightHandIKFailures = 0;

	/** In-flight state for corner move interpolation. */
	bool bCornerMoveInProgress = false;
	int32 CornerMoveLatentId = 0;
	FVector CornerMoveTargetLocation = FVector::ZeroVector;
	FRotator CornerMoveTargetRotation = FRotator::ZeroRotator;

protected:
	/** Minimum projected input magnitude required to drive shimmy along the ledge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SOTS|Parkour|Shimmy")
	float LedgeShimmyInputThreshold = 0.1f;

private:
	/** Cached action definition used when registering motion warp modifiers. */
	FSOTS_ParkourActionDefinition LastAppliedActionDefinition;
	bool bHasLastAppliedActionDefinition = false;

	/** CGF parity: cooldown timer to slow climb trace checks. */
	mutable float ClimbTraceCooldownTime = 0.0f;

	mutable bool bHasCachedActionDefinitions = false;
	mutable TArray<FSOTS_ParkourActionDefinition> CachedActionDefinitions;
	mutable TWeakObjectPtr<UUserWidget> StatsWidgetInstance;

	mutable FSOTSParkourTraceStats TraceStats;
	mutable int32 TotalTracesSinceBeginPlay = 0;
	mutable int32 MaxTracesInSingleFrame = 0;
	mutable uint64 LastTraceResetFrame = 0;

	/** Per-frame guards to avoid re-running costly hop/corner or tic-tac/drop-hang probes. */
	mutable bool bTriedLedgeCornerThisFrame = false;
	mutable bool bTriedTicTacDropHangThisFrame = false;

	mutable float ParkourStateLingerTime = 0.0f;

	mutable int32 ConsecutiveLeftFootIKFailures = 0;
	mutable int32 ConsecutiveRightFootIKFailures = 0;

	float GetExitLingerForAction(ESOTS_ParkourAction Action) const;

	bool CacheActionDefinitions() const;
	const FSOTS_ParkourActionDefinition* ResolveActionDefinition(ESOTS_ParkourAction Action) const;
	bool EnsureStatsWidget() const;
	void PushStatsToWidget() const;
	void PerformParkourDetection(const FVector& InputDirection, bool bIsContinuation);
	void ResetMovementInputInternal(bool bUpdateTags);
	void ResetParkourResultInternal(bool bUpdateTags);
	void StopActiveMontage(float BlendOutTime) const;
	void OnOwnerMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	void ResetTraceStatsForFrame() const;
	void MaybeResetTraceStats() const;
	bool CanConsumeTrace(int32 NumToConsume = 1) const;
	void ConsumeTrace(int32 NumToConsume = 1) const;

	const USOTS_ParkourConfig* GetConfig() const;
	void ApplyConfigFromDataAsset();

	void UpdateGameplayTagsFromState(ESOTS_ParkourLogicalState NewState);
	void UpdateGameplayTagsFromResult(const FSOTS_ParkourResult& Result);
	void ApplyTagDelta(const FGameplayTag& OldTag, const FGameplayTag& NewTag) const;
	void FinalizeResultTags(FSOTS_ParkourResult& Result) const;
	FGameplayTag ResolveParkourTagByName(const FName& TagName) const;

	FGameplayTag GetStateTag(ESOTS_ParkourLogicalState State) const;
	FGameplayTag GetActionTag(ESOTS_ParkourAction Action) const;
	FGameplayTag GetClimbStyleTag(ESOTS_ClimbStyle ClimbStyle) const;
};
