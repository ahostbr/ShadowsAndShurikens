#include "SOTS_ParkourComponent.h"

#include "SOTS_ParkourLog.h"

#include "SOTS_ParkourConfig.h"
#include "SOTS_ParkourTagTypes.h"
#include "SOTS_ParkourActionData.h"
#include "SOTS_ParkourDebugLibrary.h"
#include "SOTS_ParkourInterface.h"
#include "SOTS_ParkourSurfaceProbes.h"
#include "SOTS_ParkourVerticalTraceLibrary.h"
#include "SOTS_ParkourStatsWidgetInterface.h"
#include "SOTS_TagAccessHelpers.h"
#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "GameplayTagContainer.h"
#include "MotionWarpingComponent.h"
#include "OmniTraceBlueprintLibrary.h"
#include "OmniTraceTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/SpringArmComponent.h"
#include "Curves/CurveFloat.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "WorldCollision.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/EngineTypes.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"

// NOTE (CGF parity coverage):
// - Core detection/selection/execute paths map to CGF_Parkour's TryPerformParkour/TryParkourAction graphs.
// - Debug-only Blueprint graphs (e.g., DebugDrawAllTraces) are intentionally consolidated into the existing debug flags.

// SOTS_Parkour detection model (post-refactor):
// - PerformParkourDetection is the single entrypoint for physics traces.
// - Detection is event-driven (input/continuation), not tick-driven.
// - A per-frame trace budget and LOD gates cap work.
// - Mantle/Vault/Drop/TicTac behavior and MotionWarping remain unchanged.

namespace
{
FGameplayTag MakeDirectionTag(float RightOffset, bool bBackward)
{
	static const FGameplayTag TagForward = FGameplayTag::RequestGameplayTag(TEXT("Parkour.Direction.Forward"), false);
	static const FGameplayTag TagBackward = FGameplayTag::RequestGameplayTag(TEXT("Parkour.Direction.Backward"), false);
	static const FGameplayTag TagLeft = FGameplayTag::RequestGameplayTag(TEXT("Parkour.Direction.Left"), false);
	static const FGameplayTag TagRight = FGameplayTag::RequestGameplayTag(TEXT("Parkour.Direction.Right"), false);

	if (bBackward)
	{
		return TagBackward;
	}

	const float Threshold = 5.0f;
	if (RightOffset > Threshold)
	{
		return TagRight;
	}
	if (RightOffset < -Threshold)
	{
		return TagLeft;
	}
	return TagForward;
}

bool ConvertOmniTraceHit(const FOmniTracePatternResult& Result, const FVector& Start, const FVector& End, FHitResult& OutHit)
{
	if (!Result.bAnyHit)
	{
		OutHit = FHitResult();
		return false;
	}

	const FOmniTraceHitResult& First = Result.FirstBlockingHit;

	FHitResult Hit(First.HitActor.Get(), First.HitComponent.Get(), First.ImpactPoint, First.Normal);
	Hit.Location      = First.Location;
	Hit.ImpactPoint   = First.ImpactPoint;
	Hit.Normal        = First.Normal;
	Hit.ImpactNormal  = First.Normal;
	Hit.bBlockingHit  = First.bBlockingHit;
	Hit.Distance      = First.Distance;
	Hit.TraceStart    = Start;
	Hit.TraceEnd      = End;

	OutHit = Hit;
	return First.bBlockingHit;
}

bool RunOmniTraceSweep(
	UObject* WorldContextObject,
	const FVector& Start,
	const FVector& End,
	ECollisionChannel TraceChannel,
	EOmniTraceShape Shape,
	float Radius,
	float HalfHeight,
	const FVector& BoxHalfExtents,
	const AActor* ActorToIgnore,
	FHitResult& OutHit)
{
	// NOTE: All parkour traces route through this OmniTrace bridge to enforce a single backend
	// and to keep parity with the trace budget/CGF configs.
	const FVector Delta = End - Start;
	const float Distance = Delta.Size();
	const FVector Dir = Delta.IsNearlyZero() ? FVector::ForwardVector : Delta.GetSafeNormal();

	FOmniTraceRequest Request;
	Request.PatternFamily           = EOmniTraceTraceFamily::Forward;
	Request.ForwardPattern          = EOmniTraceForwardPattern::SingleRay;
	Request.Shape                   = Shape;
	Request.ShapeRadius             = Radius;
	Request.CapsuleHalfHeight       = HalfHeight;
	Request.BoxHalfExtents          = BoxHalfExtents.IsNearlyZero() ? Request.BoxHalfExtents : BoxHalfExtents;
	Request.TraceChannel            = TraceChannel;
	Request.RayCount                = 1;
	Request.MaxDistance             = FMath::Max(Distance, KINDA_SMALL_NUMBER);
	Request.OriginLocationOverride  = Start;
	Request.OriginRotationOverride  = Dir.Rotation();
	Request.DebugOptions.bEnableDebug = false; // Existing debug drawing is handled separately.

	if (IsValid(const_cast<AActor*>(ActorToIgnore)))
	{
		Request.OriginActor = const_cast<AActor*>(ActorToIgnore);
	}

	const FOmniTracePatternResult PatternResult = UOmniTraceBlueprintLibrary::OmniTrace_Pattern(WorldContextObject, Request);
	return ConvertOmniTraceHit(PatternResult, Start, Start + Dir * Request.MaxDistance, OutHit);
}

bool RunCapsuleTraceWithOmniTrace(
	UObject* WorldContextObject,
	const FVector& Start,
	const FVector& End,
	float Radius,
	float HalfHeight,
	ECollisionChannel TraceChannel,
	const AActor* ActorToIgnore,
	FHitResult& OutHit)
{
	return RunOmniTraceSweep(
		WorldContextObject,
		Start,
		End,
		TraceChannel,
		EOmniTraceShape::CapsuleSweep,
		Radius,
		HalfHeight,
		FVector::ZeroVector,
		ActorToIgnore,
		OutHit);
}

bool RunSphereTraceWithOmniTrace(
	UObject* WorldContextObject,
	const FVector& Start,
	const FVector& End,
	float Radius,
	ECollisionChannel TraceChannel,
	const AActor* ActorToIgnore,
	FHitResult& OutHit)
{
	return RunOmniTraceSweep(
		WorldContextObject,
		Start,
		End,
		TraceChannel,
		EOmniTraceShape::SphereSweep,
		Radius,
		Radius,
		FVector::ZeroVector,
		ActorToIgnore,
		OutHit);
}

} // namespace

void USOTS_ParkourComponent::SetIsGrounded(bool bInIsGrounded)
{
	bHasIsGroundedInput = true;
	bIsGroundedInput = bInIsGrounded;
}

void USOTS_ParkourComponent::SetIsInAir(bool bInIsInAir)
{
	bHasIsInAirInput = true;
	bIsInAirInput = bInIsInAir;
}

void USOTS_ParkourComponent::SetMovementSpeed2D(float InSpeed2D)
{
	bHasSpeed2DInput = true;
	MovementSpeed2DInput = FMath::Max(0.0f, InSpeed2D);
}

void USOTS_ParkourComponent::SetDesiredInputDirection(const FVector& InInputDirection)
{
	bHasInputDirection = true;
	InputDirection = InInputDirection;
}

void USOTS_ParkourComponent::SetIsSprinting(bool bInIsSprinting)
{
	bHasIsSprintingInput = true;
	bIsSprintingInput = bInIsSprinting;
}

FVector USOTS_ParkourComponent::GetResolvedInputDirection() const
{
	if (bHasInputDirection)
	{
		return InputDirection;
	}
	return CachedMovementComponent ? CachedMovementComponent->Velocity : FVector::ZeroVector;
}

float USOTS_ParkourComponent::GetResolvedSpeed2D() const
{
	if (bHasSpeed2DInput)
	{
		return MovementSpeed2DInput;
	}
	return CachedMovementComponent ? CachedMovementComponent->Velocity.Size2D() : 0.0f;
}

bool USOTS_ParkourComponent::GetResolvedIsInAir() const
{
	if (bHasIsInAirInput)
	{
		return bIsInAirInput;
	}
	return CachedMovementComponent ? CachedMovementComponent->IsFalling() : false;
}

bool USOTS_ParkourComponent::GetResolvedIsGrounded() const
{
	if (bHasIsGroundedInput)
	{
		return bIsGroundedInput;
	}
	return !GetResolvedIsInAir();
}

bool USOTS_ParkourComponent::GetResolvedIsSprinting() const
{
	if (bHasIsSprintingInput)
	{
		return bIsSprintingInput;
	}
	return false;
}

FTransform USOTS_ParkourComponent::GetLeftHandBaseTransform() const
{
	return FTransform(LastResult.LeftHandBaseRotation, LastResult.LeftHandBaseLocation);
}

FTransform USOTS_ParkourComponent::GetRightHandBaseTransform() const
{
	return FTransform(LastResult.RightHandBaseRotation, LastResult.RightHandBaseLocation);
}

FVector USOTS_ParkourComponent::GetLeftFootLocation() const
{
	const FHitResult& Hit = LastResult.LeftFootHit;
	if (Hit.bBlockingHit)
	{
		return !Hit.ImpactPoint.IsNearlyZero() ? Hit.ImpactPoint : Hit.Location;
	}
	return FVector::ZeroVector;
}

FVector USOTS_ParkourComponent::GetRightFootLocation() const
{
	const FHitResult& Hit = LastResult.RightFootHit;
	if (Hit.bBlockingHit)
	{
		return !Hit.ImpactPoint.IsNearlyZero() ? Hit.ImpactPoint : Hit.Location;
	}
	return FVector::ZeroVector;
}

FName USOTS_ParkourComponent::GetResolvedStateName() const
{
	return GetParkourStateTag().GetTagName();
}

FName USOTS_ParkourComponent::GetResolvedActionName() const
{
	return GetParkourActionTag().GetTagName();
}

void USOTS_ParkourComponent::TickLedgeShimmy(float DeltaTime)
{
	(void)DeltaTime;

	ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		bIsLedgeShimmyActive = false;
		return;
	}

	const bool bHasLedgeResult = LastResult.bIsValid && LastResult.bHasResult;
	const bool bInLedgeState = ParkourLogicalState == ESOTS_ParkourLogicalState::ReachLedge || ParkourLogicalState == ESOTS_ParkourLogicalState::Climb;
	const bool bLifecycleAllows = ParkourState == ESOTS_ParkourState::Active;

	if (!bHasLedgeResult || !bInLedgeState || !bLifecycleAllows)
	{
		if (bIsLedgeShimmyActive && bEnableDebugLogging)
		{
			UE_LOG(LogSOTS_Parkour, Verbose, TEXT("Shimmy stopped (state/result invalid)."));
		}
		bIsLedgeShimmyActive = false;
		return;
	}

	FVector SurfaceNormal = !LastResult.SurfaceNormal.IsNearlyZero() ? LastResult.SurfaceNormal : LastResult.WorldNormal;
	if (SurfaceNormal.IsNearlyZero())
	{
		SurfaceNormal = -Character->GetActorForwardVector();
	}

	FVector Tangent = FVector::CrossProduct(FVector::UpVector, SurfaceNormal);
	if (!Tangent.Normalize())
	{
		Tangent = Character->GetActorRightVector();
	}

	const FVector InputDir = GetResolvedInputDirection();
	const FVector Input2D = InputDir.GetSafeNormal2D();
	const FVector Tangent2D = Tangent.GetSafeNormal2D();
	float InputAlong = FVector::DotProduct(Input2D, Tangent2D);
	if (!FMath::IsFinite(InputAlong))
	{
		InputAlong = 0.0f;
	}

	if (FMath::Abs(InputAlong) < LedgeShimmyInputThreshold)
	{
		if (bIsLedgeShimmyActive && bEnableDebugLogging)
		{
			UE_LOG(LogSOTS_Parkour, Verbose, TEXT("Shimmy stopped (input below threshold)."));
		}
		bIsLedgeShimmyActive = false;
		return;
	}

	if (!bIsLedgeShimmyActive && bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Log, TEXT("Shimmy start: input=%.2f tangent=%s state=%d"), InputAlong, *Tangent.ToString(), static_cast<int32>(ParkourLogicalState));
	}

	bIsLedgeShimmyActive = true;
	Character->AddMovementInput(Tangent, InputAlong);
}

USOTS_ParkourComponent::USOTS_ParkourComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
	LastBroadcastParkourState = ParkourState;

	// Default camera move curve (mirrors BP ParkourCameraMove asset).
	CameraMoveCurve = TSoftObjectPtr<UCurveFloat>(FSoftObjectPath(TEXT("/Game/CGF_Parkour/Curve/ParkourCameraMove.ParkourCameraMove")));
}

void USOTS_ParkourComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (ACharacter* AsCharacter = Cast<ACharacter>(Owner))
	{
		CachedOwnerCharacter = AsCharacter;
		CachedMovementComponent = AsCharacter->GetCharacterMovement();
	}
	else if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("SOTS_ParkourComponent: Owner is not a Character; some parkour features may not work correctly."));
	}

	if (CachedOwnerCharacter.IsValid())
	{
		CachedOwnerCharacter->LandedDelegate.AddDynamic(this, &USOTS_ParkourComponent::OnOwnerLanded);

		CachedOwnerCharacter->MovementModeChangedDelegate.AddDynamic(this, &USOTS_ParkourComponent::OnOwnerMovementModeChanged);
	}

	if (!CameraBoom && CachedOwnerCharacter.IsValid())
	{
		CameraBoom = CachedOwnerCharacter->FindComponentByClass<USpringArmComponent>();
	}

	if (CameraBoom)
	{
		FirstCameraRelativeLocation = CameraBoom->GetRelativeLocation();
		TargetRelativeCameraLocation = FirstCameraRelativeLocation;

		if (CameraTargetArmLength <= 0.0f)
		{
			CameraTargetArmLength = CameraBoom->TargetArmLength;
		}
	}

	ApplyConfigFromDataAsset();

	ParkourState = ESOTS_ParkourState::Idle;
	ParkourLogicalState = ESOTS_ParkourLogicalState::NotBusy;
	LastResult.Reset();
	UpdateGameplayTagsFromState(ParkourLogicalState);

	if (bEnableDebugLogging)
	{
		ValidateCurrentConfig();
	}
}

const USOTS_ParkourConfig* USOTS_ParkourComponent::GetConfig() const
{
	return ParkourConfig ? ParkourConfig.Get() : GetDefault<USOTS_ParkourConfig>();
}

void USOTS_ParkourComponent::ApplyConfigFromDataAsset()
{
	const USOTS_ParkourConfig* Config = GetConfig();
	if (!Config)
	{
		return;
	}

	bContinuousTraceMode = Config->bContinuousTraceMode;
	MaxTracesPerFrame = Config->MaxTracesPerFrame;
	MinSpeedForDetection = Config->MinSpeedForDetection;
	MaxCameraDistanceForDetection = Config->MaxCameraDistanceForDetection;
	CharacterHeightDiff = Config->CharacterHeightDiff;
	CharacterHeightAdjustment = Config->CharacterHeightAdjustment;
	ClimbTraceCooldownMin = Config->ClimbTraceCooldownMin;
	ClimbTraceCooldownMax = Config->ClimbTraceCooldownMax;

	VaultMinHeight = Config->VaultMinHeight;
	VaultMaxHeight = Config->VaultMaxHeight;
	SecondTraceForwardOffset = Config->SecondTraceForwardOffset;
	SecondTraceVerticalOffset = Config->SecondTraceVerticalOffset;
	CapsuleDebugDrawTime = CapsuleDebugDrawTime; // unchanged; keeps existing setting

	// Confirm overlap sizing
	if (Config->ConfirmCapsuleRadius > 0.0f)
	{
		CapsuleDebugDrawTime = CapsuleDebugDrawTime; // no-op; placeholder to avoid unused warnings
	}

	HorizontalWallTraceHalfQuantity_NotBusy = Config->HorizontalWallTraceHalfQuantity_NotBusy;
	HorizontalWallTraceHalfQuantity_Climb = Config->HorizontalWallTraceHalfQuantity_Climb;
	HorizontalWallTraceRange = Config->HorizontalWallTraceRange;
	VerticalWallTraceQuantity_NotBusy = Config->VerticalWallTraceQuantity_NotBusy;
	VerticalWallTraceQuantity_Climb = Config->VerticalWallTraceQuantity_Climb;
	VerticalWallTraceRange = Config->VerticalWallTraceRange;
	bUseWallGridOverlap = Config->bUseWallGridOverlap;
	DistanceFromLedgeXYCutoff = Config->DistanceFromLedgeXYCutoff;

	LedgeMoveHorizontalDistance = Config->LedgeMoveHorizontalDistance;
	LedgeMoveVerticalOffset = Config->LedgeMoveVerticalOffset;
	LedgeMoveProbeRadius = Config->LedgeMoveProbeRadius;
	HopVerticalLift = Config->HopVerticalLift;

	BackHopDistance = Config->BackHopDistance;
	BackHopInnerDistance = Config->BackHopInnerDistance;
	BackHopAngleToleranceDegrees = Config->BackHopAngleToleranceDegrees;
	BackHopProbeRadius = Config->BackHopProbeRadius;
	BackHopTraceStartZOffset = Config->BackHopTraceStartZOffset;
	BackHopYawMaxDegrees = Config->BackHopYawMaxDegrees;
	BackHopYawMinDegrees = Config->BackHopYawMinDegrees;
	BackHopYawCenterDeadzoneDegrees = Config->BackHopYawCenterDeadzoneDegrees;
	BackHopForwardYawRejectDegrees = Config->BackHopForwardYawRejectDegrees;
	BackHopRightInputThreshold = Config->BackHopRightInputThreshold;
	BackHopForwardInputThreshold = Config->BackHopForwardInputThreshold;
	BackHopSurfaceCapsuleRadius = Config->BackHopSurfaceCapsuleRadius;
	BackHopSurfaceCapsuleHalfHeight = Config->BackHopSurfaceCapsuleHalfHeight;
	BackHopSurfaceForwardOffset = Config->BackHopSurfaceForwardOffset;
	bValidateBackHopSurface = Config->bValidateBackHopSurface;

	HandIKProbeRadius = Config->HandIKProbeRadius;
	HandIKHorizontalOffset = Config->HandIKHorizontalOffset;
	HandIKHorizontalOffsetLeft = Config->HandIKHorizontalOffsetLeft;
	HandIKHorizontalOffsetRight = Config->HandIKHorizontalOffsetRight;
	HandIKVerticalLift = Config->HandIKVerticalLift;
	HandIKTraceDepth = Config->HandIKTraceDepth;
	HandIKNormalDotThreshold = Config->HandIKNormalDotThreshold;
	HandIKInterpSpeed = Config->HandIKInterpSpeed;
	HandIKInterpSpeedNotClimb = Config->HandIKInterpSpeedNotClimb;
	HandIKMaxConsecutiveFailures = Config->HandIKMaxConsecutiveFailures;
	HandIKFailureOffsetZ = Config->HandIKFailureOffsetZ;
	HandIKMaxConsecutiveMissFrames = Config->HandIKMaxConsecutiveMissFrames;
	HandIKMissCooldownFrames = Config->HandIKMissCooldownFrames;

	ClimbIKHandSpacing = Config->ClimbIKHandSpacing;
	ClimbIKForwardOffset = Config->ClimbIKForwardOffset;
	ClimbIKUpOffset = Config->ClimbIKUpOffset;

	FootIKProbeRadius = Config->FootIKProbeRadius;
	FootIKCornerMoveProbeRadius = Config->FootIKCornerMoveProbeRadius;
	FootIKForwardOffset = Config->FootIKForwardOffset;
	FootIKRightOffset = Config->FootIKRightOffset;
	FootIKVerticalLift = Config->FootIKVerticalLift;
	FootIKTraceDepth = Config->FootIKTraceDepth;
}

void USOTS_ParkourComponent::DebugPrintConfig() const
{
	if (!bEnableDebugLogging)
	{
		return;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	if (!Config)
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("SOTS_Parkour: No ParkourConfig assigned; using hardcoded defaults."));
		return;
	}

	UE_LOG(LogSOTS_Parkour, Verbose, TEXT("Config: TracesPerFrame=%d Continuous=%d SpeedMin=%.1f CameraMax=%.1f Vault=[%.1f, %.1f] LedgeMoveDist=%.1f ProbeRadius=%.1f HopLift=%.1f"),
		Config->MaxTracesPerFrame,
		Config->bContinuousTraceMode ? 1 : 0,
		Config->MinSpeedForDetection,
		Config->MaxCameraDistanceForDetection,
		Config->VaultMinHeight,
		Config->VaultMaxHeight,
		Config->LedgeMoveHorizontalDistance,
		Config->LedgeMoveProbeRadius,
		Config->HopVerticalLift);

	UE_LOG(LogSOTS_Parkour, Verbose, TEXT("Config: BackHopDist=%.1f Inner=%.1f YawRange=[%.1f, %.1f] ValidateSurface=%d"),
		Config->BackHopDistance,
		Config->BackHopInnerDistance,
		Config->BackHopYawMinDegrees,
		Config->BackHopYawMaxDegrees,
		Config->bValidateBackHopSurface ? 1 : 0);

	UE_LOG(LogSOTS_Parkour, Verbose, TEXT("Config: HandIK Radius=%.1f Lift=%.1f Depth=%.1f NormalDot=%.2f Interp=%.2f/%.2f (climb/non) | FootIK Radius=%.1f Forward=%.1f Depth=%.1f"),
		Config->HandIKProbeRadius,
		Config->HandIKVerticalLift,
		Config->HandIKTraceDepth,
		Config->HandIKNormalDotThreshold,
		Config->HandIKInterpSpeed,
		Config->HandIKInterpSpeedNotClimb,
		Config->FootIKProbeRadius,
		Config->FootIKForwardOffset,
		Config->FootIKTraceDepth);

	UE_LOG(LogSOTS_Parkour, Verbose, TEXT("Trace stats: Frame=%d Action=%d MaxFrame=%d Total=%d Budget=%d"),
		TraceStats.TracesThisFrame,
		TraceStats.TracesThisAction,
		MaxTracesInSingleFrame,
		TotalTracesSinceBeginPlay,
		Config->MaxTracesPerFrame);
}

void USOTS_ParkourComponent::SetParkourDebugEnabled(bool bEnabled)
{
	bEnableTraceDebug = bEnabled;
	bEnableDebugLogging = bEnabled;

	if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Display, TEXT("SOTS_Parkour: Debug visuals %s"), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
	}
}

