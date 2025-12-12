#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "SOTS_ParkourTypes.h"
#include "SOTS_ParkourInterface.h"
#include "Anim/SOTS_ExperimentalStateMachineTypes.h"
#include "Animation/AnimNodeReference.h"
#include "Animation/TrajectoryTypes.h"
#include "BlendStack/BlendStackAnimNodeLibrary.h"
#include "BoneControllers/AnimNode_OffsetRootBone.h"
#include "BoneControllers/AnimNode_OrientationWarping.h"
#include "BoneControllers/AnimNode_FootPlacement.h"
#include "PoseSearch/PoseSearchResult.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "SOTS_ParkourAnimInstance.generated.h"

UENUM(BlueprintType)
enum class ESOTS_MovementState : uint8
{
    Idle   UMETA(DisplayName = "Idle"),
    Moving UMETA(DisplayName = "Moving"),
};

UENUM(BlueprintType)
enum class ESOTS_MovementMode : uint8
{
    OnGround UMETA(DisplayName = "OnGround"),
    InAir    UMETA(DisplayName = "InAir"),
};

UENUM(BlueprintType)
enum class ESOTS_MovementDirection : uint8
{
    F  UMETA(DisplayName = "F"),
    B  UMETA(DisplayName = "B"),
    LL UMETA(DisplayName = "LL"),
    LR UMETA(DisplayName = "LR"),
    RL UMETA(DisplayName = "RL"),
    RR UMETA(DisplayName = "RR"),
};

UENUM(BlueprintType)
enum class E_Gait : uint8
{
    Walking   UMETA(DisplayName = "Walking"),
    Jogging   UMETA(DisplayName = "Jogging"),
    Sprinting UMETA(DisplayName = "Sprinting"),
};

UENUM(BlueprintType)
enum class ESOTS_Stance : uint8
{
    Stand   UMETA(DisplayName = "Stand"),
    Crouch  UMETA(DisplayName = "Crouch"),
};

UENUM(BlueprintType)
enum class ESOTS_RotationMode : uint8
{
    OrientToRotation UMETA(DisplayName = "OrientToRotation"),
    Strafe           UMETA(DisplayName = "Strafe"),
};

// Experimental locomotion state machine states (mirrors the BP state enum).
UENUM(BlueprintType)
enum class ESOTS_StateMachineState : uint8
{
    IdleLoop                    UMETA(DisplayName = "IdleLoop"),
    TransitionToIdleLoop        UMETA(DisplayName = "TransitionToIdleLoop"),
    LocomotionLoop              UMETA(DisplayName = "LocomotionLoop"),
    TransitionToLocomotionLoop  UMETA(DisplayName = "TransitionToLocomotionLoop"),
    InAirLoop                   UMETA(DisplayName = "InAirLoop"),
    TransitionToInAirLoop       UMETA(DisplayName = "TransitionToInAirLoop"),
    IdleBreak                   UMETA(DisplayName = "IdleBreak"),
};

class USOTS_ParkourComponent;
class UCharacterMovementComponent;
class ACharacter;
class UChooserTable;
class UPoseSearchDatabase;

/**
 * C++ spine for the parkour AnimBP. Exposes parkour, IK, and locomotion data
 * expected by ABP_SandboxCharacterParkour. Blueprint should consume these
 * read-only fields after reparenting.
 */
UCLASS(Blueprintable, BlueprintType)
class SHADOWSANDSHURIKENS_API USOTS_ParkourAnimInstance : public UAnimInstance, public ISOTS_ParkourABPInterface
{
    GENERATED_BODY()

public:
    USOTS_ParkourAnimInstance();

    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

    // ISOTS_ParkourABPInterface
    virtual void OnParkourResultUpdated_Implementation(const FSOTS_ParkourResult& Result) override;
    virtual void OnParkourStateChanged_Implementation(ESOTS_ParkourState NewState) override;
    virtual void SetParkourStateTag_Implementation(FGameplayTag StateTag) override;
    virtual void SetParkourActionTag_Implementation(FGameplayTag ActionTag) override;
    virtual void SetClimbStyleTag_Implementation(FGameplayTag ClimbStyleTag) override;
    virtual void SetClimbMovement_Implementation(const FVector& ClimbMovementWS) override;
    virtual void SetLeftHandLedgeLocation_Implementation(const FVector& Location) override;
    virtual void SetRightHandLedgeLocation_Implementation(const FVector& Location) override;
    virtual void SetLeftHandLedgeRotation_Implementation(const FRotator& Rotation) override;
    virtual void SetRightHandLedgeRotation_Implementation(const FRotator& Rotation) override;
    virtual void SetLeftFootLocation_Implementation(const FVector& Location) override;
    virtual void SetRightFootLocation_Implementation(const FVector& Location) override;
    virtual void SetLeftClimbIK_Implementation(bool bEnable) override;
    virtual void SetRightClimbIK_Implementation(bool bEnable) override;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Debug")
    void PrintParkourSnapshot() const;