void USOTS_ParkourComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ResetTraceStatsForFrame();
	UpdateStateMachine(DeltaTime);
	TickLedgeShimmy(DeltaTime);

	// Handle lingering exit state before returning to Idle.
	if (ParkourState == ESOTS_ParkourState::Exiting && ParkourStateLingerTime > 0.0f)
	{
		ParkourStateLingerTime = FMath::Max(0.0f, ParkourStateLingerTime - DeltaTime);
		if (ParkourStateLingerTime <= 0.0f)
		{
			SetParkourState(ESOTS_ParkourState::Idle);
		}
	}

	// Drive climb trace cooldown timer on tick.
	if (ClimbTraceCooldownTime > 0.0f)
	{
		ClimbTraceCooldownTime = FMath::Max(0.0f, ClimbTraceCooldownTime - DeltaTime);
	}

	if (ParkourLogicalState != ESOTS_ParkourLogicalState::Climb)
	{
		ClimbTraceCooldownTime = 0.0f;
	}

	if (bContinuousTraceMode && ParkourState == ESOTS_ParkourState::Idle && ParkourLogicalState == ESOTS_ParkourLogicalState::NotBusy)
	{
		const FSOTS_ParkourResult PreviousResult = LastResult;
		const FVector ResolvedInputDirection = GetResolvedInputDirection();
		PerformParkourDetection(ResolvedInputDirection, false);

		const bool bHadResult = PreviousResult.bHasResult && PreviousResult.bIsValid;
		const bool bHasResultNow = LastResult.bHasResult && LastResult.bIsValid;
		const bool bActionChanged = LastResult.Action != PreviousResult.Action;
		const bool bLocationChanged = !LastResult.WorldLocation.Equals(PreviousResult.WorldLocation, 1.0f);

		if ((bHasResultNow || bHadResult) && (bActionChanged || bHasResultNow != bHadResult || bLocationChanged))
		{
			NotifyParkourResultUpdated(LastResult);
		}
	}
}

void USOTS_ParkourComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USOTS_ParkourComponent, ParkourState);
	DOREPLIFETIME(USOTS_ParkourComponent, ParkourLogicalState);
	DOREPLIFETIME(USOTS_ParkourComponent, LastResult);
}

void USOTS_ParkourComponent::OnRep_ParkourState()
{
	const ESOTS_ParkourState PreviousState = LastBroadcastParkourState;
	NotifyParkourStateChanged(PreviousState, ParkourState);
}

bool USOTS_ParkourComponent::AutoClimb(const FString& Reason)
{
	return TriggerAutoClimbJump(Reason);
}

bool USOTS_ParkourComponent::CheckBackHop(const FVector& InInputDirection, float DeltaYawDegrees) const
{
	return IsBackHopInputAllowed(InInputDirection, DeltaYawDegrees);
}

bool USOTS_ParkourComponent::CheckBackHopSurface(const FVector& WarpPoint, const FVector& Forward) const
{
	return CheckBackHopSurfaceClear(WarpPoint, Forward, CachedOwnerCharacter.Get());
}

bool USOTS_ParkourComponent::CheckMantleSurface(const FHitResult& FirstHit, float CapsuleHalfHeight, float HeightDelta, FSOTS_ParkourResult& OutResult) const
{
	OutResult.Reset();
	OutResult.Action = ESOTS_ParkourAction::Mantle;
	OutResult.ResultType = ESOTS_ParkourResultType::Mantle;
	OutResult.HeightDelta = HeightDelta;
	OutResult.Hit = FirstHit;
	return ConfirmSurfaceForAction(FirstHit, CapsuleHalfHeight, ESOTS_ParkourAction::Mantle, OutResult);
}

bool USOTS_ParkourComponent::CheckVaultSurface(const FHitResult& FirstHit, float CapsuleHalfHeight, float HeightDelta, FSOTS_ParkourResult& OutResult) const
{
	OutResult.Reset();
	OutResult.Action = ESOTS_ParkourAction::Vault;
	OutResult.ResultType = ESOTS_ParkourResultType::Mantle;
	OutResult.HeightDelta = HeightDelta;
	OutResult.Hit = FirstHit;
	return ConfirmSurfaceForAction(FirstHit, CapsuleHalfHeight, ESOTS_ParkourAction::Vault, OutResult);
}

bool USOTS_ParkourComponent::CheckPredictJump(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& OutResult) const
{
	return PredictJumpLanding(FirstHit, CapsuleHalfHeight, OutResult);
}

bool USOTS_ParkourComponent::CheckPredictJumpSurface(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& OutResult) const
{
	return PredictJumpLanding(FirstHit, CapsuleHalfHeight, OutResult);
}

float USOTS_ParkourComponent::GetClimbForwardValue() const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return 0.0f;
	}

	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(GetResolvedInputDirection().GetSafeNormal2D(), Forward);
}

float USOTS_ParkourComponent::GetClimbRightValue() const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return 0.0f;
	}

	const FVector Right = Character->GetActorRightVector().GetSafeNormal2D();
	return FVector::DotProduct(GetResolvedInputDirection().GetSafeNormal2D(), Right);
}

float USOTS_ParkourComponent::GetHorizontalAxis() const
{
	return GetResolvedInputDirection().X;
}

float USOTS_ParkourComponent::GetVerticalAxis() const
{
	return GetResolvedInputDirection().Z;
}

bool USOTS_ParkourComponent::IsParkourActionEqualToAny(const TArray<ESOTS_ParkourAction>& Actions) const
{
	return Actions.Contains(LastResult.Action);
}

bool USOTS_ParkourComponent::IsParkourStateEqualToAny(const TArray<ESOTS_ParkourState>& States) const
{
	return States.Contains(ParkourState);
}

float USOTS_ParkourComponent::GetDeltaSecondWithTimeDilation() const
{
	const UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	const float Delta = World ? World->GetDeltaSeconds() : 0.0f;
	const float OwnerDilation = OwnerActor ? OwnerActor->CustomTimeDilation : 1.0f;
	return Delta * OwnerDilation;
}

FRotator USOTS_ParkourComponent::GetWarpRotation() const
{
	FVector Facing = -LastResult.SurfaceNormal;
	if (Facing.IsNearlyZero())
	{
		Facing = -LastResult.WorldNormal;
	}
	if (Facing.IsNearlyZero())
	{
		Facing = CachedOwnerCharacter.IsValid() ? -CachedOwnerCharacter->GetActorForwardVector() : FVector::ForwardVector;
	}
	Facing = Facing.GetSafeNormal();
	return Facing.Rotation();
}

bool USOTS_ParkourComponent::IsLedgeValid() const
{
	return LastResult.bHasResult && LastResult.Hit.IsValidBlockingHit();
}

FVector USOTS_ParkourComponent::GetHopDirection() const
{
	return GetResolvedInputDirection().GetSafeNormal();
}

void USOTS_ParkourComponent::OnRep_LogicalState()
{
	UpdateGameplayTagsFromState(ParkourLogicalState);
}

float USOTS_ParkourComponent::GetExitLingerForAction(ESOTS_ParkourAction Action) const
{
	const USOTS_ParkourConfig* Config = GetConfig();
	const float VaultLinger = Config ? Config->LingerVaultSeconds : 0.35f;
	const float MantleLinger = Config ? Config->LingerMantleSeconds : 0.25f;
	const float HangLinger = Config ? Config->LingerHangSeconds : 0.20f;
	const float HopLinger = Config ? Config->LingerHopSeconds : 0.15f;

	switch (Action)
	{
	case ESOTS_ParkourAction::Vault:
	case ESOTS_ParkourAction::ThinVault:
	case ESOTS_ParkourAction::HighVault:
	case ESOTS_ParkourAction::VaultToBraced:
		return VaultLinger; // BP linger after vaults
	case ESOTS_ParkourAction::Mantle:
	case ESOTS_ParkourAction::LowMantle:
	case ESOTS_ParkourAction::DistanceMantle:
		return MantleLinger; // shorter settle for mantles
	case ESOTS_ParkourAction::CornerMove:
	case ESOTS_ParkourAction::LedgeMove:
	case ESOTS_ParkourAction::AirHang:
		return HangLinger;
	case ESOTS_ParkourAction::BackHop:
	case ESOTS_ParkourAction::TicTac:
	case ESOTS_ParkourAction::PredictJump:
		return HopLinger;
	default:
		return 0.0f;
	}
}

void USOTS_ParkourComponent::OnRep_LastResult()
{
	NotifyParkourResultUpdated(LastResult);
	UpdateGameplayTagsFromResult(LastResult);
}

void USOTS_ParkourComponent::NotifyParkourResultUpdated(const FSOTS_ParkourResult& Result)
{
	BP_OnParkourResultUpdated(Result);

	if (AActor* Owner = GetOwner())
	{
		if (Owner->GetClass()->ImplementsInterface(USOTS_ParkourInterface::StaticClass()))
		{
			ISOTS_ParkourInterface::Execute_OnParkourResultUpdated(Owner, Result);
		}
	}

	PushParkourDataToAnimInstance(Result);

	UpdateGameplayTagsFromResult(Result);
	PushStatsToWidget();
}

void USOTS_ParkourComponent::NotifyParkourStateChanged(ESOTS_ParkourState PreviousState, ESOTS_ParkourState NewState)
{
	if (PreviousState == NewState)
	{
		return;
	}

	BP_OnParkourStateChanged(NewState);

	if (AActor* Owner = GetOwner())
	{
		if (Owner->GetClass()->ImplementsInterface(USOTS_ParkourInterface::StaticClass()))
		{
			ISOTS_ParkourInterface::Execute_OnParkourStateChanged(Owner, NewState);
		}
	}

	PushParkourStateToAnimInstance(NewState);

	OnParkourStateChanged.Broadcast(PreviousState, NewState);
	LastBroadcastParkourState = NewState;

	PushStatsToWidget();
}

bool USOTS_ParkourComponent::PlayActionMontage(const FSOTS_ParkourActionDefinition& ActionDef) const
{
	UAnimMontage* Montage = ActionDef.ParkourMontage.LoadSynchronous();
	if (!Montage)
	{
		return false;
	}

	OnRequestPlayMontage.Broadcast(Montage, ActionDef.PlayRate, ActionDef.StartingPosition);
	return true;
}

FGameplayTag USOTS_ParkourComponent::GetParkourStateTag() const
{
	return GetStateTag(ParkourLogicalState);
}

FGameplayTag USOTS_ParkourComponent::GetParkourActionTag() const
{
	return GetActionTag(LastResult.Action);
}

FGameplayTag USOTS_ParkourComponent::GetClimbStyleTag() const
{
	return GetClimbStyleTag(LastResult.ClimbStyle);
}

FGameplayTag USOTS_ParkourComponent::GetCurrentParkourStateTag() const
{
	return GetParkourStateTag();
}

FGameplayTag USOTS_ParkourComponent::GetCurrentParkourActionTag() const
{
	return GetParkourActionTag();
}

FGameplayTag USOTS_ParkourComponent::GetCurrentClimbStyleTag() const
{
	return GetClimbStyleTag();
}

FGameplayTag USOTS_ParkourComponent::GetCurrentDirectionTag() const
{
	return LastResult.DirectionTag;
}

void USOTS_ParkourComponent::FinalizeResultTags(FSOTS_ParkourResult& Result) const
{
	Result.StateTag = GetParkourStateTag();
	if (!Result.ActionTag.IsValid())
	{
		Result.ActionTag = GetActionTag(Result.Action);
	}
	if (!Result.ClimbStyleTag.IsValid())
	{
		Result.ClimbStyleTag = GetClimbStyleTag(Result.ClimbStyle);
	}
	Result.bHasClimbLedgeResult = Result.Hit.IsValidBlockingHit();
	if (Result.bHasClimbLedgeResult)
	{
		Result.ClimbLedgeResult = Result.Hit;
	}

	// Derive climb IK anchors and foot IK probes after tags are finalized so state gating can mirror the BP graph.
	ComputeClimbIKTargets(Result);
	ComputeFootIKProbes(Result);
}

UObject* USOTS_ParkourComponent::GetParkourAnimInstanceObject() const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return nullptr;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		return nullptr;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return nullptr;
	}

	return AnimInstance->GetClass()->ImplementsInterface(USOTS_ParkourABPInterface::StaticClass()) ? AnimInstance : nullptr;
}

void USOTS_ParkourComponent::PushParkourDataToAnimInstance(const FSOTS_ParkourResult& Result)
{
	if (UObject* AnimObj = GetParkourAnimInstanceObject())
	{
		ISOTS_ParkourABPInterface::Execute_OnParkourResultUpdated(AnimObj, Result);
		ISOTS_ParkourABPInterface::Execute_SetParkourStateTag(AnimObj, GetParkourStateTag());
		ISOTS_ParkourABPInterface::Execute_SetParkourActionTag(AnimObj, GetParkourActionTag());
		ISOTS_ParkourABPInterface::Execute_SetClimbStyleTag(AnimObj, GetClimbStyleTag());
		ISOTS_ParkourABPInterface::Execute_SetLeftHandLedgeLocation(AnimObj, Result.LeftHandBaseLocation);
		ISOTS_ParkourABPInterface::Execute_SetRightHandLedgeLocation(AnimObj, Result.RightHandBaseLocation);
		ISOTS_ParkourABPInterface::Execute_SetLeftHandLedgeRotation(AnimObj, Result.LeftHandBaseRotation);
		ISOTS_ParkourABPInterface::Execute_SetRightHandLedgeRotation(AnimObj, Result.RightHandBaseRotation);
		ISOTS_ParkourABPInterface::Execute_SetLeftFootLocation(AnimObj, GetLeftFootLocation());
		ISOTS_ParkourABPInterface::Execute_SetRightFootLocation(AnimObj, GetRightFootLocation());
		ISOTS_ParkourABPInterface::Execute_SetLeftClimbIK(AnimObj, Result.bEnableLeftClimbIK);
		ISOTS_ParkourABPInterface::Execute_SetRightClimbIK(AnimObj, Result.bEnableRightClimbIK);
	}
}

void USOTS_ParkourComponent::PushParkourStateToAnimInstance(ESOTS_ParkourState NewState)
{
	if (UObject* AnimObj = GetParkourAnimInstanceObject())
	{
		ISOTS_ParkourABPInterface::Execute_OnParkourStateChanged(AnimObj, NewState);
		ISOTS_ParkourABPInterface::Execute_SetParkourStateTag(AnimObj, GetParkourStateTag());
	}
}

void USOTS_ParkourComponent::TryPerformParkour()
{
	if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Verbose, TEXT("USOTS_ParkourComponent::TryPerformParkour Owner=%s State=%d"), *GetNameSafe(GetOwner()), static_cast<int32>(ParkourState));
	}

	TraceStats.TracesThisAction = 0;

	if (ParkourState != ESOTS_ParkourState::Idle)
	{
		if (bEnableDebugLogging)
		{
			UE_LOG(LogSOTS_Parkour, Verbose, TEXT("USOTS_ParkourComponent: TryPerformParkour ignored; not Idle (State=%d)."), static_cast<int32>(ParkourState));
		}
		return;
	}

	FSOTS_ParkourResult DetectionResult;
	if (!TryDetectParkourOnce(DetectionResult))
	{
		if (bEnableDebugLogging)
		{
			UE_LOG(LogSOTS_Parkour, Verbose, TEXT("USOTS_ParkourComponent: No parkour opportunity found in TryPerformParkour."));
		}
		return;
	}

	SetParkourState(ESOTS_ParkourState::Entering);
	FinalizeResultTags(DetectionResult);
	LastResult = DetectionResult;
	BroadcastParkourActionStarted(LastResult.Action);

	if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Verbose, TEXT("USOTS_ParkourComponent::TryPerformParkour starting Action=%d HasResult=%d IsValid=%d"), static_cast<int32>(LastResult.Action), LastResult.bHasResult, LastResult.bIsValid);
	}

	NotifyParkourResultUpdated(LastResult);

	if (bEnableDebugLogging)
	{
		UObject* WorldContext = GetOwner() ? static_cast<UObject*>(GetOwner()) : static_cast<UObject*>(this);
		USOTS_ParkourDebugLibrary::LogParkourResult(WorldContext, LastResult, TEXT("TryPerformParkour"));
	}

	SetParkourState(ESOTS_ParkourState::Active);
	ExecuteCurrentParkour();
}

bool USOTS_ParkourComponent::TryDetectParkourOnce(FSOTS_ParkourResult& OutResult)
{
	TraceStats.TracesThisAction = 0;
	const FVector ResolvedInputDirection = GetResolvedInputDirection();
	PerformParkourDetection(ResolvedInputDirection, false);
	OutResult = LastResult;

	if (!OutResult.bHasResult && !OutResult.bIsValid)
	{
		OutResult.Reset();
		return false;
	}

	NotifyParkourResultUpdated(LastResult);

	if (bEnableDebugLogging)
	{
		UObject* WorldContext = GetOwner() ? static_cast<UObject*>(GetOwner()) : static_cast<UObject*>(this);
		USOTS_ParkourDebugLibrary::LogParkourResult(WorldContext, LastResult, TEXT("TryDetectParkourOnce"));
	}

	return true;
}

bool USOTS_ParkourComponent::DebugRunDetectionBypass(FSOTS_ParkourResult& OutResult)
{
	// Skip idle/speed/camera gates by treating this as a continuation run.
	const FVector ResolvedInputDirection = GetResolvedInputDirection();
	PerformParkourDetection(ResolvedInputDirection, true /*bIsContinuation*/);
	OutResult = LastResult;

	NotifyParkourResultUpdated(LastResult);

	return OutResult.bHasResult || OutResult.bIsValid;
}

void USOTS_ParkourComponent::ResetMovementInputInternal(bool bUpdateTags)
{
	bHasInputDirection = false;
	InputDirection = FVector::ZeroVector;
	bHasSpeed2DInput = false;
	MovementSpeed2DInput = 0.0f;

	const FGameplayTag NoDirectionTag = FGameplayTag::RequestGameplayTag(TEXT("Parkour.Direction.NoDirection"), false);
	LastResult.DirectionTag = NoDirectionTag;

	if (bUpdateTags)
	{
		UpdateGameplayTagsFromResult(LastResult);
	}
}

void USOTS_ParkourComponent::ResetMovementInput()
{
	ResetMovementInputInternal(true);
}

void USOTS_ParkourComponent::ResetParkourResultInternal(bool bUpdateTags)
{
	LastResult.Reset();
	if (bUpdateTags)
	{
		UpdateGameplayTagsFromResult(LastResult);
		PushStatsToWidget();
	}
}

void USOTS_ParkourComponent::ResetParkourResultState()
{
	ResetParkourResultInternal(true);
}

void USOTS_ParkourComponent::StopActiveMontage(float BlendOutTime) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return;
	}

	const USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(BlendOutTime);
	}
}

void USOTS_ParkourComponent::AbortParkour(const FString& Reason)
{
	if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("USOTS_ParkourComponent: AbortParkour Reason=%s"), *Reason);
	}

	BroadcastParkourActionFinished(false);

	bIsLedgeShimmyActive = false;
	bCornerMoveInProgress = false;
	ClimbTraceCooldownTime = 0.0f;

	ResetMovementInputInternal(false);
	ResetParkourResultInternal(true);

	if (CachedMovementComponent)
	{
		CachedMovementComponent->StopMovementImmediately();
	}

	SetParkourLogicalState(ESOTS_ParkourLogicalState::NotBusy);
	SetParkourState(ESOTS_ParkourState::Idle);
}

ESOTS_ParkourState USOTS_ParkourComponent::GetParkourState() const
{
	return ParkourState;
}

void USOTS_ParkourComponent::OnOwnerLanded(const FHitResult& Hit)
{
	(void)Hit;

	// CGF parity: restore manual/auto climb gates and stop any in-flight montage.
	bCanAutoClimb = true;
	bCanManualClimb = true;
	StopActiveMontage(0.2f);

	ResetMovementInputInternal(true);
	ClimbTraceCooldownTime = 0.0f;

	if (ParkourLogicalState != ESOTS_ParkourLogicalState::NotBusy)
	{
		SetParkourLogicalState(ESOTS_ParkourLogicalState::NotBusy);
	}

	if (ParkourState != ESOTS_ParkourState::Idle)
	{
		SetParkourState(ESOTS_ParkourState::Idle);
	}
}

void USOTS_ParkourComponent::OnOwnerMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	(void)PrevMovementMode;
	(void)PreviousCustomMode;

	if (!Character || Character != CachedOwnerCharacter.Get())
	{
		return;
	}

	// Abort parkour when movement mode changes (e.g., launch/fall/physics) to mirror BP safety net.
	if (ParkourState != ESOTS_ParkourState::Idle || ParkourLogicalState != ESOTS_ParkourLogicalState::NotBusy)
	{
		StopActiveMontage(0.1f);
		ResetMovementInputInternal(true);
		ResetParkourResultInternal(true);
		bIsLedgeShimmyActive = false;
		bCornerMoveInProgress = false;
		ClimbTraceCooldownTime = 0.0f;
		ParkourStateLingerTime = 0.0f;
		SetParkourLogicalState(ESOTS_ParkourLogicalState::NotBusy);
		SetParkourState(ESOTS_ParkourState::Idle);
	}
}

void USOTS_ParkourComponent::PerformParkourDetection(const FVector& InInputDirection, bool bIsContinuation)
{
	const bool bForceDebugLedgeTrace = bDebugForceLedgeTrace;
	const bool bTreatAsContinuation = bIsContinuation || bForceDebugLedgeTrace;

	LastResult.Reset();
	MaybeResetTraceStats();

	if (!GetWorld())
	{
		return;
	}

	// CGF parity: throttle climb tracing using a short randomized cooldown while climbing.
	if (ParkourLogicalState == ESOTS_ParkourLogicalState::Climb)
	{
		if (!bForceDebugLedgeTrace && ClimbTraceCooldownTime > 0.0f)
		{
			return;
		}

		if (!bForceDebugLedgeTrace)
		{
			ClimbTraceCooldownTime = FMath::RandRange(ClimbTraceCooldownMin, ClimbTraceCooldownMax);
		}
	}

	if (!bTreatAsContinuation && !CanAttemptParkourInternal())
	{
		return;
	}

	ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return;
	}

	const float Speed2DLod = GetResolvedSpeed2D();

	float CameraDistance = 0.0f;
	bool bHasCamera = false;
	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		if (const APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
		{
			CameraDistance = FVector::Dist(CameraManager->GetCameraLocation(), Character->GetActorLocation());
			bHasCamera = true;
		}
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	const float MinSpeedCutoff = Config ? Config->MinSpeedForDetection : MinSpeedForDetection;
	const float MaxCameraDistanceCutoff = Config ? Config->MaxCameraDistanceForDetection : MaxCameraDistanceForDetection;

	if (!bTreatAsContinuation)
	{
		const bool bBelowSpeedCutoff = Speed2DLod < MinSpeedCutoff;
		const bool bBeyondCameraCutoff = bHasCamera && CameraDistance > MaxCameraDistanceCutoff;
		if (bBelowSpeedCutoff || bBeyondCameraCutoff)
		{
			LastResult.bHasResult = false;
			LastResult.bIsValid = false;
			return;
		}
	}

	// Build the forward capsule trace (NotBusy vs. Climb) using existing math.
	FSOTS_ParkourCapsuleTraceSettings TraceSettings;
	const bool bUseClimbTrace = (ParkourState == ESOTS_ParkourState::Active || bTreatAsContinuation) && ClimbArrowComponent;

	if (bUseClimbTrace)
	{
		const FSOTS_ParkourTraceProfile* ClimbProfile = FindTraceProfileForAction(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.ClimbForward"), false));
		FSOTS_ParkourTraceProfile DefaultProfile;
		DefaultProfile.CapsuleRadius = 20.0f;
		DefaultProfile.CapsuleHalfHeight = 20.0f;
		DefaultProfile.ForwardDistanceMax = 60.0f;
		DefaultProfile.StartZOffset = 0.0f;

		if (!BuildClimbForwardTrace(ClimbProfile ? *ClimbProfile : DefaultProfile, TraceSettings))
		{
			return;
		}
	}
	else
	{
		const FVector Velocity = GetResolvedInputDirection();
		const bool bIsFalling = GetResolvedIsInAir();

		const FSOTS_ParkourTraceProfile* GroundProfile = FindTraceProfileForAction(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.GroundForward"), false));
		FSOTS_ParkourTraceProfile DefaultProfile;
		DefaultProfile.CapsuleRadius = 40.0f;
		DefaultProfile.CapsuleHalfHeight = 120.0f;
		DefaultProfile.ForwardDistanceMin = 50.0f;
		DefaultProfile.ForwardDistanceMax = 200.0f;
		DefaultProfile.StartZOffset = 75.0f;
		DefaultProfile.VerticalOffsetDown = 15.0f;
		DefaultProfile.VerticalOffsetUp = 70.0f;
		DefaultProfile.MinLedgeHeight = 25.0f;
		DefaultProfile.MaxLedgeHeight = 100.0f;

		if (!BuildGroundForwardTrace(GroundProfile ? *GroundProfile : DefaultProfile, InInputDirection, Character->GetActorLocation(), Velocity, bIsFalling, TraceSettings))
		{
			return;
		}
	}

	// Run the sweep.
	FHitResult Hit;
	const ECollisionChannel TraceChannel = ParkourTraceChannel;
	if (!RunCapsuleTrace(TraceSettings, Hit, TraceChannel, TEXT("SOTS_Parkour_FirstCapsule")))
	{
		return;
	}

	// Height classification using config thresholds.
	const float CharacterHalfHeight = Character->GetSimpleCollisionHalfHeight();
	const FVector CharacterLocation = Character->GetActorLocation();
	const float CharacterFeetZ = CharacterLocation.Z - CharacterHalfHeight;
	float HeightDelta = Hit.ImpactPoint.Z - CharacterFeetZ;
	float XYDistance = FVector::Dist2D(CharacterLocation, Hit.ImpactPoint);
	float WallDepth = FVector::Dist(TraceSettings.Start, Hit.ImpactPoint);

	// Optional wall grid refinement to mirror CGF wall-detect probes; skip if trace budget is exhausted.
	{
		FHitResult GridHit;
		float GridHeightDelta = HeightDelta;
		float GridXYDistance = XYDistance;
		float GridWallDepth = WallDepth;
		if (CanConsumeTrace(2) && RunWallGridProbes(Hit, CharacterHalfHeight, GridHit, GridHeightDelta, GridXYDistance, GridWallDepth))
		{
			Hit = GridHit;
			HeightDelta = GridHeightDelta;
			XYDistance = GridXYDistance;
			WallDepth = GridWallDepth;
		}
	}

	const float Speed2DForClassification = GetResolvedSpeed2D();
	const float MaxSpeed = CachedMovementComponent ? CachedMovementComponent->GetMaxSpeed() : 600.0f;

	FSOTS_ParkourResult DetectionResult;
	if (bForceDebugLedgeTrace)
	{
		// Force a climb-style result even if normal selection would pick a traversal.
		DetectionResult.Reset();
		DetectionResult.bHasResult = true;
		DetectionResult.bIsValid = true;
		DetectionResult.Action = ESOTS_ParkourAction::AirHang; // climb/ledge branch
		DetectionResult.ClimbStyle = ESOTS_ClimbStyle::Braced;
		DetectionResult.ResultType = ESOTS_ParkourResultType::Mantle;
		DetectionResult.WorldLocation = Hit.ImpactPoint;
		DetectionResult.WorldNormal = Hit.ImpactNormal;
		DetectionResult.TargetLocation = Hit.ImpactPoint;
		DetectionResult.SurfaceNormal = Hit.ImpactNormal;
		DetectionResult.HeightDelta = HeightDelta;
		DetectionResult.Hit = Hit;
	}
	else if (!SelectParkourActionFromHit(Hit, HeightDelta, XYDistance, WallDepth, Speed2DForClassification, MaxSpeed, CharacterHalfHeight, DetectionResult))
	{
		LastResult.Reset();
		SetParkourLogicalState(GetLogicalStateForAction(LastResult.Action));
		return;
	}

	FinalizeResultTags(DetectionResult);
	LastResult = DetectionResult;
	SetParkourLogicalState(GetLogicalStateForAction(LastResult.Action));
	return;
}

bool USOTS_ParkourComponent::PredictJumpLanding(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& OutResult) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return false;
	}

	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return false;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	if (!Config)
	{
		return false;
	}

	const FVector WallNormal = FirstHit.ImpactNormal.IsNearlyZero() ? FVector::ForwardVector : FirstHit.ImpactNormal.GetSafeNormal();
	const FVector Forward = WallNormal.IsNearlyZero() ? Character->GetActorForwardVector() : -WallNormal;

	// First, probe forward at ledge height to find a low obstacle/landing candidate (CGF PredictJump forward sphere).
	const FVector ForwardStart = FirstHit.ImpactPoint + FVector(0.0f, 0.0f, CapsuleHalfHeight);
	const FVector ForwardEnd = ForwardStart + Forward * Config->PredictJumpForwardDistance;
	FHitResult ForwardHit;
	const bool bForwardHit = TryConsumeTraceSlot() && RunSphereTraceWithOmniTrace(WorldContext, ForwardStart, ForwardEnd, Config->PredictJumpProbeRadius, ParkourTraceChannel, Character, ForwardHit);

	const bool bDrawPredict = Config ? Config->bDrawForwardProbeDebug : true;
	DrawDebugLineIfEnabled(ForwardStart, ForwardEnd, bDrawPredict, bForwardHit ? FColor::Green : FColor::Red);
	if (!bForwardHit || !ForwardHit.IsValidBlockingHit() || ForwardHit.bStartPenetrating)
	{
		return false;
	}

	// Then drop a sphere straight down from the forward hit to locate a safe landing.
	const FVector DownStart = ForwardHit.ImpactPoint + FVector(0.0f, 0.0f, CapsuleHalfHeight);
	const FVector DownEnd = DownStart - FVector(0.0f, 0.0f, Config->PredictJumpProbeDepth);

	FHitResult LandingHit;
	const bool bDownHit = TryConsumeTraceSlot() && RunSphereTraceWithOmniTrace(WorldContext, DownStart, DownEnd, Config->PredictJumpProbeRadius, ParkourTraceChannel, Character, LandingHit);
	DrawDebugLineIfEnabled(DownStart, DownEnd, bDrawPredict, bDownHit ? FColor::Green : FColor::Red);
	if (bDownHit && LandingHit.IsValidBlockingHit() && !LandingHit.bStartPenetrating)
	{
		OutResult.Reset();
		OutResult.bHasResult = true;
		OutResult.bIsValid = true;
		OutResult.Action = ESOTS_ParkourAction::PredictJump;
		OutResult.ClimbStyle = ESOTS_ClimbStyle::Braced;
		OutResult.ResultType = ESOTS_ParkourResultType::Mantle;
		OutResult.WorldLocation = LandingHit.ImpactPoint;
		OutResult.WorldNormal = LandingHit.ImpactNormal;
		OutResult.TargetLocation = LandingHit.ImpactPoint;
		OutResult.SurfaceNormal = LandingHit.ImpactNormal;
		const float CharacterFeetZ = Character->GetActorLocation().Z - CapsuleHalfHeight;
		OutResult.HeightDelta = LandingHit.ImpactPoint.Z - CharacterFeetZ;
		OutResult.Hit = LandingHit;
		return true;
	}

	return false;
}

bool USOTS_ParkourComponent::SelectParkourActionFromHit(
	const FHitResult& Hit,
	float HeightDelta,
	float XYDistance,
	float WallDepth,
	float Speed2DForClassification,
	float MaxSpeed,
	float CharacterHalfHeight,
	FSOTS_ParkourResult& OutResult) const
{
	OutResult.Reset();

	const USOTS_ParkourConfig* Config = GetConfig();
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Config || !Character)
	{
		return false;
	}

	if (!Hit.IsValidBlockingHit())
	{
		return false;
	}

	const float CharacterFeetZ = Character->GetActorLocation().Z - CharacterHalfHeight;

	// Blueprint parity: branch ordering mirrors Check Normal Parkour - braced TicTac first when far enough, then free-hang distance, then close free-hang climb.
	const auto MakeFreeHangResult = [&](const FHitResult& HangHit, const TCHAR* ActionTagName, FSOTS_ParkourResult& Out) -> void
	{
		Out.Reset();
		Out.bHasResult = true;
		Out.bIsValid = true;
		Out.Action = ESOTS_ParkourAction::AirHang;
		Out.ClimbStyle = ESOTS_ClimbStyle::FreeHang;
		Out.ResultType = ESOTS_ParkourResultType::Mantle;
		Out.ActionTag = ResolveParkourTagByName(ActionTagName);
		Out.ClimbStyleTag = ResolveParkourTagByName(TEXT("Parkour.ClimbStyle.FreeHang"));
		Out.WorldLocation = HangHit.ImpactPoint;
		Out.WorldNormal = HangHit.ImpactNormal;
		Out.TargetLocation = HangHit.ImpactPoint;
		Out.SurfaceNormal = HangHit.ImpactNormal;
		Out.HeightDelta = HangHit.ImpactPoint.Z - CharacterFeetZ;
		Out.Hit = HangHit;
	};

	const auto TryBracedTicTac = [&](FSOTS_ParkourResult& Out) -> bool
	{
		FSOTS_ParkourResult TicTacResult;
		TicTacResult.Reset();
		if (TryTicTacSideProbes(Hit, CharacterHalfHeight, TicTacResult))
		{
			TicTacResult.bHasResult = true;
			TicTacResult.bIsValid = true;
			Out = TicTacResult;
			return true;
		}

		return false;
	};

	const auto TryFreeHangDistance = [&](FSOTS_ParkourResult& Out) -> bool
	{
		FSOTS_ParkourResult DropHangResult;
		DropHangResult.Reset();
		DropHangResult.Action = ESOTS_ParkourAction::AirHang;
		DropHangResult.ClimbStyle = ESOTS_ClimbStyle::FreeHang;
		DropHangResult.ResultType = ESOTS_ParkourResultType::Mantle;

		if (RunDropHangProbes(Hit, CharacterHalfHeight, DropHangResult))
		{
			DropHangResult.bHasResult = true;
			DropHangResult.bIsValid = true;
			DropHangResult.HeightDelta = DropHangResult.WorldLocation.Z - CharacterFeetZ;
			DropHangResult.ActionTag = ResolveParkourTagByName(TEXT("Parkour.Action.DistanceIdleToFreeHang"));
			DropHangResult.ClimbStyleTag = ResolveParkourTagByName(TEXT("Parkour.ClimbStyle.FreeHang"));
			Out = DropHangResult;
			return true;
		}

		return false;
	};

	const auto TryClimbFallbacks = [&](FSOTS_ParkourResult& Out) -> bool
	{
		const bool bBracedDistance = XYDistance > Config->DistanceFromLedgeXYCutoff; // 122 in config (matches BP braced gate)
		const bool bFreeHangDistance = XYDistance > 100.0f; // BP non-braced distance gate

		if (!bTriedTicTacDropHangThisFrame)
		{
			bTriedTicTacDropHangThisFrame = true;

			if (bBracedDistance && TryBracedTicTac(Out))
			{
				return true;
			}

			if (bFreeHangDistance && TryFreeHangDistance(Out))
			{
				return true;
			}
		}

		if (bBracedDistance)
		{
			FSOTS_ParkourResult BracedClimb;
			BracedClimb.Reset();
			BracedClimb.bHasResult = true;
			BracedClimb.bIsValid = true;
			BracedClimb.Action = ESOTS_ParkourAction::AirHang;
			BracedClimb.ClimbStyle = ESOTS_ClimbStyle::Braced;
			BracedClimb.ResultType = ESOTS_ParkourResultType::Mantle;
			BracedClimb.ActionTag = ResolveParkourTagByName(TEXT("Parkour.Action.Climb"));
			BracedClimb.ClimbStyleTag = ResolveParkourTagByName(TEXT("Parkour.ClimbStyle.Braced"));
			BracedClimb.WorldLocation = Hit.ImpactPoint;
			BracedClimb.WorldNormal = Hit.ImpactNormal;
			BracedClimb.TargetLocation = Hit.ImpactPoint;
			BracedClimb.SurfaceNormal = Hit.ImpactNormal;
			BracedClimb.HeightDelta = HeightDelta;
			BracedClimb.Hit = Hit;
			Out = BracedClimb;
			return true;
		}

		// Close non-braced path: default to FreeHang climb tag.
		MakeFreeHangResult(Hit, TEXT("Parkour.Action.FreeHangClimb"), Out);
		return true;
	};

	const auto TryTicTacOrDropHang = [&](FSOTS_ParkourResult& Out) -> bool
	{
		if (bTriedTicTacDropHangThisFrame)
		{
			return false;
		}

		bTriedTicTacDropHangThisFrame = true;

		const bool bBracedDistance = XYDistance > Config->DistanceFromLedgeXYCutoff;
		const bool bFreeHangDistance = XYDistance > 100.0f;

		if (bBracedDistance && TryBracedTicTac(Out))
		{
			return true;
		}

		if (bFreeHangDistance && TryFreeHangDistance(Out))
		{
			return true;
		}

		return false;
	};

	const auto TryLedgeHopOrCorner = [&](FSOTS_ParkourResult& Out) -> bool
	{
		if (bTriedLedgeCornerThisFrame)
		{
			return false;
		}

		bTriedLedgeCornerThisFrame = true;

		FSOTS_ParkourResult AltResult;
		AltResult.Reset();

		if (TryDetectLedgeMoveAndHops(Hit, CharacterHalfHeight, AltResult))
		{
			Out = AltResult;
			return true;
		}

		AltResult.Reset();
		if (TryDetectCornerMove(Hit, CharacterHalfHeight, AltResult))
		{
			Out = AltResult;
			return true;
		}

		return false;
	};

	const float SpeedRatio = (MaxSpeed > KINDA_SMALL_NUMBER) ? Speed2DForClassification / MaxSpeed : 0.0f;
	const bool bVaultHeight = HeightDelta >= Config->VaultMinHeight && HeightDelta <= Config->VaultMaxHeight;
	const bool bMantleHeight = HeightDelta >= Config->MantleMinHeight && HeightDelta <= Config->MantleMaxHeight;
	const bool bDistanceMantle = XYDistance > Config->DistanceFromLedgeXYCutoff;
	const bool bDepthOkForVault = (WallDepth >= Config->VaultMinWallDepth) && (WallDepth <= Config->VaultMaxWallDepth);
	const bool bDepthOkForMantle = (WallDepth >= Config->MantleMinWallDepth) && (WallDepth <= Config->MantleMaxWallDepth);
	const bool bFastEnoughForVault = Speed2DForClassification >= Config->VaultMinSpeed || SpeedRatio > 0.5f;
	const bool bWallDepthInRange = (WallDepth >= Config->MantleMinWallDepth) && (WallDepth <= Config->VaultMaxWallDepth);

	// Condition-band table mirrors the BP Check Normal Parkour ordering to keep trace cost predictable.
	struct FSelectionBand
	{
		ESOTS_ParkourConditionType ConditionType;
		FString Label;
		TFunction<bool()> Condition;
		TFunction<bool(FSOTS_ParkourResult&)> Execute;
		bool bStopOnFailure = true;
	};

	const TArray<FSelectionBand> Bands = {
		{
			ESOTS_ParkourConditionType::WallHeight,
			TEXT("Drop"),
			[&]() { return HeightDelta < 0.0f; },
			[&](FSOTS_ParkourResult& Out) {
				const float DropHeight = FMath::Abs(HeightDelta);
				if (DropHeight > Config->MaxSafeDropHeight)
				{
					return false;
				}

				Out.bHasResult = true;
				Out.bIsValid = true;
				Out.Action = ESOTS_ParkourAction::Drop;
				Out.ResultType = ESOTS_ParkourResultType::Drop;
				Out.WorldLocation = Hit.ImpactPoint;
				Out.WorldNormal = Hit.ImpactNormal;
				Out.TargetLocation = Hit.ImpactPoint;
				Out.SurfaceNormal = Hit.ImpactNormal;
				Out.HeightDelta = -DropHeight;
				Out.Hit = Hit;
				return true;
			},
			true
		},
		{
			ESOTS_ParkourConditionType::WallDepth,
			TEXT("WallDepthOutOfRange"),
			[&]() { return !bWallDepthInRange; },
			[&](FSOTS_ParkourResult& Out) {
				if (PredictJumpLanding(Hit, CharacterHalfHeight, Out))
				{
					return true;
				}

				TriggerAutoClimbJump(TEXT("WallDepthOutOfRange"));
				return false;
			},
			true
		},
		{
			ESOTS_ParkourConditionType::WallHeight,
			TEXT("LowObstacle"),
			[&]() { return !bVaultHeight && !bMantleHeight; },
			[&](FSOTS_ParkourResult& Out) {
				if (TryLedgeHopOrCorner(Out))
				{
					return true;
				}

				if (PredictJumpLanding(Hit, CharacterHalfHeight, Out))
				{
					return true;
				}

				return TryClimbFallbacks(Out);
			},
			true
		},
		{
			ESOTS_ParkourConditionType::WallDepth,
			TEXT("TicTacOrDropHang"),
			[&]() { return bWallDepthInRange; },
			[&](FSOTS_ParkourResult& Out) {
				return TryTicTacOrDropHang(Out);
			},
			false
		},
		{
			ESOTS_ParkourConditionType::VaultHeight,
			TEXT("Vault"),
			[&]() { return bVaultHeight && bDepthOkForVault && bFastEnoughForVault; },
			[&](FSOTS_ParkourResult& Out) {
				ESOTS_ParkourAction ChosenVault = ESOTS_ParkourAction::Vault;
				if (HeightDelta <= Config->VaultThinHeightMax)
				{
					ChosenVault = ESOTS_ParkourAction::ThinVault;
				}
				else if (HeightDelta >= Config->VaultHighHeightMin)
				{
					ChosenVault = ESOTS_ParkourAction::HighVault;
				}

				FSOTS_ParkourResult VaultResult;
				VaultResult.Reset();
				VaultResult.bHasResult = true;
				VaultResult.bIsValid = true;
				VaultResult.Action = ChosenVault;
				VaultResult.ResultType = ESOTS_ParkourResultType::Mantle;
				VaultResult.HeightDelta = HeightDelta;

				if (!ConfirmSurfaceForAction(Hit, CharacterHalfHeight, ESOTS_ParkourAction::Vault, VaultResult))
				{
					TriggerAutoClimbJump(TEXT("VaultConfirmFailed"));
					return false;
				}

				Out = VaultResult;
				return true;
			},
			true
		},
		{
			ESOTS_ParkourConditionType::WallHeight,
			TEXT("Mantle"),
			[&]() { return bMantleHeight && bDepthOkForMantle; },
			[&](FSOTS_ParkourResult& Out) {
				FSOTS_ParkourResult MantleResult;
				MantleResult.Reset();
				MantleResult.bHasResult = true;
				MantleResult.bIsValid = true;
				MantleResult.Action = bDistanceMantle ? ESOTS_ParkourAction::DistanceMantle : ESOTS_ParkourAction::Mantle;
				MantleResult.ResultType = ESOTS_ParkourResultType::Mantle;
				MantleResult.HeightDelta = HeightDelta;

				if (!ConfirmSurfaceForAction(Hit, CharacterHalfHeight, ESOTS_ParkourAction::Mantle, MantleResult))
				{
					TriggerAutoClimbJump(TEXT("MantleConfirmFailed"));
					return false;
				}

				Out = MantleResult;
				return true;
			},
			true
		},
		{
			ESOTS_ParkourConditionType::Velocity,
			TEXT("HopCornerFallback"),
			[&]() { return true; },
			[&](FSOTS_ParkourResult& Out) {
				if (TryLedgeHopOrCorner(Out))
				{
					return true;
				}

				if (PredictJumpLanding(Hit, CharacterHalfHeight, Out))
				{
					return true;
				}

				if (TryClimbFallbacks(Out))
				{
					return true;
				}

				TriggerAutoClimbJump(TEXT("NoValidParkourAction"));
				return false;
			},
			true
		}
	};

	for (const FSelectionBand& Band : Bands)
	{
		(void)Band.ConditionType; // ConditionType carried for parity docs / potential table-driven ordering.

		if (!Band.Condition())
		{
			continue;
		}

		const bool bSelected = Band.Execute(OutResult);
		if (bSelected)
		{
			return true;
		}

		if (Band.bStopOnFailure)
		{
			return false;
		}
	}

	return false;
}

void USOTS_ParkourComponent::SetParkourState(ESOTS_ParkourState NewState)
{
	if (ParkourState == NewState)
	{
		return;
	}

	const ESOTS_ParkourState OldState = ParkourState;

	if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Verbose, TEXT("USOTS_ParkourComponent: State change %d -> %d"), static_cast<int32>(ParkourState), static_cast<int32>(NewState));
	}

	ParkourState = NewState;
	NotifyParkourStateChanged(OldState, ParkourState);
	UpdateStateMachine(0.0f);
}