    // Movement-analysis helpers ported from the legacy AnimBP graph.
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Movement Analysis")
    float Get_LandVelocity() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Movement Analysis")
    float Get_TrajectoryTurnAngle() const;

    // GASP Chooser compatibility helpers (match original AnimInstanceBase API).
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Movement Analysis", meta = (BlueprintThreadSafe))
    bool JustLanded_Heavy() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Movement Analysis", meta = (BlueprintThreadSafe))
    bool JustLanded_Light() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Motion Matching", meta = (BlueprintThreadSafe, DisplayName = "TrajectoryCollision"))
    bool Get_TrajectoryCollision() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Movement Analysis")
    bool PlayLand() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Movement Analysis")
    bool PlayMovingLand() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Movement Analysis", meta = (BlueprintThreadSafe))
    bool IsPivotingBP() const;

    // Play-rate helper ported from the AnimBP graph (BlendStack driven).
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Playback", meta = (BlueprintThreadSafe))
    double Get_DynamicPlayRate(const FAnimNodeReference& BlendStackInput) const;

    // Motion-matching param accessors used by the AnimGraph node.
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Motion Matching", meta = (BlueprintThreadSafe))
    float Get_MMBlendTime() const { return MMBlendTime; }

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Motion Matching", meta = (BlueprintThreadSafe, DisplayName = "Get_MMNotifyRecencyTimeOut"))
    float Get_MMNotifyRecencyTimeout() const { return MMNotifyRecencyTimeout; }

    // BP-facing accessor for MM interrupt mode.
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Motion Matching", meta = (BlueprintThreadSafe, DisplayName = "Get_MMInterruptMode"))
    EPoseSearchInterruptMode Get_MMInterruptMode() const { return MMInterruptMode; }

    // State machine hook stubs to satisfy AnimGraph bindings (plain callable functions, BP can override if desired).
    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine")
    void OnStateEntry_IdleLoop();

    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine")
    void OnStateEntry_TransitionToIdleLoop();

    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine")
    void OnStateEntry_LocomotionLoop();

    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine")
    void OnStateEntry_TransitionToLocomotionLoop();

    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine")
    void OnUpdate_TransitionToLocomotionLoop();

    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine")
    void OnStateEntry_InAirLoop();

    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine")
    void OnStateEntry_TransitionToInAirLoop();

    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine")
    void OnStateEntry_IdleBreak();

    // Blueprint-callable helper that mirrors the legacy AnimBP SetBlendStackAnimFromChooser graph.
    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine", meta = (BlueprintThreadSafe))
    void SetBlendStackAnimFromChooser(ESOTS_StateMachineState InState, bool bInForceBlend);

    // BP hook to surface the BlendStack node reference to C++ (mirrors GASP pattern).
    UFUNCTION(BlueprintImplementableEvent, Category = "SOTS|Parkour|StateMachine", meta = (BlueprintThreadSafe))
    FAnimNodeReference Get_StateMachineBlendStackNode();

    // BP hook for the Motion Matching node's "On Motion Matching State Updated" delegate.
    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|Motion Matching", meta = (BlueprintThreadSafe))
    void OnMotionMatchingStateUpdated();

    // Root offset and warping helpers ported from the AnimBP.
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Root Offset")
    EOffsetRootBoneMode Get_OffsetRootRotationMode() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Root Offset")
    EOffsetRootBoneMode Get_OffsetRootTranslationMode() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Root Offset")
    float Get_OffsetRootTranslationHalfLife() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Root Offset")
    float Get_OffsetRootTranslationRadius() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Root Offset")
    EOrientationWarpingSpace Get_OrientationWarpingWarpingSpace() const;

    // Aim offset helpers ported from the AnimBP.
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Aim Offset")
    bool Enable_AO() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Aim Offset")
    FVector2D Get_AOValue() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Aim Offset")
    float Get_AO_Yaw() const;

    // BP helper: mirrors legacy IsAnimationAlmostComplete graph logic (thread-safe read of computed flag).
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|StateMachine", meta = (BlueprintThreadSafe))
    bool IsAnimationAlmostComplete() const;