bool USOTS_ParkourComponent::CanAttemptParkourInternal() const
{
	if (!CachedOwnerCharacter.IsValid() || !CachedMovementComponent)
	{
		return false;
	}

	if (CachedMovementComponent->IsFlying() || CachedMovementComponent->IsSwimming())
	{
		return false;
	}

	// Only allow attempts when idle to mirror the "NotBusy" gate.
	return ParkourState == ESOTS_ParkourState::Idle;
}

void USOTS_ParkourComponent::ResetTraceStatsForFrame() const
{
	LastTraceResetFrame = GFrameCounter;
	TraceStats.TracesThisFrame = 0;
	TraceStats.TracesThisAction = 0;
	bTriedLedgeCornerThisFrame = false;
	bTriedTicTacDropHangThisFrame = false;
}

void USOTS_ParkourComponent::MaybeResetTraceStats() const
{
	const uint64 CurrentFrame = GFrameCounter;
	if (LastTraceResetFrame != CurrentFrame)
	{
		ResetTraceStatsForFrame();
	}
}

bool USOTS_ParkourComponent::CanConsumeTrace(int32 NumToConsume) const
{
	MaybeResetTraceStats();
	const USOTS_ParkourConfig* Config = GetConfig();
	const int32 MaxPerFrame = Config ? Config->MaxTracesPerFrame : MaxTracesPerFrame;
	if (MaxPerFrame <= 0)
	{
		return true;
	}
	return (TraceStats.TracesThisFrame + NumToConsume) <= MaxPerFrame;
}

void USOTS_ParkourComponent::ConsumeTrace(int32 NumToConsume) const
{
	MaybeResetTraceStats();
	TraceStats.TracesThisFrame += NumToConsume;
	TraceStats.TracesThisAction += NumToConsume;
	TotalTracesSinceBeginPlay += NumToConsume;
	MaxTracesInSingleFrame = FMath::Max(MaxTracesInSingleFrame, TraceStats.TracesThisFrame);
}

bool USOTS_ParkourComponent::TryConsumeTraceSlot() const
{
	if (!CanConsumeTrace())
	{
		return false;
	}

	ConsumeTrace();
	return true;
}

void USOTS_ParkourComponent::LogTraceResult(const TCHAR* Label, const FVector& Start, const FVector& End, float Radius, float HalfHeight, const FHitResult& Hit) const
{
	if (!(bEnableDebugLogging && bEnableTraceDebug))
	{
		return;
	}

	const bool bHit = Hit.IsValidBlockingHit();
	const FVector ImpactPoint = bHit ? Hit.ImpactPoint : FVector::ZeroVector;
	const FVector ImpactNormal = bHit ? Hit.ImpactNormal : FVector::ZeroVector;

	UE_LOG(LogSOTS_Parkour, Verbose, TEXT("Trace[%s] Start=%s End=%s Radius=%.1f HalfHeight=%.1f Hit=%d Impact=%s Normal=%s"),
		Label ? Label : TEXT(""),
		*Start.ToCompactString(),
		*End.ToCompactString(),
		Radius,
		HalfHeight,
		bHit ? 1 : 0,
		*ImpactPoint.ToCompactString(),
		*ImpactNormal.ToCompactString());
}

void USOTS_ParkourComponent::BroadcastParkourActionStarted(ESOTS_ParkourAction Action)
{
	if (Action == ESOTS_ParkourAction::None)
	{
		CurrentActionForEvents = ESOTS_ParkourAction::None;
		return;
	}

	CurrentActionForEvents = Action;
	OnParkourActionStarted.Broadcast(Action);
}

void USOTS_ParkourComponent::BroadcastParkourActionFinished(bool bSuccess)
{
	if (CurrentActionForEvents == ESOTS_ParkourAction::None)
	{
		return;
	}

	OnParkourActionFinished.Broadcast(CurrentActionForEvents, bSuccess);
	CurrentActionForEvents = ESOTS_ParkourAction::None;
}

void USOTS_ParkourComponent::DrawDebugLineIfEnabled(const FVector& Start, const FVector& End, bool bFeatureEnabled, const FColor& Color) const
{
	if (!bEnableTraceDebug || !bFeatureEnabled)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	const float Lifetime = Config ? Config->DebugLineLifetime : 0.1f;

	DrawDebugLine(World, Start, End, Color, false, Lifetime, 0, 1.5f);
}

void USOTS_ParkourComponent::DrawDebugCapsuleIfEnabled(const FVector& Center, float HalfHeight, float Radius, const FQuat& Rotation, bool bFeatureEnabled, const FColor& Color) const
{
	if (!bEnableTraceDebug || !bFeatureEnabled)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	const float Lifetime = Config ? Config->DebugShapeLifetime : 0.1f;

	DrawDebugCapsule(World, Center, HalfHeight, Radius, Rotation, Color, false, Lifetime, 0, 1.0f);
}

void USOTS_ParkourComponent::DrawDebugSphereIfEnabled(const FVector& Center, float Radius, bool bFeatureEnabled, const FColor& Color) const
{
	if (!bEnableTraceDebug || !bFeatureEnabled)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	const float Lifetime = Config ? Config->DebugShapeLifetime : 0.1f;

	DrawDebugSphere(World, Center, Radius, 12, Color, false, Lifetime, 0, 1.0f);
}

void USOTS_ParkourComponent::DrawDebugPointIfEnabled(const FVector& Location, float Size, bool bFeatureEnabled, const FColor& Color) const
{
	if (!bEnableTraceDebug || !bFeatureEnabled)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	const float Lifetime = Config ? Config->DebugShapeLifetime : 0.1f;

	DrawDebugPoint(World, Location, Size, Color, false, Lifetime);
}

const FSOTS_ParkourTraceProfile* USOTS_ParkourComponent::FindTraceProfileForAction(const FGameplayTag& ActionTag) const
{
	if (!ActionTag.IsValid())
	{
		return nullptr;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	if (!Config)
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("SOTS_Parkour: No ParkourConfig set; cannot resolve trace profile for %s"), *ActionTag.ToString());
		return nullptr;
	}

	if (const FSOTS_ParkourTraceProfile* Profile = Config->ActionTraceProfiles.Find(ActionTag))
	{
		return Profile;
	}

	UE_LOG(LogSOTS_Parkour, Warning, TEXT("SOTS_Parkour: Missing trace profile for action tag %s"), *ActionTag.ToString());
	return nullptr;
}

bool USOTS_ParkourComponent::BuildGroundForwardTrace(
	const FSOTS_ParkourTraceProfile& Profile,
	const FVector& InInputDirection,
	const FVector& ActorLocation,
	const FVector& Velocity,
	bool bIsFalling,
	FSOTS_ParkourCapsuleTraceSettings& OutSettings) const
{
	// CGF ParkourComp "First Capsule Trace" (NotBusy) port: forward capsule driven by speed.
	FVector Direction2D = InInputDirection;
	Direction2D.Z = 0.0f;
	if (Direction2D.IsNearlyZero())
	{
		Direction2D = FVector(Velocity.X, Velocity.Y, 0.0f);
	}

	if (!Direction2D.Normalize())
	{
		if (const ACharacter* Character = CachedOwnerCharacter.Get())
		{
			Direction2D = Character->GetActorForwardVector();
			Direction2D.Z = 0.0f;
			Direction2D.Normalize();
		}
		else
		{
			return false;
		}
	}

	const float CapsuleHalfHeight = bIsFalling && Profile.VerticalOffsetUp > 0.0f
		? Profile.VerticalOffsetUp
		: Profile.CapsuleHalfHeight;

	const float BaseZOffset = bIsFalling && Profile.VerticalOffsetDown > 0.0f
		? Profile.VerticalOffsetDown
		: Profile.StartZOffset;

	const float MinDistance = bIsFalling && Profile.MinLedgeHeight > 0.0f
		? Profile.MinLedgeHeight
		: Profile.ForwardDistanceMin;
	const float MaxDistance = bIsFalling && Profile.MaxLedgeHeight > 0.0f
		? Profile.MaxLedgeHeight
		: Profile.ForwardDistanceMax;

	const float Speed2DForward = FVector(Velocity.X, Velocity.Y, 0.0f).Size();
	const float ClampedSpeed = FMath::Clamp(Speed2DForward, 0.0f, 500.0f);
	const float Alpha = (500.0f > 0.0f) ? (ClampedSpeed / 500.0f) : 0.0f;
	const float ForwardDistance = FMath::Lerp(MinDistance, MaxDistance, Alpha);

	const FVector Start = ActorLocation + FVector(0.0f, 0.0f, BaseZOffset);
	const FVector End = Start + Direction2D * ForwardDistance;

	OutSettings.Start = Start;
	OutSettings.End = End;
	OutSettings.Radius = Profile.CapsuleRadius;
	OutSettings.HalfHeight = CapsuleHalfHeight;
	return true;
}

bool USOTS_ParkourComponent::BuildClimbForwardTrace(
	const FSOTS_ParkourTraceProfile& Profile,
	FSOTS_ParkourCapsuleTraceSettings& OutSettings) const
{
	// CGF ParkourComp climb/ReachLedge forward trace using the climb arrow as origin.
	if (!ClimbArrowComponent)
	{
		return false;
	}

	const FVector Origin = ClimbArrowComponent->GetComponentLocation();
	const FVector Forward = ClimbArrowComponent->GetForwardVector();
	const float ForwardDistance = (Profile.ForwardDistanceMax > 0.0f) ? Profile.ForwardDistanceMax : 60.0f;

	OutSettings.Start = Origin;
	OutSettings.End = Origin + Forward * ForwardDistance;
	OutSettings.Radius = Profile.CapsuleRadius;
	OutSettings.HalfHeight = Profile.CapsuleHalfHeight;
	return true;
}

void USOTS_ParkourComponent::BuildMantleTrace(
	const FSOTS_ParkourTraceProfile& Profile,
	const FVector& WarpTopPoint,
	float CapsuleHalfHeight,
	FSOTS_ParkourCapsuleTraceSettings& OutSettings) const
{
	const FVector UpVector = FVector::UpVector;
	const float StartZOffset = CapsuleHalfHeight + Profile.StartZOffset; // CGF upper sample
	const float EndZOffset = CapsuleHalfHeight + Profile.VerticalOffsetDown; // CGF lower sample

	OutSettings.Start = WarpTopPoint + UpVector * StartZOffset;
	OutSettings.End = WarpTopPoint + UpVector * EndZOffset;
	OutSettings.Radius = Profile.CapsuleRadius;
	OutSettings.HalfHeight = (Profile.CapsuleHalfHeight > 0.0f)
		? Profile.CapsuleHalfHeight
		: FMath::Clamp(CapsuleHalfHeight * 0.2f, 12.0f, 40.0f); // default clamp fallback
}

void USOTS_ParkourComponent::BuildVaultTrace(
	const FSOTS_ParkourTraceProfile& Profile,
	const FVector& WarpTopPoint,
	float CapsuleHalfHeight,
	FSOTS_ParkourCapsuleTraceSettings& OutSettings) const
{
	const FVector UpVector = FVector::UpVector;
	const float StartZOffset = CapsuleHalfHeight + Profile.StartZOffset; // Upper sample
	const float EndZOffset = CapsuleHalfHeight + Profile.VerticalOffsetDown; // Lower sample

	OutSettings.Start = WarpTopPoint + UpVector * StartZOffset;
	OutSettings.End = WarpTopPoint + UpVector * EndZOffset;
	OutSettings.Radius = Profile.CapsuleRadius;
	OutSettings.HalfHeight = (Profile.CapsuleHalfHeight > 0.0f)
		? Profile.CapsuleHalfHeight
		: FMath::Clamp(CapsuleHalfHeight * 0.2f, 12.0f, 40.0f);
}

bool USOTS_ParkourComponent::GetFirstCapsuleTraceSettings(FSOTS_ParkourCapsuleTraceSettings& OutSettings) const
{
	OutSettings.Reset();
	return BuildFirstCapsuleTrace(OutSettings);
}

bool USOTS_ParkourComponent::BuildFirstCapsuleTrace_NotBusy(FSOTS_ParkourCapsuleTraceSettings& OutSettings) const
{
	ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return false;
	}

	const FVector ActorLocation = Character->GetActorLocation();
	const FVector Velocity = GetResolvedInputDirection();
	const bool bIsFalling = GetResolvedIsInAir();

	const FSOTS_ParkourTraceProfile* GroundProfile = FindTraceProfileForAction(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.GroundForward"), false));
	FSOTS_ParkourTraceProfile DefaultProfile;
	DefaultProfile.CapsuleRadius = 40.0f;
	DefaultProfile.CapsuleHalfHeight = 120.0f;
	DefaultProfile.ForwardDistanceMin = 50.0f;
	DefaultProfile.ForwardDistanceMax = 200.0f;
	DefaultProfile.StartZOffset = 75.0f;
	DefaultProfile.VerticalOffsetDown = 15.0f;
	DefaultProfile.VerticalOffsetUp = 70.0f;
	DefaultProfile.MinLedgeHeight = 25.0f;
	DefaultProfile.MaxLedgeHeight = 100.0f;

	return BuildGroundForwardTrace(GroundProfile ? *GroundProfile : DefaultProfile, Velocity, ActorLocation, Velocity, bIsFalling, OutSettings);
}

bool USOTS_ParkourComponent::BuildFirstCapsuleTrace_Climb(FSOTS_ParkourCapsuleTraceSettings& OutSettings) const
{
	OutSettings.Reset();

	const FSOTS_ParkourTraceProfile* ClimbProfile = FindTraceProfileForAction(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.ClimbForward"), false));
	FSOTS_ParkourTraceProfile DefaultProfile;
	DefaultProfile.CapsuleRadius = 20.0f;
	DefaultProfile.CapsuleHalfHeight = 20.0f;
	DefaultProfile.ForwardDistanceMax = 60.0f;

	return BuildClimbForwardTrace(ClimbProfile ? *ClimbProfile : DefaultProfile, OutSettings);
}

bool USOTS_ParkourComponent::BuildFirstCapsuleTrace(FSOTS_ParkourCapsuleTraceSettings& OutSettings) const
{
	// Mirror the BP switch: prefer the climb branch when in an active climb-like state.
	if (ParkourState == ESOTS_ParkourState::Active && BuildFirstCapsuleTrace_Climb(OutSettings))
	{
		return true;
	}

	return BuildFirstCapsuleTrace_NotBusy(OutSettings);
}

bool USOTS_ParkourComponent::RunCapsuleTrace(
	const FSOTS_ParkourCapsuleTraceSettings& Settings,
	FHitResult& OutHit,
	ECollisionChannel TraceChannel,
	const FName& DebugName
) const
{
	ACharacter* Character = CachedOwnerCharacter.Get();
	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!Character || !WorldContext)
	{
		OutHit = FHitResult();
		return false;
	}

	if (!GetWorld())
	{
		OutHit = FHitResult();
		return false;
	}

	if (!TryConsumeTraceSlot())
	{
		OutHit = FHitResult();
		return false;
	}

	const bool bHit = RunCapsuleTraceWithOmniTrace(
		WorldContext,
		Settings.Start,
		Settings.End,
		Settings.Radius,
		Settings.HalfHeight,
		TraceChannel,
		Character,
		OutHit);

	const USOTS_ParkourConfig* Config = GetConfig();
	const bool bDrawForward = Config ? Config->bDrawForwardProbeDebug : true;
	const FColor CapsuleColor = (bHit && OutHit.IsValidBlockingHit()) ? FColor::Green : FColor::Red;
	DrawDebugCapsuleIfEnabled(Settings.Start, Settings.HalfHeight, Settings.Radius, FQuat::Identity, bDrawForward, CapsuleColor);
	if (!Settings.End.Equals(Settings.Start))
	{
		DrawDebugCapsuleIfEnabled(Settings.End, Settings.HalfHeight, Settings.Radius, FQuat::Identity, bDrawForward, CapsuleColor);
		DrawDebugLineIfEnabled(Settings.Start, Settings.End, bDrawForward, CapsuleColor);
	}
	if (bHit && OutHit.IsValidBlockingHit())
	{
		DrawDebugPointIfEnabled(OutHit.ImpactPoint, 10.0f, bDrawForward, FColor::Yellow);
	}

	const FString DebugLabel = DebugName.ToString();
	LogTraceResult(*DebugLabel, Settings.Start, Settings.End, Settings.Radius, Settings.HalfHeight, OutHit);

	if (bEnableDebugLogging)
	{
		USOTS_ParkourDebugLibrary::DrawParkourCapsuleTrace(
			WorldContext,
			Settings,
			bHit && OutHit.IsValidBlockingHit(),
			OutHit,
			CapsuleDebugDrawTime
		);
	}

	return bHit && OutHit.IsValidBlockingHit();
}

bool USOTS_ParkourComponent::TryDetectSimpleParkourOpportunity(FSOTS_ParkourResult& OutResult) const
{
	const_cast<USOTS_ParkourComponent*>(this)->PerformParkourDetection(
		const_cast<USOTS_ParkourComponent*>(this)->GetResolvedInputDirection(),
		false);

	OutResult = LastResult;
	return OutResult.bHasResult || OutResult.bIsValid;
}

bool USOTS_ParkourComponent::ConfirmSurfaceForAction(
	const FHitResult& FirstHit,
	float CapsuleHalfHeight,
	ESOTS_ParkourAction Action,
	FSOTS_ParkourResult& InOutResult) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return false;
	}

	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return false;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	const bool bDrawCorner = Config ? Config->bDrawCornerDebug : true;

	if (!GetWorld())
	{
		return false;
	}

	const FVector CharacterLocation = Character->GetActorLocation();
	const float CharacterFeetZ = CharacterLocation.Z - CapsuleHalfHeight;
	const FVector WarpTopPoint(
		FirstHit.ImpactPoint.X,
		FirstHit.ImpactPoint.Y,
		CharacterFeetZ + InOutResult.HeightDelta);

	if (!TryConsumeTraceSlot())
	{
		return false;
	}

	const FGameplayTag ProfileTag = (Action == ESOTS_ParkourAction::Vault)
		? FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.Vault"), false)
		: FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.Mantle"), false);

	const FSOTS_ParkourTraceProfile* Profile = FindTraceProfileForAction(ProfileTag);
	FSOTS_ParkourTraceProfile DefaultProfile;
	DefaultProfile.CapsuleRadius = 25.0f;
	DefaultProfile.CapsuleHalfHeight = Config ? Config->ConfirmCapsuleHalfHeight : 82.0f;
	if (Action == ESOTS_ParkourAction::Vault)
	{
		DefaultProfile.StartZOffset = 18.0f;
		DefaultProfile.VerticalOffsetDown = 5.0f;
	}
	else
	{
		DefaultProfile.StartZOffset = 8.0f;
		DefaultProfile.VerticalOffsetDown = -8.0f;
	}

	// For vaults, nudge the confirm trace slightly forward to mirror CGF's low obstacle check.
	const float ProfileForwardOffset = (Profile && Profile->ForwardDistanceMax != 0.0f) ? Profile->ForwardDistanceMax : 0.0f;
	const float VaultForwardOffset = (Config && Config->ConfirmVaultForwardOffset != 0.0f)
		? Config->ConfirmVaultForwardOffset
		: (ProfileForwardOffset != 0.0f ? ProfileForwardOffset : SecondTraceForwardOffset);
	const FVector ForwardNudge = (Action == ESOTS_ParkourAction::Vault)
		? (-FirstHit.ImpactNormal.GetSafeNormal() * VaultForwardOffset)
		: FVector::ZeroVector;

	FSOTS_ParkourCapsuleTraceSettings SurfaceTraceSettings;
	const FVector TraceWarpPoint = WarpTopPoint + ForwardNudge;
	if (Action == ESOTS_ParkourAction::Vault)
	{
		BuildVaultTrace(Profile ? *Profile : DefaultProfile, TraceWarpPoint, CapsuleHalfHeight, SurfaceTraceSettings);
	}
	else
	{
		BuildMantleTrace(Profile ? *Profile : DefaultProfile, TraceWarpPoint, CapsuleHalfHeight, SurfaceTraceSettings);
	}

	// CGF confirm capsules are overlaps; switch to explicit overlap test with CGF-sized capsule.
	SurfaceTraceSettings.End = SurfaceTraceSettings.Start;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ParkourConfirmOverlap), false, Character);
	QueryParams.AddIgnoredActor(Character);

	// Clamp to CGF capsule sizes: prefer per-action profile dimensions, then config, then trace fallback.
	const float ProfileCapsuleRadius = Profile ? Profile->CapsuleRadius : DefaultProfile.CapsuleRadius;
	const float ProfileCapsuleHalfHeight = Profile ? Profile->CapsuleHalfHeight : DefaultProfile.CapsuleHalfHeight;

	float ConfirmRadius = SurfaceTraceSettings.Radius;
	float ConfirmHalfHeight = SurfaceTraceSettings.HalfHeight;

	if (ProfileCapsuleRadius > 0.0f)
	{
		ConfirmRadius = ProfileCapsuleRadius;
	}
	else if (Config && Config->ConfirmCapsuleRadius > 0.0f)
	{
		ConfirmRadius = Config->ConfirmCapsuleRadius;
	}

	if (ProfileCapsuleHalfHeight > 0.0f)
	{
		ConfirmHalfHeight = ProfileCapsuleHalfHeight;
	}
	else if (Config && Config->ConfirmCapsuleHalfHeight > 0.0f)
	{
		ConfirmHalfHeight = Config->ConfirmCapsuleHalfHeight;
	}

	SurfaceTraceSettings.Radius = ConfirmRadius;
	SurfaceTraceSettings.HalfHeight = ConfirmHalfHeight;

	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(SurfaceTraceSettings.Radius, SurfaceTraceSettings.HalfHeight);
	TArray<FOverlapResult> Overlaps;
	const bool bHitSurface = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		SurfaceTraceSettings.Start,
		FQuat::Identity,
		ParkourTraceChannel,
		CapsuleShape,
		QueryParams);

	// Derive a synthetic hit so downstream code and debug logging still have a location/normal.
	FHitResult SurfaceHit;
	SurfaceHit.ImpactPoint = SurfaceTraceSettings.Start;
	const FVector FallbackNormal = FirstHit.TraceStart.IsNearlyZero() ? FVector::UpVector : FirstHit.TraceStart.GetSafeNormal();
	SurfaceHit.ImpactNormal = FirstHit.ImpactNormal.IsNearlyZero() ? FallbackNormal : FirstHit.ImpactNormal.GetSafeNormal();
	SurfaceHit.TraceStart = SurfaceTraceSettings.Start;
	SurfaceHit.TraceEnd = SurfaceTraceSettings.End;
	SurfaceHit.bBlockingHit = bHitSurface;

	const bool bDrawVaultMantle = Config ? Config->bDrawVaultMantleDebug : true;
	const FColor ConfirmColor = bHitSurface ? FColor::Green : FColor::Red;
	DrawDebugCapsuleIfEnabled(SurfaceTraceSettings.Start, SurfaceTraceSettings.HalfHeight, SurfaceTraceSettings.Radius, FQuat::Identity, bDrawVaultMantle, ConfirmColor);
	if (!SurfaceTraceSettings.End.Equals(SurfaceTraceSettings.Start))
	{
		DrawDebugCapsuleIfEnabled(SurfaceTraceSettings.End, SurfaceTraceSettings.HalfHeight, SurfaceTraceSettings.Radius, FQuat::Identity, bDrawVaultMantle, ConfirmColor);
		DrawDebugLineIfEnabled(SurfaceTraceSettings.Start, SurfaceTraceSettings.End, bDrawVaultMantle, ConfirmColor);
	}
	if (bHitSurface)
	{
		DrawDebugPointIfEnabled(SurfaceTraceSettings.Start, 10.0f, bDrawVaultMantle, FColor::Yellow); // CGF parity: confirm hit marker
	}

	LogTraceResult(TEXT("SurfaceConfirm"), SurfaceTraceSettings.Start, SurfaceTraceSettings.End, SurfaceTraceSettings.Radius, SurfaceTraceSettings.HalfHeight, SurfaceHit);

	if (!bHitSurface)
	{
		TriggerAutoClimbJump(TEXT("SurfaceConfirmFailed"));
		return false;
	}

	InOutResult.WorldLocation = SurfaceHit.ImpactPoint;
	InOutResult.WorldNormal = SurfaceHit.ImpactNormal;
	InOutResult.TargetLocation = SurfaceHit.ImpactPoint;
	InOutResult.SurfaceNormal = SurfaceHit.ImpactNormal;
	InOutResult.Hit = SurfaceHit;
	ComputeHandIKProbes(SurfaceHit.ImpactPoint, SurfaceHit.ImpactNormal, CapsuleHalfHeight, InOutResult);
	return true;
}

bool USOTS_ParkourComponent::RunDropHangProbes(
	const FHitResult& FirstHit,
	float CapsuleHalfHeight,
	FSOTS_ParkourResult& InOutResult) const
{
	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return false;
	}

	if (!GetWorld())
	{
		return false;
	}

	const FVector DownVector = FVector(0.0f, 0.0f, -1.0f);
	const float ProbeDepth = FMath::Max(CapsuleHalfHeight * 2.5f, 100.0f);
	const FVector Start = FirstHit.ImpactPoint;
	const FVector End = Start + DownVector * ProbeDepth;

	FHitResult PrimaryHit;
	const bool bPrimary = TryConsumeTraceSlot() && USOTS_ParkourSurfaceProbes::ProbeDropHangPrimary(
		WorldContext,
		Start,
		End,
		PrimaryHit,
		ParkourTraceChannel,
		bEnableDebugLogging,
		CapsuleDebugDrawTime);
	LogTraceResult(TEXT("DropHangPrimary"), Start, End, 35.0f, 35.0f, PrimaryHit);

	FHitResult SecondaryHit;
	const bool bSecondary = TryConsumeTraceSlot() && USOTS_ParkourSurfaceProbes::ProbeDropHangSecondary(
		WorldContext,
		Start,
		End,
		SecondaryHit,
		ParkourTraceChannel,
		bEnableDebugLogging,
		CapsuleDebugDrawTime);
	LogTraceResult(TEXT("DropHangSecondary"), Start, End, 5.0f, 5.0f, SecondaryHit);

	if (!bPrimary && !bSecondary)
	{
		return false;
	}

	const FHitResult& ChosenHit = bPrimary ? PrimaryHit : SecondaryHit;
	InOutResult.WorldLocation = ChosenHit.ImpactPoint;
	InOutResult.WorldNormal = ChosenHit.ImpactNormal;
	InOutResult.TargetLocation = ChosenHit.ImpactPoint;
	InOutResult.SurfaceNormal = ChosenHit.ImpactNormal;
	InOutResult.Hit = ChosenHit;
	return true;
}

bool USOTS_ParkourComponent::TryTicTacSideProbes(
	const FHitResult& FirstHit,
	float CapsuleHalfHeight,
	FSOTS_ParkourResult& InOutResult) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return false;
	}

	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return false;
	}

	InOutResult.DirectionTag = ResolveParkourTagByName(TEXT("Parkour.TicTacSide.NoDirection"));

	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float CapsuleRadius = Capsule ? Capsule->GetUnscaledCapsuleRadius() : 42.0f;

	FVector WallNormal = FirstHit.ImpactNormal.IsNearlyZero() ? FVector::ForwardVector : FirstHit.ImpactNormal.GetSafeNormal();
	if (WallNormal.IsNearlyZero())
	{
		WallNormal = FVector::ForwardVector;
	}

	const FVector Forward = -WallNormal;
	FVector Right = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();
	if (Right.IsNearlyZero())
	{
		Right = FVector::RightVector;
	}

	const float ForwardOffset = -65.0f;
	const float SideOffset = 55.0f;
	const float VerticalOffset = CapsuleHalfHeight * 0.5f;
	const float ProbeRadius = 30.0f;

	for (int32 Direction : {1, -1})
	{
		const FVector Base = Character->GetActorLocation() + Forward * ForwardOffset + FVector(0.0f, 0.0f, VerticalOffset);
		const FVector Start = Base + Right * SideOffset * static_cast<float>(Direction);
		const FVector End = Start + Right * SideOffset * 0.5f * static_cast<float>(Direction);

		FHitResult TicTacHit;
		const bool bTicTacHit = TryConsumeTraceSlot() && USOTS_ParkourSurfaceProbes::ProbeTicTac(
			WorldContext,
			Start,
			End,
			TicTacHit,
			ParkourTraceChannel,
			bEnableDebugLogging,
			bEnableDebugLogging ? 5.0f : CapsuleDebugDrawTime);
		LogTraceResult(TEXT("TicTacSide"), Start, End, ProbeRadius, ProbeRadius, TicTacHit);

		if (bTicTacHit)
		{
			const bool bIsRight = Direction > 0;
			InOutResult.Reset();
			InOutResult.bHasResult = true;
			InOutResult.bIsValid = true;
			InOutResult.Action = ESOTS_ParkourAction::TicTac;
			InOutResult.ClimbStyle = ESOTS_ClimbStyle::Braced;
			InOutResult.ResultType = ESOTS_ParkourResultType::Mantle;
			InOutResult.DirectionTag = ResolveParkourTagByName(bIsRight ? TEXT("Parkour.TicTacSide.Right") : TEXT("Parkour.TicTacSide.Left"));
			InOutResult.ActionTag = ResolveParkourTagByName(bIsRight ? TEXT("Parkour.Action.TicTacRightBracedHang") : TEXT("Parkour.Action.TicTacLeftBracedHang"));
			InOutResult.ClimbStyleTag = ResolveParkourTagByName(TEXT("Parkour.ClimbStyle.Braced"));
			InOutResult.WorldLocation = TicTacHit.ImpactPoint;
			InOutResult.WorldNormal = TicTacHit.ImpactNormal;
			InOutResult.TargetLocation = TicTacHit.ImpactPoint;
			InOutResult.SurfaceNormal = TicTacHit.ImpactNormal;
			InOutResult.HeightDelta = TicTacHit.ImpactPoint.Z - (Character->GetActorLocation().Z - CapsuleHalfHeight);
			InOutResult.Hit = TicTacHit;
			ComputeHandIKProbes(TicTacHit.ImpactPoint, TicTacHit.ImpactNormal, CapsuleHalfHeight, InOutResult);
			return true;
		}
	}

	return false;
}

void USOTS_ParkourComponent::ExecuteCurrentParkour()
{
	ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		BroadcastParkourActionFinished(false);
		ParkourState = ESOTS_ParkourState::Idle;
		return;
	}

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float CapsuleRadius = Capsule ? Capsule->GetUnscaledCapsuleRadius() : 42.0f;
	const float CapsuleHalfHeight = Capsule ? Capsule->GetUnscaledCapsuleHalfHeight() : 96.0f;

	if (LastResult.Action == ESOTS_ParkourAction::CornerMove)
	{
		ExecuteCornerMove(Character, Capsule, CapsuleHalfHeight, CapsuleRadius);
		return;
	}

	FVector TargetLocation = LastResult.TargetLocation;
	const FVector SurfaceNormal = LastResult.SurfaceNormal.IsNearlyZero() ? Character->GetActorForwardVector() : LastResult.SurfaceNormal;

	if (const USOTS_ParkourConfig* Config = GetConfig())
	{
		switch (LastResult.Action)
		{
		case ESOTS_ParkourAction::Mantle:
		case ESOTS_ParkourAction::Vault:
		{
			const FVector ForwardOffset = SurfaceNormal * CapsuleRadius * Config->MantleForwardOffsetScale;
			const FVector UpOffset = FVector(0.0f, 0.0f, CapsuleHalfHeight * Config->MantleUpOffsetScale);
			TargetLocation += ForwardOffset + UpOffset;
			break;
		}
		case ESOTS_ParkourAction::Drop:
		{
			TargetLocation = Character->GetActorLocation() - FVector(0.0f, 0.0f, Config->DropStepDownDistance);
			break;
		}
		default:
			break;
		}
	}

	ApplyWarpSettingsFromAction(LastResult.Action, LastResult);

	const FVector WarpTarget = TargetLocation;
	if (!ApplyMotionWarp(WarpTarget, SurfaceNormal))
	{
		Character->SetActorLocation(WarpTarget, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (bHasLastAppliedActionDefinition)
	{
		PlayActionMontage(LastAppliedActionDefinition);
	}

	if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Verbose, TEXT("USOTS_ParkourComponent: ExecuteCurrentParkour Action=%d Target=(%.1f, %.1f, %.1f)"),
			static_cast<int32>(LastResult.Action), TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
	}

	BroadcastParkourActionFinished(true);

	// Linger in Exiting for a short window (CGF post-action settle) before returning to Idle.
	ParkourStateLingerTime = GetExitLingerForAction(LastResult.Action);
	SetParkourState(ESOTS_ParkourState::Exiting);
	if (ParkourStateLingerTime <= 0.0f)
	{
		SetParkourState(ESOTS_ParkourState::Idle);
	}
}

void USOTS_ParkourComponent::ExecuteCornerMove(ACharacter* Character, UCapsuleComponent* Capsule, float CapsuleHalfHeight, float CapsuleRadius)
{
	if (!Character || !Capsule)
	{
		BroadcastParkourActionFinished(false);
		SetParkourState(ESOTS_ParkourState::Idle);
		return;
	}

	const FVector WallNormal = LastResult.SurfaceNormal.IsNearlyZero()
		? -Character->GetActorForwardVector()
		: LastResult.SurfaceNormal.GetSafeNormal();

	const FVector Facing = WallNormal.IsNearlyZero() ? -Character->GetActorForwardVector() : -WallNormal;
	const FRotator TargetRotation = FRotationMatrix::MakeFromXZ(Facing, FVector::UpVector).Rotator();

	FVector TargetLocation = LastResult.TargetLocation;
	TargetLocation += WallNormal * CapsuleRadius;
	TargetLocation += FVector(0.0f, 0.0f, CapsuleHalfHeight);

	// Mirror the Blueprint axis check: update direction tag from current horizontal input.
	const FVector Input2D = GetResolvedInputDirection().GetSafeNormal2D();
	if (!Input2D.IsNearlyZero())
	{
		const float RightDot = FVector::DotProduct(Input2D, Character->GetActorRightVector().GetSafeNormal2D());
		const bool bMoveRight = RightDot >= 0.0f;
		LastResult.DirectionTag = FGameplayTag::RequestGameplayTag(bMoveRight
			? TEXT("Parkour.Direction.Right")
			: TEXT("Parkour.Direction.Left"), false);
		UpdateGameplayTagsFromResult(LastResult);
	}

	CornerMoveTargetLocation = TargetLocation;
	CornerMoveTargetRotation = TargetRotation;
	bCornerMoveInProgress = true;

	const float Duration = (LastResult.ClimbStyle == ESOTS_ClimbStyle::FreeHang) ? 0.9f : 0.5f;
	if (Duration <= KINDA_SMALL_NUMBER)
	{
		Capsule->SetWorldLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
		OnCornerMoveFinished();
		return;
	}

	if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Verbose, TEXT("CornerMove start: duration=%.2f loc=(%.1f, %.1f, %.1f) rot=(%.1f, %.1f, %.1f)"),
			Duration,
			TargetLocation.X, TargetLocation.Y, TargetLocation.Z,
			TargetRotation.Pitch, TargetRotation.Yaw, TargetRotation.Roll);
	}

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = FName(TEXT("OnCornerMoveFinished"));
	LatentInfo.Linkage = 0;
	LatentInfo.UUID = ++CornerMoveLatentId;

	UKismetSystemLibrary::MoveComponentTo(
		Capsule,
		TargetLocation,
		TargetRotation,
		false,
		false,
		Duration,
		true,
		EMoveComponentAction::Type::Move,
		LatentInfo);
}

void USOTS_ParkourComponent::OnCornerMoveFinished()
{
	if (!bCornerMoveInProgress)
	{
		return;
	}

	bCornerMoveInProgress = false;
	BroadcastParkourActionFinished(true);

	LastResult.Action = ESOTS_ParkourAction::None;
	FinalizeResultTags(LastResult);
	UpdateGameplayTagsFromResult(LastResult);
	NotifyParkourResultUpdated(LastResult);

	SetParkourState(ESOTS_ParkourState::Exiting);
	SetParkourState(ESOTS_ParkourState::Idle);
	UpdateStateMachine(0.0f);
}

bool USOTS_ParkourComponent::ApplyMotionWarp(const FVector& TargetLocation, const FVector& SurfaceNormal) const
{
	if (!bUseMotionWarping)
	{
		return false;
	}

	ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return false;
	}

	UMotionWarpingComponent* MotionWarp = Character->FindComponentByClass<UMotionWarpingComponent>();
	if (!MotionWarp)
	{
		return false;
	}

	const bool bHasMultiple = LastResult.WarpTargets.Num() > 0;
	int32 AddedCount = 0;

	if (bHasMultiple)
	{
		for (const FSOTS_ParkourWarpRuntimeTarget& Target : LastResult.WarpTargets)
		{
			MotionWarp->AddOrUpdateWarpTargetFromTransform(Target.TargetName, Target.TargetTransform);
			AddedCount++;

			if (bEnableDebugLogging)
			{
				const FRotator Rot = Target.TargetTransform.Rotator();
				const FVector Loc = Target.TargetTransform.GetLocation();
				UE_LOG(LogSOTS_Parkour, Verbose, TEXT("USOTS_ParkourComponent: MotionWarp target set Name=%s Loc=(%.1f, %.1f, %.1f) Rot=(%.1f, %.1f, %.1f)"),
					*Target.TargetName.ToString(), Loc.X, Loc.Y, Loc.Z, Rot.Pitch, Rot.Yaw, Rot.Roll);
			}
		}
	}

	if (AddedCount == 0)
	{
		const FVector Velocity2D = GetResolvedInputDirection();
		const FTransform SingleTransform = BuildWarpTransform(TargetLocation, SurfaceNormal, Velocity2D, 0.0f, 0.0f, ESOTS_ParkourWarpFacingMode::FaceSurface);

		FName TargetName = !LastResult.WarpTargetName.IsNone()
			? LastResult.WarpTargetName
			: (MotionWarpTargetName.IsNone() ? FName(TEXT("ParkourTarget")) : MotionWarpTargetName);

		MotionWarp->AddOrUpdateWarpTargetFromTransform(TargetName, SingleTransform);
		AddedCount = 1;

		if (bEnableDebugLogging)
		{
			const FRotator Rot = SingleTransform.Rotator();
			const FVector Loc = SingleTransform.GetLocation();
			UE_LOG(LogSOTS_Parkour, Verbose, TEXT("USOTS_ParkourComponent: MotionWarp target set Name=%s Loc=(%.1f, %.1f, %.1f) Rot=(%.1f, %.1f, %.1f)"),
				*TargetName.ToString(), Loc.X, Loc.Y, Loc.Z, Rot.Pitch, Rot.Yaw, Rot.Roll);
		}
	}

	if (UMotionWarpingComponent* MotionWarpForModifiers = MotionWarp)
	{
		if (bHasLastAppliedActionDefinition)
		{
			UAnimMontage* LoadedMontage = LastAppliedActionDefinition.ParkourMontage.LoadSynchronous();
			BP_RegisterRootMotionModifiers(MotionWarpForModifiers, LastAppliedActionDefinition, LastResult.WarpTargets, LoadedMontage);
		}
	}

	return AddedCount > 0;
}

void USOTS_ParkourComponent::ValidateCurrentConfig() const
{
	const USOTS_ParkourConfig* Config = GetConfig();
	if (!Config)
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("USOTS_ParkourComponent: ParkourConfig is null; using defaults."));
		return;
	}

	if (Config->MantleMinHeight > Config->MantleMaxHeight)
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("USOTS_ParkourComponent: MantleMinHeight > MantleMaxHeight; values may be invalid."));
	}
}

bool USOTS_ParkourComponent::BuildSecondCapsuleTrace(const FHitResult& FirstHit, FSOTS_ParkourCapsuleTraceSettings& OutSettings) const
{
	ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return false;
	}

	// BP follow-up capsules push back into the wall using the hit normal, with larger half-heights.
	const FVector WallNormal = FirstHit.ImpactNormal.IsNearlyZero() ? FVector::ForwardVector : FirstHit.ImpactNormal.GetSafeNormal();
	const FVector Forward = -WallNormal;
	const FVector Up = FVector::UpVector;

	const FSOTS_ParkourTraceProfile* ConfirmProfile = FindTraceProfileForAction(FGameplayTag::RequestGameplayTag(TEXT("Parkour.TraceProfile.ConfirmForward"), false));
	FSOTS_ParkourTraceProfile DefaultProfile;
	DefaultProfile.CapsuleRadius = 25.0f;
	DefaultProfile.CapsuleHalfHeight = 82.0f;
	DefaultProfile.ForwardDistanceMax = -40.0f;
	DefaultProfile.StartZOffset = -80.0f;

	const FSOTS_ParkourTraceProfile& Profile = ConfirmProfile ? *ConfirmProfile : DefaultProfile;

	const float ForwardOffset = (Profile.ForwardDistanceMax != 0.0f)
		? Profile.ForwardDistanceMax
		: (SecondTraceForwardOffset != 0.0f ? SecondTraceForwardOffset : -30.0f);

	const float VerticalOffset = (Profile.StartZOffset != 0.0f)
		? Profile.StartZOffset
		: (SecondTraceVerticalOffset != 0.0f ? SecondTraceVerticalOffset : -30.0f);

	const float CharacterHalfHeight = Character->GetSimpleCollisionHalfHeight();
	const float ProfileHalfHeight = Profile.CapsuleHalfHeight > 0.0f ? Profile.CapsuleHalfHeight : CharacterHalfHeight;
	const float HalfHeight = FMath::Max(40.0f, ProfileHalfHeight);

	// Radius prefers profile; falls back to span-derived when unset.
	float Radius = Profile.CapsuleRadius;
	if (Radius <= 0.0f)
	{
		const float TraceSpan = FirstHit.TraceEnd.IsNearlyZero() ? 0.0f : FVector::Dist(FirstHit.TraceStart, FirstHit.TraceEnd);
		Radius = FMath::Max(20.0f, TraceSpan * 0.5f);
	}

	const FVector Start = FirstHit.ImpactPoint + Forward * ForwardOffset + Up * VerticalOffset;
	const FVector End = Start + Forward * ForwardOffset;

	OutSettings.Start = Start;
	OutSettings.End = End;
	OutSettings.Radius = Radius;
	OutSettings.HalfHeight = HalfHeight;
	return true;
}

bool USOTS_ParkourComponent::TryDetectVaultPath(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& InOutResult) const
{
	FSOTS_ParkourCapsuleTraceSettings SecondTrace;
	if (!BuildSecondCapsuleTrace(FirstHit, SecondTrace))
	{
		return false;
	}

	FHitResult SecondHit;
	if (!TryConsumeTraceSlot())
	{
		return false;
	}

	const bool bSecondHit = RunCapsuleTraceWithOmniTrace(
		GetOwner() ? static_cast<UObject*>(GetOwner()) : static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this)),
		SecondTrace.Start,
		SecondTrace.End,
		SecondTrace.Radius,
		SecondTrace.HalfHeight,
		ParkourTraceChannel,
		CachedOwnerCharacter.Get(),
		SecondHit);

	if (!bSecondHit)
	{
		return false;
	}

	InOutResult.WorldLocation = SecondHit.ImpactPoint;
	InOutResult.WorldNormal = SecondHit.ImpactNormal;
	InOutResult.TargetLocation = SecondHit.ImpactPoint;
	InOutResult.SurfaceNormal = SecondHit.ImpactNormal;
	InOutResult.HeightDelta = SecondHit.ImpactPoint.Z - (SecondTrace.Start.Z - CapsuleHalfHeight);
	InOutResult.Hit = SecondHit;

	return ConfirmSurfaceForAction(SecondHit, CapsuleHalfHeight, ESOTS_ParkourAction::Vault, InOutResult);
}