    // BP callable helper that mirrors the AnimBP graph: non-looping and below time-remaining threshold.
    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|StateMachine", meta = (BlueprintThreadSafe, DisplayName = "IsAnimationAlmostComplete"))
    bool IsAnimationAlmostComplete_WithNode(const FBlendStackAnimNodeReference& BlendStackNode, float ThresholdSeconds = 0.75f) const;

    // Additive lean helper (relative acceleration normalized -1..1 in actor space).
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Additive Lean")
    FVector CalculateRelativeAccelerationAmount() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Additive Lean")
    FVector2D Get_LeanAmount() const;

    // Steering helpers ported from the AnimBP.
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Steering")
    bool EnableSteering() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Steering", meta = (BlueprintThreadSafe))
    FQuat GetDesiredFacing() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Steering", meta = (BlueprintThreadSafe))
    FQuat Get_DesiredFacing() const;

    // Movement direction setters for BP to drive and keep last-frame parity.
    UFUNCTION(BlueprintCallable, Category = "SOTS|Parkour|State")
    void SetMovementDirection(ESOTS_MovementDirection InDirection);

    // Foot placement helpers ported from the AnimBP.
    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Foot Placement")
    FFootPlacementPlantSettings Get_FootPlacementPlantSettings() const;

    UFUNCTION(BlueprintPure, Category = "SOTS|Parkour|Foot Placement")
    FFootPlacementInterpolationSettings Get_FootPlacementInterpolationSettings() const;

protected:
    void CacheOwnerReferences();
    void UpdateMovementAndCoverData(float DeltaSeconds);
    void UpdateParkourData(float DeltaSeconds);
    void ApplyResultToAnimData(const FSOTS_ParkourResult& Result);
    void UpdateCVarDrivenVariables();
    void UpdateEssentialValues(float DeltaSeconds);
    void UpdateEssentialStates(float DeltaSeconds);
    void UpdateTargetRotation();
    void UpdateMotionMatchingData(float DeltaSeconds);
    void UpdateMotionMatchingPostSelection();
    void UpdateLegacyBpGates(float DeltaSeconds);
    void UpdateExperimentalStateMachine(float DeltaSeconds);
    void UpdateAnimationTick(float DeltaSeconds, bool bThreadSafe);
    bool TryRunStateMachineMotionMatch(const TArray<TObjectPtr<UAnimationAsset>>& CandidateAnims, const FS_ChooserOutputs& ChooserOutputs, FPoseSearchBlueprintResult& OutResult, float& OutSearchCost);

    // --- References ---
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Parkour|Refs", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<ACharacter> SandboxCharacter;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Parkour|Refs", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCharacterMovementComponent> CharacterMovement;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Parkour|Refs", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USOTS_ParkourComponent> ParkourComponent;

    // One-shot log guards to avoid spamming warnings when owner, chooser, or blend stack refs are missing.
    bool bLoggedMissingOwner = false;
    bool bLoggedMissingBlendStackNode = false;
    bool bLoggedMissingChooser = false;

    // --- Parkour state/tags ---
    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|State", meta = (AllowPrivateAccess = "true"))
    FGameplayTag ClimbStyle;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|State", meta = (AllowPrivateAccess = "true"))
    FGameplayTag ParkourAction;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|State", meta = (AllowPrivateAccess = "true"))
    FGameplayTag ParkourState;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|State", meta = (AllowPrivateAccess = "true"))
    FGameplayTag ParkourLogicalState;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|State", meta = (AllowPrivateAccess = "true"))
    FGameplayTag ClimbDirection;

    // --- IK targets ---
    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    FVector RightFootLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    FVector LeftFootLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    FVector LeftHandLedgeLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    FRotator LeftHandLedgeRotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    FVector RightHandLedgeLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    FRotator RightHandLedgeRotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    bool LegIK = false;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    bool ParkourIK_Enabled = false;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    bool IsHidingOnBeam = false;

    // --- Movement / cover / misc ---
    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Movement", meta = (AllowPrivateAccess = "true"))
    FVector PlayersForwardVector = FVector::ForwardVector;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Movement", meta = (AllowPrivateAccess = "true"))
    float CurrentMovementX = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Movement", meta = (AllowPrivateAccess = "true"))
    float CurrentMovementY = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Movement", meta = (AllowPrivateAccess = "true"))
    FVector ClimbMovement = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Movement", meta = (AllowPrivateAccess = "true"))
    float RightArmBlendWeight = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Cover", meta = (AllowPrivateAccess = "true"))
    FGameplayTag CoverState;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Cover", meta = (AllowPrivateAccess = "true"))
    float CoverMovingDirection = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Cover", meta = (AllowPrivateAccess = "true"))
    bool IsPlayerCrouched = false;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|Cover", meta = (AllowPrivateAccess = "true"))
    bool IsInCover_ABP = false;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    bool bLeftClimbIK = false;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Parkour|IK", meta = (AllowPrivateAccess = "true"))
    bool bRightClimbIK = false;