bool USOTS_ParkourComponent::TryDetectLedgeMoveAndHops(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& InOutResult) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return false;
	}

	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return false;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	const bool bDrawCorner = Config ? Config->bDrawCornerDebug : true;

	const FVector WallNormal = FirstHit.ImpactNormal.IsNearlyZero() ? FVector::ForwardVector : FirstHit.ImpactNormal.GetSafeNormal();
	const FVector Forward = -WallNormal;
	const FVector Right = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();
	const FVector Up = FVector::UpVector;
	const FVector Base = FirstHit.ImpactPoint + Up * LedgeMoveVerticalOffset;

	const float PrimaryProbeRadius = LedgeMoveProbeRadius > 0.0f ? LedgeMoveProbeRadius : 6.0f;
	const float FallbackProbeRadius = FMath::Max(2.5f, PrimaryProbeRadius * 0.5f);

	auto SphereSweepWithFallback = [&](const FVector& Start, const FVector& End, FHitResult& OutHit) -> bool
	{
		auto TryRadius = [&](float Radius) -> bool
		{
			if (!TryConsumeTraceSlot())
			{
				return false;
			}

			return RunSphereTraceWithOmniTrace(WorldContext, Start, End, Radius, ParkourTraceChannel, Character, OutHit);
		};

		if (TryRadius(PrimaryProbeRadius))
		{
			return true;
		}

		if (FallbackProbeRadius < PrimaryProbeRadius - KINDA_SMALL_NUMBER && TryRadius(FallbackProbeRadius))
		{
			return true;
		}

		return false;
	};

	const auto BuildSweepEnds = [&](const FVector& Direction, float Distance)
	{
		const FVector Start = Base;
		const FVector End = Base + Direction * Distance;
		return TPair<FVector, FVector>(Start, End);
	};

	// 1) Ledge move (sideways along the wall)
	for (int32 Side : { -1, 1 })
	{
		const float Distance = LedgeMoveHorizontalDistance * static_cast<float>(Side);
		const TPair<FVector, FVector> Ends = BuildSweepEnds(Right, Distance);

		FHitResult SideHit;
		if (SphereSweepWithFallback(Ends.Key, Ends.Value, SideHit))
		{
			InOutResult.Reset();
			InOutResult.bHasResult = true;
			InOutResult.bIsValid = true;
			InOutResult.Action = ESOTS_ParkourAction::LedgeMove;
			InOutResult.ClimbStyle = ESOTS_ClimbStyle::Braced;
			InOutResult.ResultType = ESOTS_ParkourResultType::Mantle;
			InOutResult.WorldLocation = SideHit.ImpactPoint;
			InOutResult.WorldNormal = SideHit.ImpactNormal;
			InOutResult.TargetLocation = SideHit.ImpactPoint;
			InOutResult.SurfaceNormal = SideHit.ImpactNormal;
			InOutResult.HeightDelta = SideHit.ImpactPoint.Z - (Character->GetActorLocation().Z - CapsuleHalfHeight);
			InOutResult.Hit = SideHit;
			InOutResult.DirectionTag = MakeDirectionTag(Distance, false);

			// Optional confirm clamp using shared config dimensions (approximate BP overlap).
			if (Config && Config->ConfirmCapsuleRadius > 0.0f && Config->ConfirmCapsuleHalfHeight > 0.0f)
			{
				InOutResult.WorldLocation = SideHit.ImpactPoint;
				InOutResult.TargetLocation = SideHit.ImpactPoint;
				InOutResult.SurfaceNormal = SideHit.ImpactNormal;
				InOutResult.Hit = SideHit;
			}

			ComputeHandIKProbes(SideHit.ImpactPoint, SideHit.ImpactNormal, CapsuleHalfHeight, InOutResult);
			return true;
		}
	}

	// 2) Back hop away from wall (input/yaw/normal gated, optional clearance check)
	{
		const FVector InputDir = GetResolvedInputDirection();
		const float DeltaYaw = GetControlToActorDeltaYawDegrees(Character);
		const float WallDotUp = FMath::Abs(FVector::DotProduct(WallNormal, FVector::UpVector));
		const float VerticalTolerance = FMath::Sin(FMath::DegreesToRadians(FMath::Max(1.0f, BackHopAngleToleranceDegrees)));
		const bool bWallIsVerticalEnough = WallDotUp <= VerticalTolerance;

		if (bWallIsVerticalEnough && IsBackHopInputAllowed(InputDir, DeltaYaw))
		{
				auto TryBackHopDistance = [&](float Distance) -> bool
			{
				const TPair<FVector, FVector> Ends = BuildSweepEnds(Forward * -1.0f, Distance);
				FHitResult BackHit;
					if (TryConsumeTraceSlot() && RunSphereTraceWithOmniTrace(WorldContext, Ends.Key + Up * (HopVerticalLift + CharacterHeightDiff), Ends.Value + Up * (HopVerticalLift + CharacterHeightDiff), BackHopProbeRadius, ParkourTraceChannel, Character, BackHit))
				{
					if (bValidateBackHopSurface && !CheckBackHopSurfaceClear(BackHit.ImpactPoint, Forward * -1.0f, Character))
					{
						return false;
					}

					InOutResult.Reset();
					InOutResult.bHasResult = true;
					InOutResult.bIsValid = true;
					InOutResult.Action = ESOTS_ParkourAction::BackHop;
					InOutResult.ClimbStyle = ESOTS_ClimbStyle::Braced;
					InOutResult.ResultType = ESOTS_ParkourResultType::Drop;
					InOutResult.WorldLocation = BackHit.ImpactPoint;
					InOutResult.WorldNormal = BackHit.ImpactNormal;
					InOutResult.TargetLocation = BackHit.ImpactPoint;
					InOutResult.SurfaceNormal = BackHit.ImpactNormal;
					InOutResult.HeightDelta = BackHit.ImpactPoint.Z - (Character->GetActorLocation().Z - CapsuleHalfHeight);
					InOutResult.Hit = BackHit;
					InOutResult.DirectionTag = MakeDirectionTag(0.0f, true);
					ComputeHandIKProbes(BackHit.ImpactPoint, BackHit.ImpactNormal, CapsuleHalfHeight, InOutResult);
					return true;
				}
				return false;
			};

			if (TryBackHopDistance(BackHopDistance))
			{
				return true;
			}

			// Legacy inner back-hop distance for closer walls.
			if (BackHopInnerDistance > 0.0f && TryBackHopDistance(BackHopInnerDistance))
			{
				return true;
			}
		}
	}

	// 3) Corner-move / predictive hop grid (multi-pass)
	struct FHopTest
	{
		float RightOffset;
		float ForwardDistance;
		bool bPredictive;
	};

	TArray<FHopTest> HopTests;
	const int32 HalfQty = (ParkourState == ESOTS_ParkourState::Active) ? HorizontalWallTraceHalfQuantity_Climb : HorizontalWallTraceHalfQuantity_NotBusy;
	for (int32 Index = -HalfQty; Index <= HalfQty; ++Index)
	{
		const float RightOffset = HorizontalWallTraceRange * static_cast<float>(Index);
		HopTests.Add({RightOffset, 140.0f, false});
	}
	// Predictive forward band (slightly longer)
	HopTests.Add({HorizontalWallTraceRange * -HalfQty, 180.0f, true});
	HopTests.Add({0.0f, 200.0f, true});
	HopTests.Add({HorizontalWallTraceRange * HalfQty, 180.0f, true});

	const float ForwardBackOffset = -60.0f;
	const float DownDepthBase = FMath::Max(CapsuleHalfHeight * 2.0f + 80.0f, VerticalWallTraceRange * FMath::Max(1, (ParkourState == ESOTS_ParkourState::Active) ? VerticalWallTraceQuantity_Climb : VerticalWallTraceQuantity_NotBusy) * 0.5f);

	for (const FHopTest& Test : HopTests)
	{
		const FVector Lateral = Right * Test.RightOffset;
		const FVector Start = Base + Lateral + Forward * ForwardBackOffset + Up * HopVerticalLift;
		const FVector End = Start + Forward * Test.ForwardDistance;

		FHitResult ForwardHit;
		if (!SphereSweepWithFallback(Start, End, ForwardHit))
		{
			continue;
		}

		const float DownDepth = DownDepthBase * (Test.bPredictive ? 1.25f : 1.0f);
		const FVector DownStart = ForwardHit.ImpactPoint + Up * HopVerticalLift;
		const FVector DownEnd = DownStart - Up * DownDepth;
		FHitResult DownHit;
		if (!SphereSweepWithFallback(DownStart, DownEnd, DownHit))
		{
			continue;
		}

		InOutResult.Reset();
		InOutResult.bHasResult = true;
		InOutResult.bIsValid = true;
		InOutResult.Action = Test.bPredictive ? ESOTS_ParkourAction::PredictJump : ESOTS_ParkourAction::LedgeMove;
		InOutResult.ClimbStyle = ESOTS_ClimbStyle::Braced;
		InOutResult.ResultType = ESOTS_ParkourResultType::Mantle;
		InOutResult.WorldLocation = DownHit.ImpactPoint;
		InOutResult.WorldNormal = DownHit.ImpactNormal;
		InOutResult.TargetLocation = DownHit.ImpactPoint;
		InOutResult.SurfaceNormal = DownHit.ImpactNormal;
		InOutResult.HeightDelta = DownHit.ImpactPoint.Z - (Character->GetActorLocation().Z - CapsuleHalfHeight);
		InOutResult.Hit = DownHit;
		InOutResult.DirectionTag = MakeDirectionTag(Test.RightOffset, false);
		ComputeHandIKProbes(DownHit.ImpactPoint, DownHit.ImpactNormal, CapsuleHalfHeight, InOutResult);
		return true;
	}

	return false;
}

bool USOTS_ParkourComponent::TryDetectCornerMove(const FHitResult& FirstHit, float CapsuleHalfHeight, FSOTS_ParkourResult& InOutResult) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return false;
	}

	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return false;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	const bool bDrawCorner = Config ? Config->bDrawCornerDebug : true;

	const FVector WallNormal = FirstHit.ImpactNormal.IsNearlyZero() ? FVector::ForwardVector : FirstHit.ImpactNormal.GetSafeNormal();
	const FVector Forward = -WallNormal;
	const FVector Right = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();
	const FVector Up = FVector::UpVector;

	const float CornerSideStep = HorizontalWallTraceRange * 2.0f;
	const float CornerForwardDistance = LedgeMoveHorizontalDistance;
	const float CornerBackOffset = -40.0f;
	const int32 VerticalTraceQty = (ParkourState == ESOTS_ParkourState::Active) ? VerticalWallTraceQuantity_Climb : VerticalWallTraceQuantity_NotBusy;
	const float DownDepthBase = FMath::Max(CapsuleHalfHeight * 2.0f + 80.0f, VerticalWallTraceRange * FMath::Max(1, VerticalTraceQty) * 0.5f);

	for (int32 Side : { -1, 1 })
	{
		const float SideOffset = CornerSideStep * static_cast<float>(Side);
		const FVector Start = FirstHit.ImpactPoint + Up * (LedgeMoveVerticalOffset + CharacterHeightDiff) + Right * SideOffset + Forward * CornerBackOffset;
		const FVector End = Start + Forward * CornerForwardDistance + Right * SideOffset;

		if (!TryConsumeTraceSlot())
		{
			return false;
		}

		FHitResult CornerHit;
		const bool bCornerHit = RunSphereTraceWithOmniTrace(WorldContext, Start, End, LedgeMoveProbeRadius, ParkourTraceChannel, Character, CornerHit);
		DrawDebugLineIfEnabled(Start, End, bDrawCorner, bCornerHit ? FColor::Cyan : FColor::Red);
		if (!bCornerHit)
		{
			continue;
		}

		const FVector CornerNormal = CornerHit.ImpactNormal.GetSafeNormal();
		const float ParallelDot = FVector::DotProduct(CornerNormal, WallNormal);
		if (ParallelDot > 0.6f)
		{
			continue;
		}

		const FVector DownStart = CornerHit.ImpactPoint + Up * (HopVerticalLift + CharacterHeightDiff);
		const FVector DownEnd = DownStart - Up * DownDepthBase;

		if (!TryConsumeTraceSlot())
		{
			return false;
		}

		FHitResult DownHit;
		const bool bDownHit = RunSphereTraceWithOmniTrace(WorldContext, DownStart, DownEnd, LedgeMoveProbeRadius, ParkourTraceChannel, Character, DownHit);
		DrawDebugLineIfEnabled(DownStart, DownEnd, bDrawCorner, bDownHit ? FColor::Green : FColor::Red);
		if (!bDownHit)
		{
			continue;
		}

		DrawDebugPointIfEnabled(CornerHit.ImpactPoint, 8.0f, bDrawCorner, FColor::Cyan);
		DrawDebugPointIfEnabled(DownHit.ImpactPoint, 8.0f, bDrawCorner, FColor::Yellow);

		InOutResult.Reset();
		InOutResult.bHasResult = true;
		InOutResult.bIsValid = true;
		InOutResult.Action = ESOTS_ParkourAction::CornerMove;
		InOutResult.ClimbStyle = ESOTS_ClimbStyle::Braced;
		InOutResult.ResultType = ESOTS_ParkourResultType::Mantle;
		InOutResult.WorldLocation = DownHit.ImpactPoint;
		InOutResult.WorldNormal = DownHit.ImpactNormal;
		InOutResult.TargetLocation = DownHit.ImpactPoint;
		InOutResult.SurfaceNormal = CornerNormal.IsNearlyZero() ? DownHit.ImpactNormal : CornerNormal;
		InOutResult.HeightDelta = DownHit.ImpactPoint.Z - (Character->GetActorLocation().Z - CapsuleHalfHeight);
		InOutResult.Hit = DownHit;
		InOutResult.DirectionTag = MakeDirectionTag(SideOffset, false);
		ComputeHandIKProbes(DownHit.ImpactPoint, DownHit.ImpactNormal, CapsuleHalfHeight, InOutResult);

		UE_LOG(LogSOTS_Parkour, Verbose, TEXT("CornerMove: side=%d dot=%.2f target=%s"), Side, ParallelDot, *DownHit.ImpactPoint.ToString());
		return true;
	}

	return false;
}

void USOTS_ParkourComponent::UpdateStateMachine(float DeltaTime)
{
	(void)DeltaTime;

	const ESOTS_ParkourLogicalState DesiredState = EvaluateLogicalState();
	if (DesiredState != ParkourLogicalState)
	{
		SetParkourLogicalState(DesiredState);
	}
}

ESOTS_ParkourLogicalState USOTS_ParkourComponent::GetLogicalStateForAction(ESOTS_ParkourAction Action) const
{
	switch (Action)
	{
	case ESOTS_ParkourAction::Mantle:
	case ESOTS_ParkourAction::LowMantle:
	case ESOTS_ParkourAction::DistanceMantle:
		return ESOTS_ParkourLogicalState::Mantle;

	case ESOTS_ParkourAction::Vault:
	case ESOTS_ParkourAction::ThinVault:
	case ESOTS_ParkourAction::HighVault:
	case ESOTS_ParkourAction::VaultToBraced:
		return ESOTS_ParkourLogicalState::Vault;

	case ESOTS_ParkourAction::TicTac:
		return ESOTS_ParkourLogicalState::TicTac;

	case ESOTS_ParkourAction::LedgeMove:
	case ESOTS_ParkourAction::BackHop:
	case ESOTS_ParkourAction::PredictJump:
		return ESOTS_ParkourLogicalState::ReachLedge;

	case ESOTS_ParkourAction::CornerMove:
		return ESOTS_ParkourLogicalState::CornerMove;

	case ESOTS_ParkourAction::AirHang:
	case ESOTS_ParkourAction::Drop:
		return ESOTS_ParkourLogicalState::Climb;

	case ESOTS_ParkourAction::None:
	default:
		return ESOTS_ParkourLogicalState::NotBusy;
	}
}

ESOTS_ParkourLogicalState USOTS_ParkourComponent::EvaluateLogicalState() const
{
	if (ParkourState == ESOTS_ParkourState::Idle || ParkourState == ESOTS_ParkourState::Exiting)
	{
		return ESOTS_ParkourLogicalState::NotBusy;
	}

	if (!LastResult.bIsValid)
	{
		return ParkourLogicalState;
	}

	switch (LastResult.Action)
	{
	case ESOTS_ParkourAction::Mantle:
	case ESOTS_ParkourAction::LowMantle:
	case ESOTS_ParkourAction::DistanceMantle:
		return ESOTS_ParkourLogicalState::Mantle;

	case ESOTS_ParkourAction::Vault:
	case ESOTS_ParkourAction::ThinVault:
	case ESOTS_ParkourAction::HighVault:
	case ESOTS_ParkourAction::VaultToBraced:
		return ESOTS_ParkourLogicalState::Vault;

	case ESOTS_ParkourAction::TicTac:
		return ESOTS_ParkourLogicalState::TicTac;

	case ESOTS_ParkourAction::LedgeMove:
	case ESOTS_ParkourAction::BackHop:
	case ESOTS_ParkourAction::PredictJump:
		return ESOTS_ParkourLogicalState::ReachLedge;

	case ESOTS_ParkourAction::CornerMove:
		return ESOTS_ParkourLogicalState::CornerMove;

	case ESOTS_ParkourAction::AirHang:
	case ESOTS_ParkourAction::Drop:
	default:
		return ESOTS_ParkourLogicalState::Climb;
	}
}

float USOTS_ParkourComponent::GetControlToActorDeltaYawDegrees(const ACharacter* Character) const
{
	if (!Character)
	{
		return 0.0f;
	}

	const AController* Controller = Character->GetController();
	const float ControlYaw = Controller ? Controller->GetControlRotation().Yaw : Character->GetActorRotation().Yaw;
	const float ActorYaw = Character->GetActorRotation().Yaw;
	return FMath::UnwindDegrees(ControlYaw - ActorYaw);
}

bool USOTS_ParkourComponent::IsBackHopInputAllowed(const FVector& InInputDirection, float DeltaYawDegrees) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return false;
	}

	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = Character->GetActorRightVector().GetSafeNormal2D();
	const FVector Input2D = InInputDirection.GetSafeNormal2D();

	const float ForwardMag = FVector::DotProduct(Input2D, Forward);
	const float RightMag = FVector::DotProduct(Input2D, Right);
	const float AbsDeltaYaw = FMath::Abs(FMath::UnwindDegrees(DeltaYawDegrees));

	// Strafe-based back hop: require lateral input and yaw outside the center deadzone but within limits.
	const bool bYawWithinWindow = DeltaYawDegrees >= BackHopYawMinDegrees && DeltaYawDegrees <= BackHopYawMaxDegrees;
	const bool bOutsideCenter = AbsDeltaYaw >= BackHopYawCenterDeadzoneDegrees;
	const bool bStrafeInput = FMath::Abs(RightMag) >= BackHopRightInputThreshold;
	const bool bStrafeAllowed = bStrafeInput && bYawWithinWindow && bOutsideCenter;

	// Straight-back hop: require forward input and a large yaw delta away from facing.
	const bool bForwardInput = ForwardMag >= BackHopForwardInputThreshold;
	const bool bForwardYaw = AbsDeltaYaw >= BackHopForwardYawRejectDegrees;
	const bool bForwardAllowed = bForwardInput && bForwardYaw;

	return bStrafeAllowed || bForwardAllowed;
}

bool USOTS_ParkourComponent::CheckBackHopSurfaceClear(const FVector& WarpPoint, const FVector& Forward, const ACharacter* Character) const
{
	if (!Character)
	{
		return false;
	}

	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return false;
	}

	const FVector CapsuleCenter = WarpPoint + Forward.GetSafeNormal() * BackHopSurfaceForwardOffset + FVector(0.0f, 0.0f, BackHopSurfaceCapsuleHalfHeight);
	FHitResult ClearanceHit;
	const bool bHit = TryConsumeTraceSlot() && RunCapsuleTraceWithOmniTrace(
		WorldContext,
		CapsuleCenter,
		CapsuleCenter,
		BackHopSurfaceCapsuleRadius,
		BackHopSurfaceCapsuleHalfHeight,
		ParkourTraceChannel,
		Character,
		ClearanceHit);

	const bool bDraw = GetConfig() ? GetConfig()->bDrawLedgeAndBackHopDebug : true;
	DrawDebugCapsuleIfEnabled(CapsuleCenter, BackHopSurfaceCapsuleHalfHeight, BackHopSurfaceCapsuleRadius, FQuat::Identity, bDraw, bHit ? FColor::Red : FColor::Green);

	return !bHit || !ClearanceHit.IsValidBlockingHit();
}

bool USOTS_ParkourComponent::TriggerAutoClimbJump(const FString& Reason) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character || !bCanAutoClimb)
	{
		return false;
	}

	if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Verbose, TEXT("AutoClimb fallback (SetJump) triggered: %s"), *Reason);
	}

	const_cast<ACharacter*>(Character)->Jump();
	return true;
}

void USOTS_ParkourComponent::SetParkourLogicalState(ESOTS_ParkourLogicalState NewLogicalState)
{
	if (ParkourLogicalState == NewLogicalState)
	{
		return;
	}

	const ESOTS_ParkourLogicalState PreviousState = ParkourLogicalState;
	if (NewLogicalState == ESOTS_ParkourLogicalState::Climb && PreviousState != ESOTS_ParkourLogicalState::Climb)
	{
		bSkipFootIKTraceOnNextCall = true;
		ConsecutiveLeftHandIKFailures = 0;
		ConsecutiveRightHandIKFailures = 0;
		bSkipHandIKTraceOnNextCall = false;
		HandIKCooldownFramesRemaining = 0;
	}
	else if (NewLogicalState != ESOTS_ParkourLogicalState::Climb)
	{
		bSkipFootIKTraceOnNextCall = false;
		ClimbTraceCooldownTime = 0.0f;
		ConsecutiveLeftHandIKFailures = 0;
		ConsecutiveRightHandIKFailures = 0;
		bSkipHandIKTraceOnNextCall = false;
		HandIKCooldownFramesRemaining = 0;
	}

	ParkourLogicalState = NewLogicalState;
	UpdateGameplayTagsFromState(ParkourLogicalState);
	HandleCameraTimelineForStateChange(PreviousState, ParkourLogicalState);
}

void USOTS_ParkourComponent::HandleCameraTimelineForStateChange(ESOTS_ParkourLogicalState PreviousState, ESOTS_ParkourLogicalState NewState)
{
	(void)PreviousState;

	if (NewState == ESOTS_ParkourLogicalState::BeamHidden)
	{
		return;
	}

	// BP parity: only drive the camera timeline when entering ReachLedge or returning to NotBusy.
	if (NewState != ESOTS_ParkourLogicalState::ReachLedge && NewState != ESOTS_ParkourLogicalState::NotBusy)
	{
		return;
	}

	if (!CameraBoom && CachedOwnerCharacter.IsValid())
	{
		CameraBoom = CachedOwnerCharacter->FindComponentByClass<USpringArmComponent>();
	}

	UCurveFloat* Curve = CameraMoveCurve.LoadSynchronous();
	if (!CameraBoom || !Curve)
	{
		return;
	}

	TargetRelativeCameraLocation = FirstCameraRelativeLocation;

	const float DesiredArmLength = (CameraTargetArmLength > KINDA_SMALL_NUMBER)
		? CameraTargetArmLength
		: CameraBoom->TargetArmLength;
	LocalTargetLengthDiff = DesiredArmLength - CameraBoom->TargetArmLength;

	AddCameraTimeline(CameraTimelineDuration);
}

void USOTS_ParkourComponent::AddCameraTimeline(float TimeOverride)
{
	UWorld* World = GetWorld();
	UCurveFloat* Curve = CameraMoveCurve.LoadSynchronous();
	if (!World || !CameraBoom || !Curve)
	{
		return;
	}

	CameraTimelineStartLocation = CameraBoom->GetRelativeLocation();
	CameraTimelineStartArmLength = CameraBoom->TargetArmLength;
	CameraCurveAlpha = 0.0f;
	CameraTimelineElapsed = 0.0f;

	float Duration = (TimeOverride > 0.0f) ? TimeOverride : CameraTimelineDuration;
	if (Duration <= 0.0f && Curve->FloatCurve.GetNumKeys() > 0)
	{
		Duration = Curve->FloatCurve.GetLastKey().Time;
	}

	if (Duration <= 0.0f)
	{
		return;
	}

	CameraTimelineActiveDuration = Duration;

	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.ClearTimer(CameraTimelineHandle);

	const float TickInterval = FMath::Max(World->GetDeltaSeconds(), KINDA_SMALL_NUMBER);
	TimerManager.SetTimer(CameraTimelineHandle, this, &USOTS_ParkourComponent::CameraTimelineTick, TickInterval, true);
	bCameraTimelineActive = true;
}

void USOTS_ParkourComponent::CameraTimelineTick()
{
	UWorld* World = GetWorld();
	UCurveFloat* Curve = CameraMoveCurve.LoadSynchronous();
	if (!World || !CameraBoom || !Curve || !bCameraTimelineActive)
	{
		FinishCameraTimeline();
		return;
	}

	const FTimerManager& TimerManager = World->GetTimerManager();
	if (!TimerManager.IsTimerActive(CameraTimelineHandle))
	{
		FinishCameraTimeline();
		return;
	}

	const float Duration = CameraTimelineActiveDuration > 0.0f ? CameraTimelineActiveDuration : CameraTimelineDuration;
	CameraTimelineElapsed = TimerManager.GetTimerElapsed(CameraTimelineHandle);

	const float EvalTime = Duration > 0.0f ? FMath::Clamp(CameraTimelineElapsed, 0.0f, Duration) : CameraTimelineElapsed;
	CameraCurveAlpha = Curve->GetFloatValue(EvalTime);

	const FVector LocationDelta = TargetRelativeCameraLocation - CameraTimelineStartLocation;
	const FVector NewRelativeLocation = CameraTimelineStartLocation + LocationDelta * CameraCurveAlpha;
	CameraBoom->SetRelativeLocation(NewRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);

	const float NewArmLength = CameraTimelineStartArmLength + LocalTargetLengthDiff * CameraCurveAlpha;
	CameraBoom->TargetArmLength = NewArmLength;

	if (CameraTimelineElapsed >= Duration)
	{
		FinishCameraTimeline();
	}
}

void USOTS_ParkourComponent::FinishCameraTimeline()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CameraTimelineHandle);
	}

	bCameraTimelineActive = false;
	CameraCurveAlpha = 0.0f;
	CameraTimelineElapsed = 0.0f;

	if (CameraBoom)
	{
		const float FinalArmLength = CameraTimelineStartArmLength + LocalTargetLengthDiff;
		CameraBoom->SetRelativeLocation(TargetRelativeCameraLocation, false, nullptr, ETeleportType::TeleportPhysics);
		CameraBoom->TargetArmLength = FinalArmLength;
	}
}

void USOTS_ParkourComponent::UpdateGameplayTagsFromState(ESOTS_ParkourLogicalState NewState)
{
	const FGameplayTag NewStateTag = GetStateTag(NewState);
	ApplyTagDelta(ActiveStateTag, NewStateTag);
	ActiveStateTag = NewStateTag;
}

void USOTS_ParkourComponent::UpdateGameplayTagsFromResult(const FSOTS_ParkourResult& Result)
{
	const bool bHasMeaningfulResult = Result.bHasResult || Result.bIsValid;

	const FGameplayTag NewActionTag = bHasMeaningfulResult ? GetActionTag(Result.Action) : FGameplayTag();
	const FGameplayTag NewClimbStyleTag = bHasMeaningfulResult ? GetClimbStyleTag(Result.ClimbStyle) : FGameplayTag();
	const FGameplayTag NewDirectionTag = bHasMeaningfulResult ? Result.DirectionTag : FGameplayTag();

	ApplyTagDelta(ActiveActionTag, NewActionTag);
	ApplyTagDelta(ActiveClimbStyleTag, NewClimbStyleTag);
	ApplyTagDelta(ActiveDirectionTag, NewDirectionTag);

	ActiveActionTag = NewActionTag;
	ActiveClimbStyleTag = NewClimbStyleTag;
	ActiveDirectionTag = NewDirectionTag;
}

void USOTS_ParkourComponent::ApplyTagDelta(const FGameplayTag& OldTag, const FGameplayTag& NewTag) const
{
	if (OldTag == NewTag)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(Owner))
	{
		if (OldTag.IsValid())
		{
			TagSubsystem->RemoveTagFromActor(Owner, OldTag);
		}

		if (NewTag.IsValid())
		{
			TagSubsystem->AddTagToActor(Owner, NewTag);
		}
	}
}

	// CGF parity: resolve parkour tags through the global tag spine instead of local RequestGameplayTag.
	FGameplayTag USOTS_ParkourComponent::ResolveParkourTagByName(const FName& TagName) const
	{
		if (TagName.IsNone())
		{
			return FGameplayTag();
		}

		if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
		{
			return TagSubsystem->GetTagByName(TagName);
		}

		return FGameplayTag::RequestGameplayTag(TagName, false);
	}

FGameplayTag USOTS_ParkourComponent::GetStateTag(ESOTS_ParkourLogicalState State) const
{
	switch (State)
	{
	case ESOTS_ParkourLogicalState::NotBusy:
			return ResolveParkourTagByName(TEXT("Parkour.State.NotBusy"));
	case ESOTS_ParkourLogicalState::Mantle:
			return ResolveParkourTagByName(TEXT("Parkour.State.Mantle"));
	case ESOTS_ParkourLogicalState::Vault:
			return ResolveParkourTagByName(TEXT("Parkour.State.Vault"));
	case ESOTS_ParkourLogicalState::Climb:
			return ResolveParkourTagByName(TEXT("Parkour.State.Climb"));
	case ESOTS_ParkourLogicalState::ReachLedge:
			return ResolveParkourTagByName(TEXT("Parkour.State.ReachLedge"));
	case ESOTS_ParkourLogicalState::TicTac:
			return ResolveParkourTagByName(TEXT("Parkour.State.TicTac"));
	case ESOTS_ParkourLogicalState::CornerMove:
			return ResolveParkourTagByName(TEXT("Parkour.State.CornerMove"));
	case ESOTS_ParkourLogicalState::BeamHidden:
			return ResolveParkourTagByName(TEXT("Parkour.State.BeamHidden"));
	default:
		return FGameplayTag();
	}
}

FGameplayTag USOTS_ParkourComponent::GetActionTag(ESOTS_ParkourAction Action) const
{
	switch (Action)
	{
	case ESOTS_ParkourAction::Mantle:
	case ESOTS_ParkourAction::LowMantle:
			return ResolveParkourTagByName(TEXT("Parkour.Action.Mantle"));
	case ESOTS_ParkourAction::DistanceMantle:
			return ResolveParkourTagByName(TEXT("Parkour.Action.DistanceMantle"));
	case ESOTS_ParkourAction::Vault:
	case ESOTS_ParkourAction::ThinVault:
	case ESOTS_ParkourAction::HighVault:
	case ESOTS_ParkourAction::VaultToBraced:
			return ResolveParkourTagByName(TEXT("Parkour.Action.Vault"));
	case ESOTS_ParkourAction::LedgeMove:
			return ResolveParkourTagByName(TEXT("Parkour.Action.LedgeMove"));
	case ESOTS_ParkourAction::Drop:
			return ResolveParkourTagByName(TEXT("Parkour.Action.Drop"));
	case ESOTS_ParkourAction::TicTac:
			return ResolveParkourTagByName(TEXT("Parkour.Action.TicTac"));
	case ESOTS_ParkourAction::BackHop:
			return ResolveParkourTagByName(TEXT("Parkour.Action.BackHop"));
	case ESOTS_ParkourAction::PredictJump:
			return ResolveParkourTagByName(TEXT("Parkour.Action.PredictJump"));
	case ESOTS_ParkourAction::CornerMove:
			return ResolveParkourTagByName(TEXT("Parkour.Action.CornerMove"));
	case ESOTS_ParkourAction::AirHang:
			return ResolveParkourTagByName(TEXT("Parkour.Action.DistanceIdleToFreeHang"));
	default:
		return FGameplayTag();
	}
}

FGameplayTag USOTS_ParkourComponent::GetClimbStyleTag(ESOTS_ClimbStyle ClimbStyle) const
{
	switch (ClimbStyle)
	{
	case ESOTS_ClimbStyle::FreeHang:
		return ResolveParkourTagByName(TEXT("Parkour.ClimbStyle.FreeHang"));
	case ESOTS_ClimbStyle::Braced:
		return ResolveParkourTagByName(TEXT("Parkour.ClimbStyle.Braced"));
	default:
		return FGameplayTag();
	}
}

bool USOTS_ParkourComponent::RunWallGridProbes(const FHitResult& FirstHit, float CapsuleHalfHeight, FHitResult& OutBestHit, float& OutHeightDelta, float& OutXYDistance, float& OutWallDepth) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character || !CachedMovementComponent)
	{
		return false;
	}

	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return false;
	}

	const int32 HalfQty = (ParkourState == ESOTS_ParkourState::Active)
		? HorizontalWallTraceHalfQuantity_Climb
		: HorizontalWallTraceHalfQuantity_NotBusy;
	const int32 VerticalQty = (ParkourState == ESOTS_ParkourState::Active)
		? VerticalWallTraceQuantity_Climb
		: VerticalWallTraceQuantity_NotBusy;

	if (HalfQty <= 0 || VerticalQty <= 0 || HorizontalWallTraceRange <= 0.0f || VerticalWallTraceRange <= 0.0f)
	{
		return false;
	}

	const FVector CharacterLocation = Character->GetActorLocation();
	const float FeetZ = CharacterLocation.Z - CapsuleHalfHeight;
	const USOTS_ParkourConfig* Config = GetConfig();
	const bool bDrawLedgeAndBackHop = Config ? Config->bDrawLedgeAndBackHopDebug : true;

	const FVector TraceDir = (FirstHit.TraceEnd - FirstHit.TraceStart).GetSafeNormal();
	const FVector WallNormal = FirstHit.ImpactNormal.IsNearlyZero() ? FVector::ForwardVector : FirstHit.ImpactNormal.GetSafeNormal();
	FVector Forward = !TraceDir.IsNearlyZero() ? TraceDir : -WallNormal;
	if (Forward.IsNearlyZero())
	{
		Forward = Character->GetActorForwardVector();
	}

	FVector Right = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();
	if (Right.IsNearlyZero())
	{
		Right = FVector::RightVector;
	}

	const float TraceDistance = FMath::Max(FVector::Dist(FirstHit.TraceStart, FirstHit.TraceEnd), 50.0f);
	const float StartHeight = GetVerticalDetectStartHeight();
	const float ProbeRadius = LedgeMoveProbeRadius > 0.0f ? LedgeMoveProbeRadius : 6.0f;
	const float ForwardBackOffset = -10.0f;

	bool bFound = false;
	float BestHeight = -MAX_flt;
	float BestDistance2D = MAX_flt;
	float BestDepth = MAX_flt;

	for (int32 HorzIndex = -HalfQty; HorzIndex <= HalfQty; ++HorzIndex)
	{
		const float Lateral = HorizontalWallTraceRange * static_cast<float>(HorzIndex);
		for (int32 VertIndex = 0; VertIndex < VerticalQty; ++VertIndex)
		{
			const float Z = StartHeight + VerticalWallTraceRange * static_cast<float>(VertIndex);

			FVector Start = CharacterLocation + Right * Lateral + Forward * ForwardBackOffset;
			Start.Z = Z;
			const FVector End = Start + Forward * TraceDistance;

			FHitResult GridHit;

			bool bHit = false;
			if (bUseWallGridOverlap)
			{
				// Parity toggle: when enabled we do overlap-only samples at the probe point; otherwise fall back to sweeps along Forward.
				if (!TryConsumeTraceSlot())
				{
					return bFound;
				}

				bHit = RunSphereTraceWithOmniTrace(WorldContext, Start, Start, ProbeRadius, ParkourTraceChannel, Character, GridHit);
			}

			if (!bHit)
			{
				if (!TryConsumeTraceSlot())
				{
					return bFound;
				}

				if (!RunSphereTraceWithOmniTrace(WorldContext, Start, End, ProbeRadius, ParkourTraceChannel, Character, GridHit))
				{
					continue;
				}

				bHit = true;
			}

			const float HeightDelta = GridHit.ImpactPoint.Z - FeetZ;
			const float XYDistance = FVector::Dist2D(CharacterLocation, GridHit.ImpactPoint);
			const float Depth = FVector::Dist(Start, GridHit.ImpactPoint);

			if (HeightDelta > BestHeight || (FMath::IsNearlyEqual(HeightDelta, BestHeight, 1.0f) && XYDistance < BestDistance2D))
			{
				BestHeight = HeightDelta;
				BestDistance2D = XYDistance;
				BestDepth = Depth;
				OutBestHit = GridHit;
				OutHeightDelta = HeightDelta;
				OutXYDistance = XYDistance;
				OutWallDepth = Depth;
				bFound = true;
			}


			DrawDebugLineIfEnabled(Start, End, bDrawLedgeAndBackHop, FColor::Emerald);
		}
	}

	return bFound;
}

void USOTS_ParkourComponent::ComputeHandIKProbes(const FVector& SurfacePoint, const FVector& SurfaceNormal, float CapsuleHalfHeight, FSOTS_ParkourResult& InOutResult) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		return;
	}

	// Optional cooldown: skip hand IK for a few frames after repeated misses to mirror BP disable-after-N behavior.
	if (HandIKCooldownFramesRemaining > 0)
	{
		--HandIKCooldownFramesRemaining;
		InOutResult.bHasLeftHandSurface = false;
		InOutResult.bHasRightHandSurface = false;
		InOutResult.LeftHandHit = FHitResult();
		InOutResult.RightHandHit = FHitResult();
		return;
	}

	const bool bAllowHandIK = ParkourLogicalState == ESOTS_ParkourLogicalState::Climb || ParkourLogicalState == ESOTS_ParkourLogicalState::ReachLedge;
	if (!bAllowHandIK)
	{
		InOutResult.bHasLeftHandSurface = false;
		InOutResult.bHasRightHandSurface = false;
		InOutResult.LeftHandHit = FHitResult();
		InOutResult.RightHandHit = FHitResult();
		return;
	}

	if (bSkipHandIKTraceOnNextCall)
	{
		bSkipHandIKTraceOnNextCall = false;
		HandIKCooldownFramesRemaining = FMath::Max(HandIKCooldownFramesRemaining, HandIKMissCooldownFrames);
		InOutResult.bHasLeftHandSurface = false;
		InOutResult.bHasRightHandSurface = false;
		InOutResult.LeftHandHit = FHitResult();
		InOutResult.RightHandHit = FHitResult();
		return;
	}

	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return;
	}

	const USOTS_ParkourConfig* Config = GetConfig();
	const bool bDrawIK = Config ? Config->bDrawIKDebug : true;

	const FVector Up = FVector::UpVector;
	const FVector WallNormal = SurfaceNormal.IsNearlyZero() ? FVector::ForwardVector : SurfaceNormal.GetSafeNormal();
	FVector Tangent = FVector::CrossProduct(Up, WallNormal).GetSafeNormal();
	if (Tangent.IsNearlyZero())
	{
		Tangent = FVector::RightVector;
	}

	const FVector AdjustedSurfacePoint = SurfacePoint + Up * CharacterHeightDiff;

	const float PrimaryProbeRadius = HandIKProbeRadius > 0.0f ? HandIKProbeRadius : 15.0f; // BP radius=15 fallback
	const float SecondaryProbeRadius = 2.5f; // BP secondary refinement radius
	const TArray<float> ProbeRadii = {PrimaryProbeRadius};
	const float TraceDepth = (HandIKTraceDepth > 0.0f) ? HandIKTraceDepth : 50.0f; // BP primary depth ~50
	const float SecondaryTraceDepth = 50.0f;
	const float SecondaryStartLift = 5.0f;
	const float VerticalLift = HandIKVerticalLift;
	const auto ResolveHorizontalOffset = [&](float SideSpecificOffset)
	{
		if (SideSpecificOffset > 0.0f)
		{
			return SideSpecificOffset;
		}

		return HandIKHorizontalOffset > 0.0f ? HandIKHorizontalOffset : 8.0f;
	};

	const auto BuildOffsets = [](float BaseOffset)
	{
		TArray<float> Offsets;
		Offsets.Reserve(5);
		const float ClampedBase = FMath::Max(0.0f, BaseOffset);
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const float Offset = FMath::Max(0.0f, ClampedBase - 2.0f * static_cast<float>(Index));
			Offsets.AddUnique(Offset);
		}
		return Offsets;
	};

	const TArray<float> LeftOffsets = BuildOffsets(ResolveHorizontalOffset(HandIKHorizontalOffsetLeft));
	const TArray<float> RightOffsets = BuildOffsets(ResolveHorizontalOffset(HandIKHorizontalOffsetRight));

	auto RefineHitAtContact = [&](FHitResult& Hit)
	{
		if (!Hit.IsValidBlockingHit())
		{
			return;
		}

		const FVector SecondaryStart = Hit.ImpactPoint + Up * SecondaryStartLift;
		const FVector SecondaryEnd = SecondaryStart - Up * SecondaryTraceDepth;

		FHitResult SecondaryHit;
		if (TryConsumeTraceSlot() && RunSphereTraceWithOmniTrace(WorldContext, SecondaryStart, SecondaryEnd, SecondaryProbeRadius, ParkourTraceChannel, Character, SecondaryHit))
		{
			if (SecondaryHit.bBlockingHit)
			{
				Hit = SecondaryHit;
			}
		}
	};

	auto TraceHand = [&](int32 Side, const TArray<float>& Offsets, FHitResult& OutHit) -> bool
	{
		for (float Offset : Offsets)
		{
			const FVector Start = AdjustedSurfacePoint + Tangent * Offset * static_cast<float>(Side) + Up * VerticalLift;
			const FVector End = Start - Up * TraceDepth;

			for (float Radius : ProbeRadii)
			{
				FHitResult Hit;
				if (!TryConsumeTraceSlot())
				{
					return false;
				}

				if (!RunSphereTraceWithOmniTrace(WorldContext, Start, End, Radius, ParkourTraceChannel, Character, Hit))
				{
					continue;
				}

				const FVector HitNormal = Hit.ImpactNormal.IsNearlyZero() ? WallNormal : Hit.ImpactNormal.GetSafeNormal();
				const float Align = FVector::DotProduct(HitNormal, WallNormal);
				if (Align < HandIKNormalDotThreshold)
				{
					continue;
				}

				OutHit = Hit;
				RefineHitAtContact(OutHit);

				// BP parity: nudge contact point slightly along the surface normal for stability.
				if (!OutHit.ImpactNormal.IsNearlyZero())
				{
					const FVector Nudge = OutHit.ImpactNormal.GetSafeNormal() * 3.0f;
					OutHit.ImpactPoint += Nudge;
					OutHit.Location = OutHit.ImpactPoint;
				}


				const FColor Color = Radius >= PrimaryProbeRadius ? FColor::Orange : FColor::Yellow;
				DrawDebugLineIfEnabled(Start, End, bDrawIK, Color);
				DrawDebugSphereIfEnabled(Hit.ImpactPoint, Radius, bDrawIK, Color);

				return true;
			}
		}

		return false;
	};

	InOutResult.bHasLeftHandSurface = TraceHand(-1, LeftOffsets, InOutResult.LeftHandHit);
	if (!InOutResult.bHasLeftHandSurface)
	{
		InOutResult.LeftHandHit = FHitResult();
	}

	InOutResult.bHasRightHandSurface = TraceHand(1, RightOffsets, InOutResult.RightHandHit);
	if (!InOutResult.bHasRightHandSurface)
	{
		InOutResult.RightHandHit = FHitResult();
	}

	const int32 MaxFailures = HandIKMaxConsecutiveFailures;
	const float FailureOffset = HandIKFailureOffsetZ;

	ConsecutiveLeftHandIKFailures = InOutResult.bHasLeftHandSurface ? 0 : ConsecutiveLeftHandIKFailures + 1;
	ConsecutiveRightHandIKFailures = InOutResult.bHasRightHandSurface ? 0 : ConsecutiveRightHandIKFailures + 1;
	if (InOutResult.bHasLeftHandSurface || InOutResult.bHasRightHandSurface)
	{
		HandIKCooldownFramesRemaining = 0;
	}

	const int32 MaxHandMissFrames = Config ? Config->HandIKMaxConsecutiveMissFrames : 2;
	const int32 MissCooldownFrames = Config ? Config->HandIKMissCooldownFrames : HandIKMissCooldownFrames;
	if (MaxHandMissFrames > 0 && !InOutResult.bHasLeftHandSurface && !InOutResult.bHasRightHandSurface)
	{
		if (ConsecutiveLeftHandIKFailures >= MaxHandMissFrames && ConsecutiveRightHandIKFailures >= MaxHandMissFrames)
		{
			bSkipHandIKTraceOnNextCall = true;
			HandIKCooldownFramesRemaining = FMath::Max(HandIKCooldownFramesRemaining, MissCooldownFrames);
		}
	}

	if (InOutResult.bHasLeftHandSurface)
	{
		InOutResult.ClimbLeftHandOffsetZ = 0.0f;
	}
	else if (MaxFailures > 0 && ConsecutiveLeftHandIKFailures >= MaxFailures)
	{
		InOutResult.ClimbLeftHandOffsetZ = FailureOffset;
	}

	if (InOutResult.bHasRightHandSurface)
	{
		InOutResult.ClimbRightHandOffsetZ = 0.0f;
	}
	else if (MaxFailures > 0 && ConsecutiveRightHandIKFailures >= MaxFailures)
	{
		InOutResult.ClimbRightHandOffsetZ = FailureOffset;
	}
}