    // --- Essential locomotion state (mirrors legacy ABP UpdateEssentialValues/States) ---
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    TEnumAsByte<EMovementMode> MovementMode = MOVE_None;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    TEnumAsByte<EMovementMode> MovementMode_LastFrame = MOVE_None;

    // Movement direction (mirrors legacy BP enum usage).
    UPROPERTY(Transient, BlueprintReadWrite, Category = "SOTS|Parkour|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_MovementDirection MovementDirection = ESOTS_MovementDirection::F;

    UPROPERTY(Transient, BlueprintReadWrite, Category = "SOTS|Parkour|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_MovementDirection MovementDirection_LastFrame = ESOTS_MovementDirection::F;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_MovementState MovementState = ESOTS_MovementState::Idle;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_MovementState MovementState_LastFrame = ESOTS_MovementState::Idle;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_MovementMode MovementMode_SOTS = ESOTS_MovementMode::OnGround;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_MovementMode MovementMode_SOTS_LastFrame = ESOTS_MovementMode::OnGround;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    E_Gait Gait = E_Gait::Walking;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    E_Gait Gait_LastFrame = E_Gait::Walking;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_Stance Stance = ESOTS_Stance::Stand;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_Stance Stance_LastFrame = ESOTS_Stance::Stand;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_RotationMode RotationMode = ESOTS_RotationMode::OrientToRotation;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    ESOTS_RotationMode RotationMode_LastFrame = ESOTS_RotationMode::OrientToRotation;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    bool HasVelocity = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    bool HasVelocity_LastFrame = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    bool HasAcceleration = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FVector Acceleration = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    float AccelerationAmount = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    float Speed2D = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FVector VelocityAcceleration = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FVector Velocity_LastFrame = FVector::ZeroVector;

    // Landing/movement gates used by the legacy ABP; now sourced from C++.
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FVector LandVelocity = FVector::ZeroVector;

    // Simple movement mode gates for transition rules.
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool IsGrounded = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool IsInAir = false;

    // Steering/root-motion gate used by the AnimBP; C++ owned.
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool CanSteerRootMotion = false;

    // Legacy BP-only gates now owned by C++ (kept for AnimGraph reads; not used by state machines).
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool IsMoving = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool IsStarting = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool IsPivoting = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool ShouldTurnInPlace = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool ShouldSpinTransition = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool JustLandedLight = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool JustLandedHeavy = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    bool JustTraversed = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    FVector2D LeanAmount = FVector2D::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    FVector2D AOValue = FVector2D::ZeroVector;

    // State-machine driven timers (replaces IsAnimationAlmostComplete BP helper).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SOTS|Anim|StateMachine", meta = (AllowPrivateAccess = "true"))
    FName LogicalStateMachineName = TEXT("State Controller");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SOTS|Anim|StateMachine", meta = (AllowPrivateAccess = "true"))
    FName LogicalStateName = TEXT("Idle Loop");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SOTS|Anim|StateMachine", meta = (AllowPrivateAccess = "true"))
    int32 LogicalStateIndex = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SOTS|Anim|StateMachine", meta = (AllowPrivateAccess = "true"))
    float LogicalStateAlmostDoneThreshold = 0.1f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|StateMachine", meta = (AllowPrivateAccess = "true"))
    bool IsLogicalStateAlmostComplete = false;

    // Input state placeholder (Idle/Moving) for BP reads; do not write in BP.
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    FGameplayTag CharacterInputState;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FVector LastNonZeroVelocity = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FTransform CharacterTransform = FTransform::Identity;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FTransform RootTransform = FTransform::Identity;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FRotator TargetRotation = FRotator::ZeroRotator;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    FRotator TargetRotationDelta = FRotator::ZeroRotator;

    UPROPERTY(Transient, BlueprintReadWrite, Category = "SOTS|Anim|State", meta = (AllowPrivateAccess = "true"))
    FRotator TargetRotationOnTransitionStart = FRotator::ZeroRotator;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    bool IsSandboxCharacterValid = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    bool OffsetRootBoneEnabled = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Essential", meta = (AllowPrivateAccess = "true"))
    float OffsetRootTranslationRadius = 0.0f;

    // --- Motion Matching scaffolding (parity with old ABP variables) ---
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    int32 MMDatabaseLOD = 0;

    UPROPERTY(Transient, BlueprintReadWrite, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UPoseSearchDatabase> CurrentSelectedDatabase = nullptr;

    UPROPERTY(Transient, BlueprintReadWrite, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    TArray<FName> CurrentDatabaseTags;

    // Name list mirror for legacy BP bindings that expected an array of FNames.
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    TArray<FName> CurrentDatabaseTagNames;

    // Exposes the motion-matching "no valid animation found" condition for state machine rules.
    UPROPERTY(Transient, BlueprintReadWrite, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    bool NoValidAnim = false;

    // Exposes whether the motion-matching selection is a looping animation (for transition-to-loop decisions).
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    bool SelectedAnimIsLooping = false;

    // Motion matching control values (ported from the old AnimBP helper functions).
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    float MMBlendTime = 0.5f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    float MMNotifyRecencyTimeout = 0.2f;

    // Mirrors EPoseSearchInterruptMode so the AnimGraph switch binds directly.
    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    EPoseSearchInterruptMode MMInterruptMode = EPoseSearchInterruptMode::DoNotInterrupt;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    float HeavyLandSpeedThreshold = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    FTransformTrajectory Trajectory;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    bool TrajectoryCollision = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    FVector TrajectoryVelocityPast = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    FVector TrajectoryVelocityCurrent = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    FVector FutureVelocity = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    FVector TrajectoryVelocityFuture = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    float TrajectoryTimeToLand = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|MM", meta = (AllowPrivateAccess = "true"))
    float PreviousDesiredControllerYaw = 0.0f;

    // --- Experimental state machine (Blend Stack + Chooser/MM) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true"))
    FS_BlendStackInputs BlendStackInputs;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true"))
    FS_BlendStackInputs Previous_BlendStackInputs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true"))
    // Default to locomotion so the experimental state machine starts on a grounded walk/run path.
    ESOTS_StateMachineState StateMachineState = ESOTS_StateMachineState::LocomotionLoop;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true"))
    bool bNoValidAnim = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true", DisplayName = "NotifyTransition_Re-Transition"))
    bool bNotifyTransition_ReTransition = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true", DisplayName = "NotifyTransition_ToLoop"))
    bool bNotifyTransition_ToLoop = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true", DisplayName = "Search Cost"))
    float SearchCost = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true"))
    float MMCostLimit_Default = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true", DisplayName = "ForceBlend"))
    bool bForceBlend = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UChooserTable> AnimationsForStateMachineChooser;

    // Set in AnimGraph via ConvertToBlendStackNode so we can call ForceBlendNextUpdate.
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true"))
    FBlendStackAnimNodeReference StateMachineBlendStackNode;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true"))
    bool UseExperimentalStateMachine = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "State Machine (Experimental)", meta = (AllowPrivateAccess = "true"))
    bool DebugExperimentalStateMachine = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "SOTS|Anim|Threading", meta = (AllowPrivateAccess = "true"))
    bool UseThreadSafeUpdateAnimation = false;

    // Tracks the last frame where thread-safe update ran so we can skip duplicate game-thread ticks.
    uint64 LastThreadSafeUpdateFrame = 0;

    // Land gating helpers to suppress tiny air blips from triggering land animations.
    float TimeInAir = 0.0f;
    float TimeOnGround = 0.0f;
    float MinAirTimeForLand = 0.08f;
    float MinLandSpeedForLand = 0.0f;

    bool bTriedCacheOnce = false;

private:
    // Resolves the BlendStack node via the BP getter if it has not been cached yet.
    bool TryResolveStateMachineBlendStackNode();

    float Get_StrafeYawRotationOffset() const;
    void SnapToGroundOnLand();
    void UpdateMotionMatchingTrajectory(float DeltaSeconds);

    // Foot placement tuning (mirrors ABP defaults and stop variants).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Foot Placement", meta = (AllowPrivateAccess = "true"))
    FFootPlacementPlantSettings PlantSettings_Default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Foot Placement", meta = (AllowPrivateAccess = "true"))
    FFootPlacementPlantSettings PlantSettings_Stops;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Foot Placement", meta = (AllowPrivateAccess = "true"))
    FFootPlacementInterpolationSettings InterpolationSettings_Default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOTS|Parkour|Foot Placement", meta = (AllowPrivateAccess = "true"))
    FFootPlacementInterpolationSettings InterpolationSettings_Stops;
};