void USOTS_ParkourComponent::ComputeClimbIKTargets(FSOTS_ParkourResult& InOutResult) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character)
	{
		InOutResult.bEnableLeftClimbIK = false;
		InOutResult.bEnableRightClimbIK = false;
		return;
	}

	const FGameplayTag ClimbStateTag = FGameplayTag::RequestGameplayTag(TEXT("Parkour.State.Climb"), false);
	const bool bStateSaysClimb = InOutResult.StateTag.IsValid() && InOutResult.StateTag.MatchesTagExact(ClimbStateTag);
	const bool bLogicalClimb = ParkourLogicalState == ESOTS_ParkourLogicalState::Climb || ParkourLogicalState == ESOTS_ParkourLogicalState::ReachLedge;
	if (!bStateSaysClimb && !bLogicalClimb)
	{
		InOutResult.bEnableLeftClimbIK = false;
		InOutResult.bEnableRightClimbIK = false;
		return;
	}

	const FHitResult& AnchorHit = InOutResult.bHasClimbLedgeResult && InOutResult.ClimbLedgeResult.IsValidBlockingHit()
		? InOutResult.ClimbLedgeResult
		: (InOutResult.Hit.IsValidBlockingHit() ? InOutResult.Hit : (InOutResult.LeftHandHit.IsValidBlockingHit() ? InOutResult.LeftHandHit : InOutResult.RightHandHit));

	if (!AnchorHit.IsValidBlockingHit())
	{
		InOutResult.bEnableLeftClimbIK = false;
		InOutResult.bEnableRightClimbIK = false;
		return;
	}

	FVector WallNormal = AnchorHit.ImpactNormal.IsNearlyZero() ? FVector::ForwardVector : AnchorHit.ImpactNormal.GetSafeNormal();
	if (WallNormal.IsNearlyZero())
	{
		WallNormal = FVector::ForwardVector;
	}
	const FVector Forward = -WallNormal;
	FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	if (!Right.Normalize())
	{
		Right = Character->GetActorRightVector();
	}
	const FVector Up = FVector::UpVector;

	FVector Base = AnchorHit.ImpactPoint;
	if (!InOutResult.TargetLocation.IsNearlyZero())
	{
		Base = static_cast<FVector>(InOutResult.TargetLocation);
	}

	Base += FVector::UpVector * CharacterHeightDiff;

	auto AssignHand = [&](int32 Side, bool& bHasSurface, FHitResult& OutHit)
	{
		const float HandOffsetZ = (Side < 0 ? InOutResult.ClimbLeftHandOffsetZ : InOutResult.ClimbRightHandOffsetZ);
		const FVector Target = Base
			+ Right * (ClimbIKHandSpacing * static_cast<float>(Side))
			+ Forward * ClimbIKForwardOffset
			+ Up * (ClimbIKUpOffset + HandOffsetZ);

		if (bHasSurface)
		{
			return; // Preserve existing traced contact.
		}

		OutHit = AnchorHit;
		OutHit.ImpactPoint = Target;
		OutHit.Location = Target;
		OutHit.ImpactNormal = WallNormal;
		OutHit.Normal = WallNormal;
		OutHit.bBlockingHit = true;
		bHasSurface = true;
	};

	AssignHand(-1, InOutResult.bHasLeftHandSurface, InOutResult.LeftHandHit);
	AssignHand(1, InOutResult.bHasRightHandSurface, InOutResult.RightHandHit);

	auto BuildBaseHandTargets = [](const FHitResult& Hit, FVector& OutLoc, FRotator& OutRot)
	{
		if (!Hit.IsValidBlockingHit())
		{
			OutLoc = FVector::ZeroVector;
			OutRot = FRotator::ZeroRotator;
			return;
		}

		const FVector Normal = Hit.ImpactNormal.IsNearlyZero() ? FVector::ForwardVector : Hit.ImpactNormal.GetSafeNormal();
		const FVector Forward = -Normal; // ReverseRotationZ forward

		FVector Location = Hit.ImpactPoint;
		Location += Normal * 3.0f; // BP forward*3 nudge
		Location.Z -= 9.0f; // BP pre-MP-setter Z adjustment

		OutLoc = Location;
		OutRot = FRotationMatrix::MakeFromX(Forward).Rotator();
	};

	BuildBaseHandTargets(InOutResult.LeftHandHit, InOutResult.LeftHandBaseLocation, InOutResult.LeftHandBaseRotation);
	BuildBaseHandTargets(InOutResult.RightHandHit, InOutResult.RightHandBaseLocation, InOutResult.RightHandBaseRotation);

	InOutResult.bEnableLeftClimbIK = true;
	InOutResult.bEnableRightClimbIK = true;
}

void USOTS_ParkourComponent::ComputeFootIKProbes(FSOTS_ParkourResult& InOutResult) const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Character || !Mesh)
	{
		InOutResult.bHasLeftFootSurface = false;
		InOutResult.bHasRightFootSurface = false;
		InOutResult.LeftFootHit = FHitResult();
		InOutResult.RightFootHit = FHitResult();
		return;
	}

	UObject* WorldContext = GetOwner()
		? static_cast<UObject*>(GetOwner())
		: static_cast<UObject*>(const_cast<USOTS_ParkourComponent*>(this));

	if (!WorldContext)
	{
		return;
	}

	const USOTS_ParkourConfig* Config = GetConfig();

	// BP parity: skip the very first sweep after entering climb.
	if (bSkipFootIKTraceOnNextCall)
	{
		bSkipFootIKTraceOnNextCall = false;
		InOutResult.bHasLeftFootSurface = false;
		InOutResult.bHasRightFootSurface = false;
		InOutResult.LeftFootHit = FHitResult();
		InOutResult.RightFootHit = FHitResult();
		return;
	}

	const FGameplayTag ClimbStateTag = FGameplayTag::RequestGameplayTag(TEXT("Parkour.State.Climb"), false);
	const bool bClimbTagActive = InOutResult.StateTag.IsValid() && InOutResult.StateTag.MatchesTagExact(ClimbStateTag);
	const bool bAllowFootIK = bClimbTagActive || ParkourLogicalState == ESOTS_ParkourLogicalState::Climb;
	if (!bAllowFootIK)
	{
		InOutResult.bHasLeftFootSurface = false;
		InOutResult.bHasRightFootSurface = false;
		InOutResult.LeftFootHit = FHitResult();
		InOutResult.RightFootHit = FHitResult();
		return;
	}

	const FVector Forward = Character->GetActorForwardVector();
	const FVector Right = Character->GetActorRightVector();
	const FVector Up = FVector::UpVector;
	const FGameplayTag CornerMoveTag = FGameplayTag::RequestGameplayTag(TEXT("Parkour.Action.CornerMove"), false);
	const bool bIsCornerMove = InOutResult.ActionTag.IsValid() && InOutResult.ActionTag.MatchesTagExact(CornerMoveTag);
	const bool bDrawIK = Config ? Config->bDrawIKDebug : true;
	const float PrimaryProbeRadius = FootIKProbeRadius > 0.0f ? FootIKProbeRadius : 6.0f;
	const float CornerProbeRadius = FootIKCornerMoveProbeRadius > 0.0f ? FootIKCornerMoveProbeRadius : 15.0f;
	const float ProbeRadius = bIsCornerMove ? CornerProbeRadius : PrimaryProbeRadius;
	const float ForwardOffset = FootIKForwardOffset > 0.0f ? FootIKForwardOffset : 40.0f;
	const float BaseRightOffset = (FootIKRightOffset != 0.0f) ? FootIKRightOffset : 7.0f;
	const float VerticalLift = FootIKVerticalLift;
	const float TraceDepth = FootIKTraceDepth > 0.0f ? FootIKTraceDepth : 50.0f;
	const FVector HeightOffset = Up * CharacterHeightDiff;

	TArray<float> RightOffsets;
	RightOffsets.Reserve(5);
	for (int32 Index = 0; Index < 5; ++Index)
	{
		RightOffsets.Add(BaseRightOffset + 4.0f * static_cast<float>(Index));
	}

	auto TraceFoot = [&](const FName& SocketName, int32 Side, FHitResult& OutHit) -> bool
	{
		const FVector SocketLocation = Mesh->DoesSocketExist(SocketName)
			? Mesh->GetSocketLocation(SocketName)
			: Character->GetActorLocation();

		for (float RightOffset : RightOffsets)
		{
			const FVector Start = SocketLocation
				+ HeightOffset
				+ Forward * ForwardOffset
				+ Right * RightOffset * static_cast<float>(Side)
				+ Up * VerticalLift;
			const FVector End = Start - Up * TraceDepth;

			if (!TryConsumeTraceSlot())
			{
				return false;
			}

			FHitResult Hit;
			if (!RunSphereTraceWithOmniTrace(WorldContext, Start, End, ProbeRadius, FootIKTraceChannel, Character, Hit))
			{
				continue;
			}

			OutHit = Hit;


			const FColor Color = bIsCornerMove ? FColor::Cyan : FColor::Green;
			DrawDebugLineIfEnabled(Start, End, bDrawIK, Color);
			DrawDebugSphereIfEnabled(Hit.ImpactPoint, ProbeRadius, bDrawIK, Color);

			return true;
		}

		return false;
	};

	InOutResult.bHasLeftFootSurface = TraceFoot(TEXT("foot_l"), -1, InOutResult.LeftFootHit);
	if (!InOutResult.bHasLeftFootSurface)
	{
		InOutResult.LeftFootHit = FHitResult();
	}

	InOutResult.bHasRightFootSurface = TraceFoot(TEXT("foot_r"), 1, InOutResult.RightFootHit);
	if (!InOutResult.bHasRightFootSurface)
	{
		InOutResult.RightFootHit = FHitResult();
	}

	// Simple parity guard: after consecutive misses, skip next foot IK pass to mimic BP "no hit for N frames" behavior.
	ConsecutiveLeftFootIKFailures = InOutResult.bHasLeftFootSurface ? 0 : ConsecutiveLeftFootIKFailures + 1;
	ConsecutiveRightFootIKFailures = InOutResult.bHasRightFootSurface ? 0 : ConsecutiveRightFootIKFailures + 1;
	const int32 MaxFootFailures = 2; // CGF disables after a couple missed frames
	if (ConsecutiveLeftFootIKFailures >= MaxFootFailures && ConsecutiveRightFootIKFailures >= MaxFootFailures)
	{
		bSkipFootIKTraceOnNextCall = true;
	}
}

bool USOTS_ParkourComponent::ClassifyAdvancedParkour_Implementation(const FSOTS_ParkourResult& BasicResult, FSOTS_ParkourResult& OutResult) const
{
	// Default: no advanced variant chosen. Override in Blueprint/C++ to apply tag-driven or condition-based selection.
	(void)BasicResult;
	(void)OutResult;
	return false;
}

float USOTS_ParkourComponent::GetVerticalDetectStartHeight() const
{
	const ACharacter* Character = CachedOwnerCharacter.Get();
	if (!Character || !CachedMovementComponent)
	{
		return 0.0f;
	}

	return USOTS_ParkourVerticalTraceLibrary::GetVerticalWallDetectStartHeight(
		Character,
		CachedMovementComponent,
		ParkourState);
}

bool USOTS_ParkourComponent::CacheActionDefinitions() const
{
	// Avoid reloading if we already have a cached set.
	if (bHasCachedActionDefinitions)
	{
		return CachedActionDefinitions.Num() > 0;
	}

	// Always prefer the DataTable when assigned, otherwise fall back to the DataAsset.
	CachedActionDefinitions.Reset();
	bHasCachedActionDefinitions = false;

	if (UDataTable* DataTable = ParkourActionDefinitionsTable.LoadSynchronous())
	{
		if (DataTable->GetRowStruct() && DataTable->GetRowStruct()->IsChildOf(FSOTS_ParkourActionDefinition::StaticStruct()))
		{
			TArray<FSOTS_ParkourActionDefinition*> Rows;
			DataTable->GetAllRows<FSOTS_ParkourActionDefinition>(TEXT("SOTS_ParkourComponent"), Rows);
			for (const FSOTS_ParkourActionDefinition* Row : Rows)
			{
				if (Row)
				{
					CachedActionDefinitions.Add(*Row);
				}
			}

			bHasCachedActionDefinitions = CachedActionDefinitions.Num() > 0;
		}
		else
		{
			UE_LOG(LogSOTS_Parkour, Warning, TEXT("USOTS_ParkourComponent: ParkourActionDefinitionsTable row struct %s is not FSOTS_ParkourActionDefinition; cannot load actions."), *GetNameSafe(DataTable->GetRowStruct()));
		}
	}

	if (!bHasCachedActionDefinitions && ParkourActionSet)
	{
		CachedActionDefinitions = ParkourActionSet->Actions;
		bHasCachedActionDefinitions = CachedActionDefinitions.Num() > 0;
	}

	return bHasCachedActionDefinitions;
}

const FSOTS_ParkourActionDefinition* USOTS_ParkourComponent::ResolveActionDefinition(ESOTS_ParkourAction Action) const
{
	if (!CacheActionDefinitions())
	{
		return nullptr;
	}

	const FGameplayTag ActionTag = GetActionTag(Action);
	const FSOTS_ParkourActionDefinition* FoundAction = CachedActionDefinitions.FindByPredicate([Action, ActionTag](const FSOTS_ParkourActionDefinition& Def)
	{
		return Def.ActionType == Action || (ActionTag.IsValid() && Def.ActionTag == ActionTag);
	});

	return FoundAction;
}

FTransform USOTS_ParkourComponent::BuildWarpTransform(
	const FVector& BaseLocation,
	const FVector& SurfaceNormal,
	const FVector& Velocity,
	float XOffset,
	float ZOffset,
	ESOTS_ParkourWarpFacingMode FacingMode,
	bool bReverseRotation,
	bool bAdjustForCharacterHeight,
	float AuthoringHalfHeight) const
{
	FVector DesiredForward = FVector::ForwardVector;

	if (FacingMode == ESOTS_ParkourWarpFacingMode::FaceVelocity && !Velocity.IsNearlyZero())
	{
		DesiredForward = Velocity.GetSafeNormal2D();
	}
	else if (FacingMode == ESOTS_ParkourWarpFacingMode::FaceForward)
	{
		DesiredForward = CachedOwnerCharacter.IsValid() ? CachedOwnerCharacter->GetActorForwardVector() : FVector::ForwardVector;
	}
	else
	{
		FVector Facing = SurfaceNormal.IsNearlyZero() ? FVector::ForwardVector : SurfaceNormal;
		if (!Facing.Normalize())
		{
			Facing = FVector::ForwardVector;
		}
		DesiredForward = -Facing; // face the wall
	}

	if (bReverseRotation)
	{
		DesiredForward *= -1.0f;
	}

	float AdjustedZOffset = ZOffset;
	AdjustedZOffset += CharacterHeightAdjustment;
	if (bAdjustForCharacterHeight)
	{
		const ACharacter* Character = CachedOwnerCharacter.Get();
		if (Character)
		{
			const float AuthoringHeight = FMath::Max(AuthoringHalfHeight, 1.0f);
			AdjustedZOffset += (Character->GetSimpleCollisionHalfHeight() - AuthoringHeight);
		}
	}

	FVector TargetLocation = BaseLocation;
	TargetLocation += -DesiredForward * XOffset;
	TargetLocation.Z += AdjustedZOffset;

	const FRotator TargetRotation = DesiredForward.Rotation();
	return FTransform(TargetRotation, TargetLocation, FVector::OneVector);
}

bool USOTS_ParkourComponent::EnsureStatsWidget() const
{
	if (StatsWidgetInstance.IsValid())
	{
		return true;
	}

	if (ParkourStatsWidgetClass.IsNull())
	{
		return false;
	}

	const ACharacter* CharacterOwner = CachedOwnerCharacter.Get();
	if (!CharacterOwner || !CharacterOwner->IsLocallyControlled())
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(CharacterOwner->GetController());
	if (!PC)
	{
		return false;
	}

	UClass* WidgetClass = ParkourStatsWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		return false;
	}

	if (!WidgetClass->ImplementsInterface(USOTS_ParkourStatsWidgetInterface::StaticClass()))
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("USOTS_ParkourComponent: ParkourStatsWidgetClass %s does not implement SOTS_ParkourStatsWidgetInterface"), *GetNameSafe(WidgetClass));
		return false;
	}

	if (UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass))
	{
		Widget->AddToViewport();
		StatsWidgetInstance = Widget;
		return true;
	}

	return false;
}

void USOTS_ParkourComponent::PushStatsToWidget() const
{
	UUserWidget* Widget = StatsWidgetInstance.Get();
	if (!Widget)
	{
		if (!EnsureStatsWidget())
		{
			return;
		}
		Widget = StatsWidgetInstance.Get();
	}

	if (!Widget || !Widget->GetClass()->ImplementsInterface(USOTS_ParkourStatsWidgetInterface::StaticClass()))
	{
		return;
	}

	const FGameplayTag StateTag = GetStateTag(ParkourLogicalState);
	const FGameplayTag ActionTag = GetActionTag(LastResult.Action);
	const FGameplayTag ClimbStyleTag = GetClimbStyleTag(LastResult.ClimbStyle);

	ISOTS_ParkourStatsWidgetInterface::Execute_SetParkourStateTag(Widget, StateTag);
	ISOTS_ParkourStatsWidgetInterface::Execute_SetParkourActionTag(Widget, ActionTag);
	ISOTS_ParkourStatsWidgetInterface::Execute_SetParkourClimbStyleTag(Widget, ClimbStyleTag);
}

void USOTS_ParkourComponent::SetParkourStatsWidget(UUserWidget* Widget)
{
	if (!Widget)
	{
		StatsWidgetInstance.Reset();
		return;
	}

	if (!Widget->GetClass()->ImplementsInterface(USOTS_ParkourStatsWidgetInterface::StaticClass()))
	{
		UE_LOG(LogSOTS_Parkour, Warning, TEXT("USOTS_ParkourComponent: Provided stats widget %s does not implement SOTS_ParkourStatsWidgetInterface"), *GetNameSafe(Widget));
		return;
	}

	StatsWidgetInstance = Widget;
	PushStatsToWidget();
}

void USOTS_ParkourComponent::ApplyWarpSettingsFromAction(ESOTS_ParkourAction Action, FSOTS_ParkourResult& InOutResult)
{
	const FSOTS_ParkourActionDefinition* FoundAction = ResolveActionDefinition(Action);

	if (!FoundAction || FoundAction->WarpSettings.Num() == 0)
	{
		return;
	}

	LastAppliedActionDefinition = *FoundAction;
	bHasLastAppliedActionDefinition = true;

	// Build runtime warp targets for all windows; keep earliest window metadata for convenience.
	InOutResult.WarpTargets.Reset();

	const FVector BaseLocation = InOutResult.TargetLocation;
	const FVector BaseNormal = InOutResult.SurfaceNormal.IsNearlyZero()
		? FVector::ForwardVector
		: InOutResult.SurfaceNormal;
	const FVector Velocity2D = GetResolvedInputDirection();

	const FSOTS_ParkourWarpSettings* Earliest = nullptr;

	for (const FSOTS_ParkourWarpSettings& Entry : FoundAction->WarpSettings)
	{
		FSOTS_ParkourWarpRuntimeTarget RuntimeTarget;
		RuntimeTarget.TargetName = Entry.WarpTargetName.IsNone() ? FName(TEXT("ParkourTarget")) : Entry.WarpTargetName;
		RuntimeTarget.TargetTransform = BuildWarpTransform(
			BaseLocation,
			BaseNormal,
			Velocity2D,
			Entry.XOffset,
			Entry.ZOffset,
			Entry.FacingMode,
			Entry.bReversedRotation,
			Entry.bAdjustForCharacterHeight,
			MotionWarpAuthoringHalfHeight);
		RuntimeTarget.StartTime = Entry.StartTime;
		RuntimeTarget.EndTime = Entry.EndTime;
		RuntimeTarget.WarpPointAnimProvider = Entry.WarpPointAnimProvider;
		RuntimeTarget.WarpPointAnimBoneName = Entry.WarpPointAnimBoneName;
		RuntimeTarget.bWarpTranslation = Entry.bWarpTranslation;
		RuntimeTarget.bIgnoreZAxis = Entry.bIgnoreZAxis;
		RuntimeTarget.bWarpRotation = Entry.bWarpRotation;
		RuntimeTarget.RotationType = Entry.RotationType;
		RuntimeTarget.bReversedRotation = Entry.bReversedRotation;
		RuntimeTarget.bAdjustForCharacterHeight = Entry.bAdjustForCharacterHeight;
		RuntimeTarget.WarpTransformType = Entry.WarpTransformType;
		InOutResult.WarpTargets.Add(RuntimeTarget);

		if (!Earliest || Entry.StartTime < Earliest->StartTime)
		{
			Earliest = &Entry;
		}
	}

	if (Earliest)
	{
		InOutResult.WarpTargetName = Earliest->WarpTargetName;
		InOutResult.WarpStartTime = Earliest->StartTime;
		InOutResult.WarpEndTime = Earliest->EndTime;
	}

	if (bEnableDebugLogging)
	{
		UE_LOG(LogSOTS_Parkour, Verbose, TEXT("USOTS_ParkourComponent: Warp settings applied Action=%d Target=%s Time=[%.2f, %.2f]"),
			static_cast<int32>(Action), *InOutResult.WarpTargetName.ToString(), InOutResult.WarpStartTime, InOutResult.WarpEndTime);
	}
}

void USOTS_ParkourComponent::LeftClimbIK(bool bEnable)
{
	OnToggleLeftClimbIK.Broadcast(TEXT("Left"), bEnable);
}

void USOTS_ParkourComponent::RightClimbIK(bool bEnable)
{
	OnToggleRightClimbIK.Broadcast(TEXT("Right"), bEnable);
}

void USOTS_ParkourComponent::HandleOutParkourNotify()
{
	// Reset logical and ABP state/action when exiting parkour via notify.
	LastResult.Reset();
	UpdateGameplayTagsFromResult(LastResult);
	SetParkourLogicalState(ESOTS_ParkourLogicalState::NotBusy);
	SetParkourState(ESOTS_ParkourState::Idle);
	NotifyParkourResultUpdated(LastResult);
}
