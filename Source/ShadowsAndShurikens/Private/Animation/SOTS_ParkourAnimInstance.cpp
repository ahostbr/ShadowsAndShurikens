#include "Animation/SOTS_ParkourAnimInstance.h"

#include "SOTS_ParkourComponent.h"
#include "SOTS_ParkourInterface.h"
#include "Anim/SOTS_ExperimentalStateMachineTypes.h"
#include "Animation/AnimationAsset.h"
#include "Animation/BlendProfile.h"
#include "AnimationWarpingLibrary.h"
#include "BlendStack/AnimNode_BlendStack.h"
#include "BlendStack/BlendStackAnimNodeLibrary.h"
#include "ChooserFunctionLibrary.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/RotationMatrix.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTSParkourSM, Log, All);

namespace
{
UBlendProfile* ResolveBlendProfile(USkeletalMeshComponent* Mesh, const FName& ProfileName)
{
    if (ProfileName.IsNone() || !Mesh)
    {
        return nullptr;
    }

    const USkeletalMesh* SkelMesh = Mesh->GetSkeletalMeshAsset();
    USkeleton* Skeleton = SkelMesh ? const_cast<USkeleton*>(SkelMesh->GetSkeleton()) : nullptr;
    return Skeleton ? Skeleton->GetBlendProfile(ProfileName) : nullptr;
}

bool EvaluateExperimentalChooser(const UChooserTable* Chooser, ESOTS_StateMachineState StateMachineState, const UObject* ContextObject, FS_ChooserOutputs& OutOutputs, TArray<TObjectPtr<UAnimationAsset>>& OutAnims)
{
    OutOutputs.Reset();
    OutAnims.Reset();

    if (!Chooser || !ContextObject)
    {
        UE_LOG(LogSOTSParkourSM, Warning, TEXT("EvaluateExperimentalChooser: Chooser or ContextObject null (State=%d)"), static_cast<int32>(StateMachineState));
        return false;
    }

    // Mirror the BP "Evaluate Chooser" node: object context + struct for outputs.
    FChooserEvaluationContext Context(const_cast<UObject*>(ContextObject));
    Context.AddStructParam(OutOutputs);

    // Collect candidate animation assets; trigger output columns on first result.
    const FObjectChooserBase::EIteratorStatus Status = UChooserTable::EvaluateChooser(
        Context,
        Chooser,
        FObjectChooserBase::FObjectChooserIteratorCallback::CreateLambda([
            &OutAnims
        ](UObject* InResult)
        {
            if (UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(InResult))
            {
                OutAnims.Add(AnimAsset);
            }

            // Mirror EvaluateChooserMulti behavior: fire outputs on first result, keep iterating otherwise.
            return OutAnims.Num() == 1 ? FObjectChooserBase::EIteratorStatus::ContinueWithOutputs
                                        : FObjectChooserBase::EIteratorStatus::Continue;
        }));

    const bool bHasAny = OutAnims.Num() > 0;
    const bool bStoppedOrOutput = Status != FObjectChooserBase::EIteratorStatus::Continue;
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("EvaluateExperimentalChooser: State=%d Chooser=%s Candidates=%d Status=%d bHasAny=%d"), static_cast<int32>(StateMachineState), *Chooser->GetName(), OutAnims.Num(), static_cast<int32>(Status), bHasAny);
    return bHasAny || bStoppedOrOutput;
}

bool HasStopTag(const FGameplayTagContainer& Tags)
{
    const FGameplayTag StopsTag = FGameplayTag::RequestGameplayTag(TEXT("Stops"), false);
    if (StopsTag.IsValid() && Tags.HasTagExact(StopsTag))
    {
        return true;
    }

    static const FName StopName(TEXT("Stop"));
    static const FName StopsName(TEXT("Stops"));

    for (const FGameplayTag& Tag : Tags)
    {
        const FName TagName = Tag.GetTagName();
        if (TagName.IsEqual(StopName, ENameCase::IgnoreCase) || TagName.IsEqual(StopsName, ENameCase::IgnoreCase))
        {
            return true;
        }
    }

    return false;
}

bool HasStopTag(const TArray<FName>& Names)
{
    static const FName StopName(TEXT("Stop"));
    static const FName StopsName(TEXT("Stops"));

    for (const FName& Name : Names)
    {
        if (Name.IsEqual(StopName, ENameCase::IgnoreCase) || Name.IsEqual(StopsName, ENameCase::IgnoreCase))
        {
            return true;
        }
    }

    return false;
}
}

bool USOTS_ParkourAnimInstance::TryRunStateMachineMotionMatch(const TArray<TObjectPtr<UAnimationAsset>>& CandidateAnims, const FS_ChooserOutputs& ChooserOutputs, FPoseSearchBlueprintResult& OutResult, float& OutSearchCost)
{
    OutResult = FPoseSearchBlueprintResult();
    OutSearchCost = 0.0f;

    TArray<UObject*> AssetsToSearch;
    AssetsToSearch.Reserve(CandidateAnims.Num());
    for (UAnimationAsset* Anim : CandidateAnims)
    {
        if (Anim)
        {
            AssetsToSearch.Add(Anim);
        }
    }

    if (AssetsToSearch.Num() == 0)
    {
        UE_LOG(LogSOTSParkourSM, Warning, TEXT("TryRunStateMachineMotionMatch: No assets to search"));
        return false;
    }

    const FPoseSearchContinuingProperties ContinuingProperties;
    const FPoseSearchFutureProperties FutureProperties;
    UPoseSearchLibrary::MotionMatch(this, AssetsToSearch, TEXT("PoseHistory"), ContinuingProperties, FutureProperties, OutResult);

    OutSearchCost = OutResult.SearchCost;

    const float EffectiveCostLimit = (ChooserOutputs.MMCostLimit > 0.0f) ? ChooserOutputs.MMCostLimit : MMCostLimit_Default;
    if (EffectiveCostLimit > 0.0f && OutSearchCost > EffectiveCostLimit)
    {
        UE_LOG(LogSOTSParkourSM, Warning, TEXT("TryRunStateMachineMotionMatch: Rejected anim=%s Cost=%.2f Limit=%.2f"), OutResult.SelectedAnim ? *OutResult.SelectedAnim->GetName() : TEXT("None"), OutSearchCost, EffectiveCostLimit);
        return false;
    }

    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("TryRunStateMachineMotionMatch: Selected anim=%s Time=%.3f Cost=%.2f Limit=%.2f"), OutResult.SelectedAnim ? *OutResult.SelectedAnim->GetName() : TEXT("None"), OutResult.SelectedTime, OutSearchCost, EffectiveCostLimit);
    return OutResult.SelectedAnim != nullptr;
}

bool USOTS_ParkourAnimInstance::IsAnimationAlmostComplete() const
{
    return IsLogicalStateAlmostComplete;
}

bool USOTS_ParkourAnimInstance::IsAnimationAlmostComplete_WithNode(const FBlendStackAnimNodeReference& BlendStackNode, const float ThresholdSeconds) const
{
    // Match the legacy BP: only consider non-looping assets and trigger when time remaining is below the threshold.
    const bool bIsLooping = UBlendStackAnimNodeLibrary::IsCurrentAssetLooping(BlendStackNode);
    if (bIsLooping)
    {
        return false;
    }

    const float TimeRemaining = UBlendStackAnimNodeLibrary::GetCurrentAssetTimeRemaining(BlendStackNode);
    return TimeRemaining <= ThresholdSeconds;
}

USOTS_ParkourAnimInstance::USOTS_ParkourAnimInstance()
{
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("USOTS_ParkourAnimInstance ctor"));
}

void USOTS_ParkourAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    CacheOwnerReferences();
    const ACharacter* InitOwner = SandboxCharacter.Get();
    UE_LOG(LogSOTSParkourSM, Log, TEXT("NativeInitializeAnimation: Owner=%s ExperimentalSM=%d"), InitOwner ? *InitOwner->GetName() : TEXT("None"), UseExperimentalStateMachine);
}

void USOTS_ParkourAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    const bool bAlreadyUpdated = UseThreadSafeUpdateAnimation && LastThreadSafeUpdateFrame == GFrameCounter;

    // Only run the full update if thread-safe did not already execute this frame.
    if (!bAlreadyUpdated)
    {
        UpdateAnimationTick(DeltaSeconds, /*bThreadSafe*/ false);
    }
}

void USOTS_ParkourAnimInstance::NativeThreadSafeUpdateAnimation(const float DeltaSeconds)
{
    // Mirror BP guard: only run when both the sandbox character is valid and thread-safe update is enabled.
    if (!UseThreadSafeUpdateAnimation)
    {
        return;
    }

    IsSandboxCharacterValid = SandboxCharacter != nullptr;
    if (!IsSandboxCharacterValid)
    {
        return;
    }

    // Run the shared update path directly to avoid double-ticking (hover was caused by invoking NativeUpdateAnimation from here).
    UE_LOG(LogSOTSParkourSM, VeryVerbose, TEXT("NativeThreadSafeUpdateAnimation: Delta=%.4f"), DeltaSeconds);
    UpdateAnimationTick(DeltaSeconds, /*bThreadSafe*/ true);
    LastThreadSafeUpdateFrame = GFrameCounter;
}

void USOTS_ParkourAnimInstance::UpdateAnimationTick(const float DeltaSeconds, const bool bThreadSafe)
{
    UE_LOG(LogSOTSParkourSM, VeryVerbose, TEXT("UpdateAnimationTick: Delta=%.4f ThreadSafe=%d ExperimentalSM=%d"), DeltaSeconds, bThreadSafe, UseExperimentalStateMachine);

    // Re-cache if pawn changed or not yet cached.
    const ACharacter* OwnerCharacter = SandboxCharacter.Get();

    if (!OwnerCharacter || OwnerCharacter != TryGetPawnOwner())
    {
        CacheOwnerReferences();
        OwnerCharacter = SandboxCharacter.Get();
    }

    if (!OwnerCharacter)
    {
        return;
    }

    UpdateCVarDrivenVariables();
    UpdateMovementAndCoverData(DeltaSeconds);
    // Order mirrors legacy ABP: movement → essential values → states → MM → parkour.
    UpdateEssentialValues(DeltaSeconds);
    UpdateEssentialStates(DeltaSeconds);
    UpdateTargetRotation();
    UpdateMotionMatchingData(DeltaSeconds);

    // Lightweight landing snap runs only on the game thread to avoid hover between MM transitions.
    if (!bThreadSafe)
    {
        SnapToGroundOnLand();
    }

    UpdateExperimentalStateMachine(DeltaSeconds);
    UpdateLegacyBpGates(DeltaSeconds);
    UpdateParkourData(DeltaSeconds);
}

void USOTS_ParkourAnimInstance::CacheOwnerReferences()
{
    ACharacter* Owner = SandboxCharacter.Get();
    ACharacter* PawnOwner = Cast<ACharacter>(TryGetPawnOwner());

    if (Owner != PawnOwner)
    {
        Owner = PawnOwner;
        SandboxCharacter = Owner;
    }

    if (!Owner)
    {
        CharacterMovement = nullptr;
        ParkourComponent = nullptr;
        IsSandboxCharacterValid = false;
        if (!bLoggedMissingOwner)
        {
            UE_LOG(LogSOTSParkourSM, Warning, TEXT("CacheOwnerReferences: No owner"));
            bLoggedMissingOwner = true;
        }
        return;
    }

    bLoggedMissingOwner = false;

    CharacterMovement = Owner->FindComponentByClass<UCharacterMovementComponent>();
    ParkourComponent = Owner->FindComponentByClass<USOTS_ParkourComponent>();
    IsSandboxCharacterValid = true;
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("CacheOwnerReferences: Owner=%s Movement=%s ParkourComp=%s"), *Owner->GetName(), CharacterMovement ? *CharacterMovement->GetName() : TEXT("None"), ParkourComponent ? *ParkourComponent->GetName() : TEXT("None"));
}

void USOTS_ParkourAnimInstance::UpdateCVarDrivenVariables()
{
    // Mirrors BP Update_CVarDrivenVariables: pull runtime CVars and component tags.
    auto GetBoolCVar = [](const TCHAR* Name, bool& OutValue)
    {
        if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            OutValue = CVar->GetBool();
        }
    };

    auto GetIntCVar = [](const TCHAR* Name, int32& OutValue)
    {
        if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            OutValue = CVar->GetInt();
        }
    };

    auto GetFloatCVar = [](const TCHAR* Name, float& OutValue)
    {
        if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            OutValue = CVar->GetFloat();
        }
    };

    // Offset root toggles.
    GetBoolCVar(TEXT("a.animnode.offsetrootbone.enable"), OffsetRootBoneEnabled);
    GetFloatCVar(TEXT("DDCvar.OffsetRootBone.TranslationRadius"), OffsetRootTranslationRadius);

    // Motion matching and threading flags.
    GetIntCVar(TEXT("DDCvar.MMDatabaseLOD"), MMDatabaseLOD);
    // Thread-safe update: hard default off. Only enable when explicitly tagged on the mesh; ignore CVar unless a tag opts in.
    bool bThreadSafeUpdate = false;
    if (USkeletalMeshComponent* Mesh = GetSkelMeshComponent())
    {
        const bool bForceThreadSafeTag = Mesh->ComponentHasTag(TEXT("Force ThreadSafe Animation Update"));
        const bool bForceNoThreadSafeTag = Mesh->ComponentHasTag(TEXT("Force No ThreadSafe Animation Update"));
        if (bForceThreadSafeTag)
        {
            // Optional CVar gate if designers want it; default path ignores CVar.
            bool bCVarEnable = false;
            GetBoolCVar(TEXT("DDCVar.ThreadSafeAnimationUpdate.Enable"), bCVarEnable);
            bThreadSafeUpdate = bCVarEnable || true; // tag explicitly allows
        }
        if (bForceNoThreadSafeTag)
        {
            bThreadSafeUpdate = false;
        }
    }
    UseThreadSafeUpdateAnimation = bThreadSafeUpdate;

    // Experimental state machine toggles (with component-tag overrides).
    // Default to enabled so the experimental state machine runs unless a CVar/tag explicitly disables it.
    bool bEnableExperimental = true;
    bool bDebugExperimental = false;
    GetBoolCVar(TEXT("DDCVar.ExperimentalStateMachine.Enable"), bEnableExperimental);
    GetBoolCVar(TEXT("DDCVar.ExperimentalStateMachine.Debug"), bDebugExperimental);

    bool bForceSMSetup = false;
    bool bForceMMSetup = false;
    if (USkeletalMeshComponent* Mesh = GetSkelMeshComponent())
    {
        bForceSMSetup = Mesh->ComponentHasTag(TEXT("Force SM Setup"));
        bForceMMSetup = Mesh->ComponentHasTag(TEXT("Force MM Setup"));
    }

    const bool bEnableOrTag = bEnableExperimental || bForceSMSetup;
    const bool bNotForceMM = !bForceMMSetup;
    UseExperimentalStateMachine = bEnableOrTag && bNotForceMM;
    DebugExperimentalStateMachine = bDebugExperimental;
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("UpdateCVarDrivenVariables: UseExperimental=%d DebugExperimental=%d ForceSMTag=%d ForceMMTag=%d"), UseExperimentalStateMachine, DebugExperimentalStateMachine, bForceSMSetup, bForceMMSetup);
}

void USOTS_ParkourAnimInstance::UpdateMovementAndCoverData(const float /*DeltaSeconds*/)
{
    const ACharacter* OwnerCharacter = SandboxCharacter.Get();

    const FVector VelocityWS = CharacterMovement ? CharacterMovement->Velocity : FVector::ZeroVector;
    const FTransform ActorTM = OwnerCharacter ? OwnerCharacter->GetActorTransform() : FTransform::Identity;

    CharacterTransform = ActorTM;
    // Console-driven offset root toggle to mirror AnimBP.
    bool bOffsetRootEnabledCVar = true;
    if (const IConsoleVariable* OffsetRootCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("a.animnode.offsetrootbone.enable")))
    {
        bOffsetRootEnabledCVar = OffsetRootCVar->GetBool();
    }
    OffsetRootBoneEnabled = bOffsetRootEnabledCVar;

    // If offset root is enabled, try to use the OffsetRoot socket transform and apply the AnimBP yaw offset (+90 degrees).
    if (OffsetRootBoneEnabled)
    {
        if (USkeletalMeshComponent* Mesh = GetSkelMeshComponent())
        {
            static const FName OffsetRootName(TEXT("OffsetRoot"));
            if (Mesh->DoesSocketExist(OffsetRootName))
            {
                FTransform OffsetRootTransform = Mesh->GetSocketTransform(OffsetRootName, RTS_World);
                FRotator OffsetRootRot = OffsetRootTransform.GetRotation().Rotator();
                OffsetRootRot.Yaw += 90.0f; // matches BP Make Transform yaw adjust
                OffsetRootTransform.SetRotation(OffsetRootRot.Quaternion());
                RootTransform = OffsetRootTransform;
            }
            else
            {
                RootTransform = ActorTM;
            }
        }
        else
        {
            RootTransform = ActorTM;
        }
    }
    else
    {
        RootTransform = ActorTM;
    }

    const FVector VelocityLS = ActorTM.InverseTransformVectorNoScale(VelocityWS);
    CurrentMovementX = VelocityLS.X;
    CurrentMovementY = VelocityLS.Y;
    PlayersForwardVector = ActorTM.GetUnitAxis(EAxis::X);

    IsPlayerCrouched = OwnerCharacter ? OwnerCharacter->bIsCrouched : false;

    // TODO: Integrate real cover system values once the cover component is wired in C++.
    CoverState = FGameplayTag::EmptyTag;
    CoverMovingDirection = 0.0f;
    IsInCover_ABP = false;

    // TODO: Drive this from curves/tags once available; placeholder to keep ABP stable.
    RightArmBlendWeight = 1.0f;
}

void USOTS_ParkourAnimInstance::UpdateParkourData(const float /*DeltaSeconds*/)
{
    if (!ParkourComponent)
    {
        ApplyResultToAnimData(FSOTS_ParkourResult());
        ParkourState = FGameplayTag::EmptyTag;
        ParkourLogicalState = FGameplayTag::EmptyTag;
        ParkourAction = FGameplayTag::EmptyTag;
        ClimbStyle = FGameplayTag::EmptyTag;
        ClimbDirection = FGameplayTag::EmptyTag;
        return;
    }

    const FSOTS_ParkourResult Result = ParkourComponent->GetLastResult();
    ApplyResultToAnimData(Result);
}

void USOTS_ParkourAnimInstance::ApplyResultToAnimData(const FSOTS_ParkourResult& Result)
{
    const ACharacter* OwnerCharacter = SandboxCharacter.Get();

    // Tags sourced from component helpers to stay consistent with tag spine.
    if (ParkourComponent)
    {
        ParkourState = ParkourComponent->GetParkourStateTag();
        ParkourLogicalState = ParkourComponent->GetParkourStateTag();
        ParkourAction = ParkourComponent->GetParkourActionTag();
        ClimbStyle = ParkourComponent->GetClimbStyleTag();
        ClimbDirection = ParkourComponent->GetDirectionTag();
    }
    else
    {
        ParkourState = FGameplayTag::EmptyTag;
        ParkourLogicalState = FGameplayTag::EmptyTag;
        ParkourAction = FGameplayTag::EmptyTag;
        ClimbStyle = FGameplayTag::EmptyTag;
        ClimbDirection = Result.DirectionTag;
    }

    // Hand IK targets from hits (AnimBP now owns parity application; C++ provides base in FSOTS_ParkourResult).
    LeftHandLedgeLocation = Result.LeftHandBaseLocation;
    RightHandLedgeLocation = Result.RightHandBaseLocation;
    LeftHandLedgeRotation = Result.LeftHandBaseRotation;
    RightHandLedgeRotation = Result.RightHandBaseRotation;

    // Foot targets: consume traced hits when available; otherwise fall back to sockets/actor.
    if (USkeletalMeshComponent* Mesh = GetSkelMeshComponent())
    {
        const FVector LeftSocket = Mesh->DoesSocketExist(TEXT("foot_l")) ? Mesh->GetSocketLocation(TEXT("foot_l")) : Mesh->GetComponentLocation();
        const FVector RightSocket = Mesh->DoesSocketExist(TEXT("foot_r")) ? Mesh->GetSocketLocation(TEXT("foot_r")) : Mesh->GetComponentLocation();

        LeftFootLocation = Result.bHasLeftFootSurface && Result.LeftFootHit.IsValidBlockingHit()
            ? Result.LeftFootHit.ImpactPoint
            : LeftSocket;
        RightFootLocation = Result.bHasRightFootSurface && Result.RightFootHit.IsValidBlockingHit()
            ? Result.RightFootHit.ImpactPoint
            : RightSocket;
    }
    else
    {
        const FVector Fallback = OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
        LeftFootLocation = Result.bHasLeftFootSurface && Result.LeftFootHit.IsValidBlockingHit() ? Result.LeftFootHit.ImpactPoint : Fallback;
        RightFootLocation = Result.bHasRightFootSurface && Result.RightFootHit.IsValidBlockingHit() ? Result.RightFootHit.ImpactPoint : Fallback;
    }

    IsHidingOnBeam = false; // TODO: expose from parkour result/component once available.

    const bool bHasHands = Result.bHasLeftHandSurface || Result.bHasRightHandSurface;
    ParkourIK_Enabled = (Result.bHasResult || Result.bIsValid) ? bHasHands : false;
    bLeftClimbIK = Result.bEnableLeftClimbIK;
    bRightClimbIK = Result.bEnableRightClimbIK;

    const bool bIsFalling = CharacterMovement ? CharacterMovement->IsFalling() : false;
    const FGameplayTag ClimbStateTag = FGameplayTag::RequestGameplayTag(TEXT("Parkour.State.Climb"), false);
    const bool bIsClimbState = ParkourState.IsValid() && ParkourState.MatchesTagExact(ClimbStateTag);
    LegIK = !bIsFalling && !bIsClimbState;
}

void USOTS_ParkourAnimInstance::UpdateEssentialValues(const float DeltaSeconds)
{
    const bool bPrevHasVelocity = HasVelocity;

    // Mirrors ABP UpdateEssentialValues: track velocity/acceleration magnitudes for locomotion/MM.
    const FVector CurrentVelocity = CharacterMovement ? CharacterMovement->Velocity : FVector::ZeroVector;
    Velocity = CurrentVelocity;
    const FVector PrevVelocity = Velocity_LastFrame;
    Velocity_LastFrame = CurrentVelocity;
    LandVelocity = CurrentVelocity;

    Speed2D = CurrentVelocity.Size2D();
    HasVelocity = Speed2D > 5.0f; // match Parkour AnimBP threshold
    if (HasVelocity)
    {
        LastNonZeroVelocity = CurrentVelocity;
    }

    HasVelocity_LastFrame = bPrevHasVelocity;

    Acceleration = CharacterMovement ? CharacterMovement->GetCurrentAcceleration() : FVector::ZeroVector;
    const float MaxAccel = CharacterMovement ? FMath::Max(CharacterMovement->GetMaxAcceleration(), 0.001f) : 0.001f;
    AccelerationAmount = Acceleration.Size() / MaxAccel; // normalized like BP
    HasAcceleration = AccelerationAmount > 0.0f;

    const float SafeDeltaSeconds = FMath::Max(DeltaSeconds, 0.001f);
    VelocityAcceleration = (CurrentVelocity - PrevVelocity) / SafeDeltaSeconds;

    // PrevVelocity currently unused but kept for parity/debug if future delta calculations are needed.
    (void)PrevVelocity;
}

void USOTS_ParkourAnimInstance::UpdateEssentialStates(const float DeltaSeconds)
{
    // Mirrors ABP UpdateEssentialStates bookkeeping (movement mode and locomotion tags).
    MovementMode_LastFrame = MovementMode;
    MovementMode_SOTS_LastFrame = MovementMode_SOTS;
    if (CharacterMovement)
    {
        MovementMode = TEnumAsByte<EMovementMode>(CharacterMovement->MovementMode);

        // If movement mode is stuck in Flying, detect ground contact and force a Falling handoff so land logic triggers.
        if (MovementMode == MOVE_Flying)
        {
            bool bGrounded = CharacterMovement->IsMovingOnGround();

            if (!bGrounded)
            {
                if (const ACharacter* Character = SandboxCharacter.Get())
                {
                    if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
                    {
                        const float CapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
                        const float CapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
                        const FVector ActorLocation = Character->GetActorLocation();
                        const FVector Up = FVector::UpVector;

                        const FVector TraceStart = ActorLocation + Up * 4.0f;
                        const FVector TraceEnd = TraceStart - Up * (CapsuleHalfHeight + 30.0f);

                        FCollisionQueryParams Params(TEXT("SOTS_FlyingToFallingProbe"), false);
                        Params.AddIgnoredActor(Character);

                        FHitResult Hit;
                        if (UWorld* World = GetWorld())
                        {
                            const bool bHit = World->SweepSingleByChannel(
                                Hit,
                                TraceStart,
                                TraceEnd,
                                FQuat::Identity,
                                ECC_Visibility,
                                FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
                                Params);

                            bGrounded = bHit && Hit.bBlockingHit;
                        }
                    }
                }
            }

            if (bGrounded)
            {
                MovementMode = MOVE_Falling;
            }
        }
    }
    else
    {
        MovementMode = MOVE_None;
    }

    MovementState_LastFrame = MovementState;
    Gait_LastFrame = Gait;
    Stance_LastFrame = Stance;
    RotationMode_LastFrame = RotationMode;

    // Gait mirrors BP enum switch: derive from desired gait input if exposed; fall back to speed buckets.
    E_Gait DesiredGait = Gait;
    const float DesiredSpeed = CharacterMovement ? CharacterMovement->GetMaxSpeed() : Speed2D;
    if (DesiredSpeed >= 600.0f)
    {
        DesiredGait = E_Gait::Sprinting;
    }
    else if (DesiredSpeed >= 300.0f)
    {
        DesiredGait = E_Gait::Jogging;
    }
    else
    {
        DesiredGait = E_Gait::Walking;
    }
    Gait = DesiredGait;

    // Stance: crouched vs standing.
    const bool bIsCrouching = CharacterMovement ? CharacterMovement->IsCrouching() : false;
    Stance = bIsCrouching ? ESOTS_Stance::Crouch : ESOTS_Stance::Stand;

    // Rotation mode: orient-to-movement vs strafe.
    const bool bOrientToMovement = CharacterMovement ? CharacterMovement->bOrientRotationToMovement : true;
    RotationMode = bOrientToMovement ? ESOTS_RotationMode::OrientToRotation : ESOTS_RotationMode::Strafe;

    // Simple input state proxy for BP consumption (idle vs moving gate).
    const bool bWantsToMove = !FutureVelocity.IsNearlyZero(1.0f) || HasVelocity;
    CharacterInputState = bWantsToMove ? FGameplayTag::EmptyTag : FGameplayTag::EmptyTag;

    // Transition-friendly gates.
    IsGrounded = (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking);
    IsInAir = (MovementMode == MOVE_Falling || MovementMode == MOVE_Flying || MovementMode == MOVE_Swimming);

    MovementMode_SOTS = IsGrounded ? ESOTS_MovementMode::OnGround : ESOTS_MovementMode::InAir;
    MovementState = bWantsToMove ? ESOTS_MovementState::Moving : ESOTS_MovementState::Idle;

    // Allow steering when moving or airborne; prevents idle root-motion steering in BP.
    CanSteerRootMotion = HasVelocity || IsInAir;

    // Track air/ground time for land gating.
    if (IsInAir)
    {
        TimeInAir += DeltaSeconds;
        TimeOnGround = 0.0f;
    }
    else
    {
        TimeOnGround += DeltaSeconds;
        TimeInAir = 0.0f;
    }
}

void USOTS_ParkourAnimInstance::UpdateTargetRotation()
{
    const FRotator CharacterRot = CharacterTransform.GetRotation().Rotator();
    const FRotator RootRot = RootTransform.GetRotation().Rotator();

    FRotator DesiredRotation = CharacterRot;
    const bool bStrafingWithVelocity = HasVelocity && RotationMode == ESOTS_RotationMode::Strafe;
    if (bStrafingWithVelocity)
    {
        DesiredRotation.Yaw += Get_StrafeYawRotationOffset();
    }

    TargetRotation = DesiredRotation;
    TargetRotationDelta = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, RootRot);
}

float USOTS_ParkourAnimInstance::Get_StrafeYawRotationOffset() const
{
    return 0.0f;
}

void USOTS_ParkourAnimInstance::UpdateLegacyBpGates(const float /*DeltaSeconds*/)
{
    // BP-only gates and helper values that were previously computed in the AnimBP event graph.
    const FVector CurrentVel = Velocity;
    const FVector FutureVel = FutureVelocity;
    const float FutureToleranceSq = FMath::Square(10.0f);

    const bool bFutureMoving = !FutureVel.IsNearlyZero(FutureToleranceSq);
    const bool bAccelMoving = !Acceleration.IsNearlyZero();
    IsMoving = bFutureMoving && bAccelMoving;

    const float FutureSpeed2D = FutureVel.Size2D();
    const float CurrentSpeed2D = Speed2D;
    const FGameplayTag PivotTag = FGameplayTag::RequestGameplayTag(TEXT("Pivots"), false);
    auto ContainsTagName = [](const TArray<FName>& Names, const FGameplayTag& Tag)
    {
        return Tag.IsValid() && Names.Contains(Tag.GetTagName());
    };

    const bool bHasPivotTag = ContainsTagName(CurrentDatabaseTags, PivotTag);
    IsStarting = IsMoving && (FutureSpeed2D >= (CurrentSpeed2D + 100.0f)) && !bHasPivotTag;

    // Simple pivot/turn/spin heuristics derived from the BP graphs.
    const FVector CurrentDir = CurrentVel.GetSafeNormal2D();
    const FVector FutureDir = FutureVel.GetSafeNormal2D();
    float FacingDeltaDegrees = 0.0f;
    if (!CurrentDir.IsNearlyZero() && !FutureDir.IsNearlyZero())
    {
        const float Dot = FMath::Clamp(FVector::DotProduct(CurrentDir, FutureDir), -1.0f, 1.0f);
        FacingDeltaDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));
    }

    const float AbsTurnAngle = FacingDeltaDegrees;

    const auto MapRangeClamped = [](const FVector2D& InRange, const FVector2D& OutRange, float Value)
    {
        return FMath::GetMappedRangeValueClamped(InRange, OutRange, Value);
    };

    float StanceGaitAngleThreshold = 0.0f;
    if (Stance == ESOTS_Stance::Crouch)
    {
        StanceGaitAngleThreshold = 65.0f;
    }
    else
    {
        switch (Gait)
        {
            case E_Gait::Walking:
            StanceGaitAngleThreshold = MapRangeClamped(FVector2D(150.0f, 200.0f), FVector2D(70.0f, 60.0f), Speed2D);
            break;
            case E_Gait::Jogging:
            StanceGaitAngleThreshold = MapRangeClamped(FVector2D(300.0f, 500.0f), FVector2D(70.0f, 60.0f), Speed2D);
            break;
            case E_Gait::Sprinting:
            StanceGaitAngleThreshold = MapRangeClamped(FVector2D(300.0f, 700.0f), FVector2D(60.0f, 50.0f), Speed2D);
            break;
        default:
            StanceGaitAngleThreshold = 60.0f;
            break;
        }
    }

    bool bSpeedGate = false;
    if (Stance == ESOTS_Stance::Crouch)
    {
        bSpeedGate = FMath::IsWithinInclusive(Speed2D, 50.0f, 200.0f);
    }
    else
    {
        switch (Gait)
        {
            case E_Gait::Walking:
            bSpeedGate = FMath::IsWithinInclusive(Speed2D, 50.0f, 200.0f);
            break;
            case E_Gait::Jogging:
            bSpeedGate = FMath::IsWithinInclusive(Speed2D, 200.0f, 550.0f);
            break;
            case E_Gait::Sprinting:
            bSpeedGate = FMath::IsWithinInclusive(Speed2D, 200.0f, 700.0f);
            break;
        default:
            bSpeedGate = FMath::IsWithinInclusive(Speed2D, 50.0f, 200.0f);
            break;
        }
    }

    const bool bStanceGaitPivot = IsMoving && bSpeedGate && (AbsTurnAngle >= StanceGaitAngleThreshold);

    float RotationModeAngleThreshold = 45.0f;
    if (RotationMode == ESOTS_RotationMode::Strafe)
    {
        RotationModeAngleThreshold = 30.0f;
    }

    const bool bRotationModePivot = IsMoving && (AbsTurnAngle >= RotationModeAngleThreshold);

    const float YawCurrent = CharacterTransform.GetRotation().Rotator().Yaw;
    const float YawRoot = RootTransform.GetRotation().Rotator().Yaw;
    const float YawDelta = FMath::FindDeltaAngleDegrees(YawCurrent, YawRoot);

    IsPivoting = bRotationModePivot || bStanceGaitPivot;
    const bool bSpinAngle = FMath::Abs(YawDelta) >= 130.0f;
    const bool bSpinSpeed = Speed2D >= 150.0f;
    ShouldSpinTransition = bSpinAngle && bSpinSpeed && !bHasPivotTag;

    const float AbsYawDelta = FMath::Abs(YawDelta);
    const bool bYawGate = AbsYawDelta >= 50.0f;

    // Parity with BP ShouldTurnInPlace: allow when aiming (strafe) or on a stick-flick stop.
    const bool bAimingLike = (RotationMode == ESOTS_RotationMode::Strafe);
    const bool bStoppedThisFrame = !HasVelocity && HasVelocity_LastFrame;
    const bool bTurnGate = bAimingLike || bStoppedThisFrame;

    ShouldTurnInPlace = bYawGate && bTurnGate;

    // Lean amount mirrors AnimBP helper (relative accel scaled by speed).
    LeanAmount = Get_LeanAmount();

    // AO value mirrors the AnimBP helper.
    AOValue = Get_AOValue();

    const bool bWasAir = (MovementMode_LastFrame == MOVE_Falling || MovementMode_LastFrame == MOVE_Flying || MovementMode_LastFrame == MOVE_Swimming);
    const bool bNowGround = (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking);
    const bool bWasFlying = (MovementMode_LastFrame == MOVE_Flying);
    const bool bJustLanded = bWasAir && bNowGround;
    const float LandSpeedZAbs = FMath::Abs(LandVelocity.Z);
    const float HeavyThreshold = FMath::Abs(HeavyLandSpeedThreshold);

    const bool bLandTimeGate = (TimeInAir >= MinAirTimeForLand) || bWasFlying;
    const bool bLandSpeedGate = (LandSpeedZAbs >= MinLandSpeedForLand) || bWasFlying;
    const bool bAllowLand = bJustLanded && bLandTimeGate && bLandSpeedGate;

    JustLandedHeavy = bAllowLand && (LandSpeedZAbs >= HeavyThreshold);
    JustLandedLight = bAllowLand && (LandSpeedZAbs < HeavyThreshold);

    const float TraversalCurve = GetCurveValue(TEXT("MovingTraversal"));
    const bool bCurveActive = TraversalCurve > 0.0f;
    const bool bDefaultSlotInactive = !IsSlotActive(TEXT("DefaultSlot"));
    const bool bLowTurnAngle = FMath::Abs(FacingDeltaDegrees) <= 50.0f;
    JustTraversed = bCurveActive && bDefaultSlotInactive && bLowTurnAngle;

    // Replace BP "IsAnimationAlmostComplete" helper: compute remaining fraction for the configured logical state.
    IsLogicalStateAlmostComplete = false;
    if (!LogicalStateMachineName.IsNone() && !LogicalStateName.IsNone())
    {
        const int32 MachineIndex = GetStateMachineIndex(LogicalStateMachineName);
        if (MachineIndex != INDEX_NONE)
        {
            const FName CurrentState = GetCurrentStateName(MachineIndex);
            if (CurrentState == LogicalStateName)
            {
                const float Remaining = GetRelevantAnimTimeRemainingFraction(MachineIndex, LogicalStateIndex);
                if (Remaining >= 0.0f)
                {
                    IsLogicalStateAlmostComplete = Remaining <= LogicalStateAlmostDoneThreshold;
                }
            }
        }
    }
}

void USOTS_ParkourAnimInstance::SnapToGroundOnLand()
{
    if (!IsSandboxCharacterValid || !CharacterMovement)
    {
        return;
    }

    const bool bNowGround = (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking);
    const bool bWasAir = (MovementMode_LastFrame == MOVE_Falling || MovementMode_LastFrame == MOVE_Flying || MovementMode_LastFrame == MOVE_Swimming);
    if (!(bNowGround && bWasAir))
    {
        return;
    }

    ACharacter* Character = SandboxCharacter.Get();
    if (!Character)
    {
        return;
    }

    const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
    if (!Capsule)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float CapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
    const float CapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
    const FVector ActorLocation = Character->GetActorLocation();
    const FVector Up = FVector::UpVector;

    const FVector TraceStart = ActorLocation + Up * 6.0f;
    const FVector TraceEnd = TraceStart - Up * (CapsuleHalfHeight + 60.0f);

    FCollisionQueryParams Params(TEXT("SOTS_SnapToGround"), false);
    Params.AddIgnoredActor(Character);

    FHitResult Hit;
    const bool bHit = World->SweepSingleByChannel(
        Hit,
        TraceStart,
        TraceEnd,
        FQuat::Identity,
        ECC_Visibility,
        FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
        Params);

    if (!bHit || !Hit.bBlockingHit)
    {
        return;
    }

    FVector NewLocation = Hit.ImpactPoint;
    NewLocation.Z = Hit.ImpactPoint.Z + CapsuleHalfHeight;
    Character->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

    FVector NewVelocity = CharacterMovement->Velocity;
    NewVelocity.Z = 0.0f;
    CharacterMovement->Velocity = NewVelocity;
}

void USOTS_ParkourAnimInstance::UpdateMotionMatchingTrajectory(const float DeltaSeconds)
{
    // Clear previous frame values so early outs leave clean data.
    TrajectoryTimeToLand = 0.0f;
    TrajectoryCollision = false;
    TrajectoryVelocityPast = FVector::ZeroVector;
    TrajectoryVelocityCurrent = FVector::ZeroVector;
    TrajectoryVelocityFuture = FVector::ZeroVector;
    FutureVelocity = FVector::ZeroVector;

    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    const ACharacter* OwnerCharacter = SandboxCharacter.Get();
    if (!OwnerCharacter || !CharacterMovement)
    {
        return;
    }

    const UWorld* World = OwnerCharacter->GetWorld();
    if (!World)
    {
        return;
    }

    // Configure sampling similar to the BP case-study: dense history, modest forward prediction.
    constexpr float HistorySamplingInterval = 0.04f;
    constexpr int32 HistorySamples = 30;
    constexpr float PredictionSamplingInterval = 0.1f;
    constexpr int32 PredictionSamples = 15;

    FPoseSearchTrajectoryData TrajectoryData;

    // Generate a trajectory using PoseSearch utilities, preserving history in Trajectory.
    FTransformTrajectory GeneratedTrajectory;
    UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectory(
        this,
        TrajectoryData,
        DeltaSeconds,
        Trajectory,
        PreviousDesiredControllerYaw,
        GeneratedTrajectory,
        HistorySamplingInterval,
        HistorySamples,
        PredictionSamplingInterval,
        PredictionSamples);

    // Apply world collision handling; keep defaults lightweight (no debug, modest obstacle height).
    FTransformTrajectory CollisionAdjustedTrajectory;
    FPoseSearchTrajectory_WorldCollisionResults CollisionResults;
    const TArray<AActor*> IgnoreActors;
    // API is experimental/deprecated for CMC; keep usage but silence warning until engine migrates.
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    UPoseSearchTrajectoryLibrary::HandleTransformTrajectoryWorldCollisions(
        this,
        this,
        GeneratedTrajectory,
        /*bApplyGravity*/ true,
        /*FloorCollisionsOffset*/ 0.01f,
        CollisionAdjustedTrajectory,
        CollisionResults,
        ETraceTypeQuery::TraceTypeQuery1,
        /*bTraceComplex*/ false,
        IgnoreActors,
        EDrawDebugTrace::None,
        /*bIgnoreSelf*/ true,
        /*MaxObstacleHeight*/ 150.0f);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS

    Trajectory = CollisionAdjustedTrajectory;
    TrajectoryTimeToLand = CollisionResults.TimeToLand;
    TrajectoryCollision = TrajectoryTimeToLand > 0.0f;

    if (Trajectory.Samples.Num() == 0)
    {
        return;
    }

    // Derive velocities from the transform trajectory at fixed windows.
    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, -0.3f, -0.2f, TrajectoryVelocityPast, /*bExtrapolate*/ false);
    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, 0.0f, 0.2f, TrajectoryVelocityCurrent, /*bExtrapolate*/ false);
    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, 0.4f, 0.5f, TrajectoryVelocityFuture, /*bExtrapolate*/ false);

    FutureVelocity = TrajectoryVelocityFuture;
}

void USOTS_ParkourAnimInstance::UpdateExperimentalStateMachine(const float /*DeltaSeconds*/)
{
    if (!UseExperimentalStateMachine)
    {
        UE_LOG(LogSOTSParkourSM, VeryVerbose, TEXT("UpdateExperimentalStateMachine: Skipped (disabled)"));
        return;
    }

    const bool bShouldForceBlend = bForceBlend;
    SetBlendStackAnimFromChooser(StateMachineState, bShouldForceBlend);

    // Consume one-shot force blend requests when driven via the property.
    if (bShouldForceBlend)
    {
        bForceBlend = false;
        UE_LOG(LogSOTSParkourSM, Verbose, TEXT("UpdateExperimentalStateMachine: Consumed one-shot force blend"));
    }
}

bool USOTS_ParkourAnimInstance::TryResolveStateMachineBlendStackNode()
{
    if (StateMachineBlendStackNode.GetAnimNodePtr<FAnimNode_BlendStack>())
    {
        return true;
    }

    // Pull the node reference from the AnimBP and convert it so we can call BlendStack helpers in C++.
    const FAnimNodeReference NodeFromBp = Get_StateMachineBlendStackNode();

    EAnimNodeReferenceConversionResult ConversionResult = EAnimNodeReferenceConversionResult::Failed;
    const FBlendStackAnimNodeReference Converted = UBlendStackAnimNodeLibrary::ConvertToBlendStackNode(NodeFromBp, ConversionResult);

    if (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded && Converted.GetAnimNodePtr<FAnimNode_BlendStack>())
    {
        StateMachineBlendStackNode = Converted;
        bLoggedMissingBlendStackNode = false;
        UE_LOG(LogSOTSParkourSM, Verbose, TEXT("TryResolveStateMachineBlendStackNode: Cached BlendStack node from BP getter"));
        return true;
    }

    if (!bLoggedMissingBlendStackNode)
    {
        UE_LOG(LogSOTSParkourSM, Warning, TEXT("TryResolveStateMachineBlendStackNode: Failed to convert AnimBP node (Result=%d)"), static_cast<int32>(ConversionResult));
        bLoggedMissingBlendStackNode = true;
    }

    return false;
}

void USOTS_ParkourAnimInstance::SetBlendStackAnimFromChooser(const ESOTS_StateMachineState InState, const bool bInForceBlend)
{
    UE_LOG(LogSOTSParkourSM, Log, TEXT("SetBlendStackAnimFromChooser: State=%d ForceBlendReq=%d"), static_cast<int32>(InState), bInForceBlend);
    StateMachineState = InState;
    Previous_BlendStackInputs = BlendStackInputs;
    bNoValidAnim = false;
    bNotifyTransition_ReTransition = false;
    bNotifyTransition_ToLoop = false;
    SearchCost = 0.0f;

    FS_ChooserOutputs ChooserOutputs;
    TArray<TObjectPtr<UAnimationAsset>> CandidateAnims;
    const bool bChooserValid = EvaluateExperimentalChooser(AnimationsForStateMachineChooser, StateMachineState, this, ChooserOutputs, CandidateAnims);

    if (!bChooserValid || CandidateAnims.Num() == 0)
    {
        // Fall back to the previous input to avoid leaving the BlendStack without an anim (prevents hovering when data is missing).
        bNoValidAnim = true;
        if (Previous_BlendStackInputs.Anim)
        {
            BlendStackInputs = Previous_BlendStackInputs;
            bNoValidAnim = false;
            if (TryResolveStateMachineBlendStackNode())
            {
                UBlendStackAnimNodeLibrary::ForceBlendNextUpdate(StateMachineBlendStackNode);
            }
            if (!bLoggedMissingChooser)
            {
                UE_LOG(LogSOTSParkourSM, Warning, TEXT("SetBlendStackAnimFromChooser: Chooser invalid/no candidates, reusing previous anim=%s (State=%d)"), BlendStackInputs.Anim ? *BlendStackInputs.Anim->GetName() : TEXT("None"), static_cast<int32>(InState));
                bLoggedMissingChooser = true;
            }
        }
        else if (!bLoggedMissingChooser)
        {
            UE_LOG(LogSOTSParkourSM, Warning, TEXT("SetBlendStackAnimFromChooser: Chooser invalid or no candidates and no fallback (State=%d)"), static_cast<int32>(InState));
            bLoggedMissingChooser = true;
        }
        return;
    }
    bLoggedMissingChooser = false;

    auto FormatTagNames = [](const TArray<FName>& Names)
    {
        if (Names.Num() == 0)
        {
            return FString(TEXT("None"));
        }
        return FString::JoinBy(Names, TEXT(","), [](const FName& Name) { return Name.ToString(); });
    };

    auto AssignDatabaseTagsFromNames = [this](const TArray<FName>& Names)
    {
        CurrentDatabaseTagNames = Names;
        CurrentDatabaseTags = Names;
    };

    bool bChangedAnim = false;

    if (!ChooserOutputs.bUseMM)
    {
        UAnimationAsset* SelectedAnim = CandidateAnims[0];
        if (!SelectedAnim)
        {
            // Attempt a graceful fallback if we have a previous valid input; otherwise flag invalid.
            bNoValidAnim = true;
            if (Previous_BlendStackInputs.Anim)
            {
                BlendStackInputs = Previous_BlendStackInputs;
                bNoValidAnim = false;
                UE_LOG(LogSOTSParkourSM, Warning, TEXT("SetBlendStackAnimFromChooser: Primary candidate null, reusing previous anim=%s (State=%d)"), BlendStackInputs.Anim ? *BlendStackInputs.Anim->GetName() : TEXT("None"), static_cast<int32>(InState));
            }
            else
            {
                UE_LOG(LogSOTSParkourSM, Warning, TEXT("SetBlendStackAnimFromChooser: Primary candidate null (State=%d)"), static_cast<int32>(InState));
            }
            return;
        }

        bool bLooping = false;
        UPoseSearchLibrary::IsAnimationAssetLooping(SelectedAnim, bLooping);

        BlendStackInputs.Anim = SelectedAnim;
        BlendStackInputs.bLoop = bLooping;
        BlendStackInputs.StartTime = ChooserOutputs.StartTime;
        BlendStackInputs.BlendTime = ChooserOutputs.BlendTime;
        BlendStackInputs.BlendProfile = ResolveBlendProfile(GetSkelMeshComponent(), ChooserOutputs.BlendProfileName);
        BlendStackInputs.Tags = ChooserOutputs.Tags;

        CurrentSelectedDatabase = nullptr;
        SearchCost = 0.0f;
        SelectedAnimIsLooping = BlendStackInputs.bLoop;
        AssignDatabaseTagsFromNames(ChooserOutputs.Tags);

        bChangedAnim = Previous_BlendStackInputs.Anim != BlendStackInputs.Anim;
        UE_LOG(LogSOTSParkourSM, Log, TEXT("SetBlendStackAnimFromChooser: Chose anim=%s Loop=%d Start=%.3f Blend=%.3f Tags=%d [%s] Changed=%d UseMM=0"), SelectedAnim ? *SelectedAnim->GetName() : TEXT("None"), BlendStackInputs.bLoop, BlendStackInputs.StartTime, BlendStackInputs.BlendTime, BlendStackInputs.Tags.Num(), *FormatTagNames(BlendStackInputs.Tags), bChangedAnim);
    }
    else
    {
        FPoseSearchBlueprintResult Result;
        if (!TryRunStateMachineMotionMatch(CandidateAnims, ChooserOutputs, Result, SearchCost))
        {
            // Keep the previous input alive to avoid popping to reference pose when MM fails.
            bNoValidAnim = true;
            if (Previous_BlendStackInputs.Anim)
            {
                BlendStackInputs = Previous_BlendStackInputs;
                bNoValidAnim = false;
                UE_LOG(LogSOTSParkourSM, Warning, TEXT("SetBlendStackAnimFromChooser: Motion match failed, reusing previous anim=%s (State=%d)"), BlendStackInputs.Anim ? *BlendStackInputs.Anim->GetName() : TEXT("None"), static_cast<int32>(InState));
            }
            else
            {
                UE_LOG(LogSOTSParkourSM, Warning, TEXT("SetBlendStackAnimFromChooser: Motion match failed (State=%d)"), static_cast<int32>(InState));
            }
            return;
        }

        UAnimationAsset* SelectedAnim = Cast<UAnimationAsset>(Result.SelectedAnim);
        if (!SelectedAnim)
        {
            bNoValidAnim = true;
            return;
        }

        bool bLooping = false;
        UPoseSearchLibrary::IsAnimationAssetLooping(SelectedAnim, bLooping);

        BlendStackInputs.Anim = SelectedAnim;
        BlendStackInputs.bLoop = bLooping;
        BlendStackInputs.StartTime = Result.SelectedTime;
        BlendStackInputs.BlendTime = ChooserOutputs.BlendTime;
        BlendStackInputs.BlendProfile = ResolveBlendProfile(GetSkelMeshComponent(), ChooserOutputs.BlendProfileName);
        BlendStackInputs.Tags = ChooserOutputs.Tags;

        CurrentSelectedDatabase = const_cast<UPoseSearchDatabase*>(Result.SelectedDatabase.Get());
        SelectedAnimIsLooping = BlendStackInputs.bLoop;
        UpdateMotionMatchingPostSelection();
        CurrentDatabaseTagNames = ChooserOutputs.Tags;

        bChangedAnim = Previous_BlendStackInputs.Anim != BlendStackInputs.Anim;
        UE_LOG(LogSOTSParkourSM, Log, TEXT("SetBlendStackAnimFromChooser: MM anim=%s Loop=%d Start=%.3f Blend=%.3f Tags=%d [%s] Changed=%d Cost=%.2f"), SelectedAnim ? *SelectedAnim->GetName() : TEXT("None"), BlendStackInputs.bLoop, BlendStackInputs.StartTime, BlendStackInputs.BlendTime, BlendStackInputs.Tags.Num(), *FormatTagNames(BlendStackInputs.Tags), bChangedAnim, SearchCost);
    }

    bNotifyTransition_ReTransition = bChangedAnim;
    bNotifyTransition_ToLoop = bChangedAnim && BlendStackInputs.bLoop;

    // Match the case-study behavior: always push a blend when a valid selection exists.
    const bool bHasBlendStackNode = TryResolveStateMachineBlendStackNode();
    if (bHasBlendStackNode)
    {
        UBlendStackAnimNodeLibrary::ForceBlendNextUpdate(StateMachineBlendStackNode);
        UE_LOG(LogSOTSParkourSM, Verbose, TEXT("SetBlendStackAnimFromChooser: ForceBlendNextUpdate; ReTransition=%d ToLoop=%d BlendProfile=%s ForceBlendReq=%d"), bNotifyTransition_ReTransition, bNotifyTransition_ToLoop, BlendStackInputs.BlendProfile ? *BlendStackInputs.BlendProfile->GetName() : TEXT("None"), bInForceBlend);
    }
    else
    {
        if (!bLoggedMissingBlendStackNode)
        {
            UE_LOG(LogSOTSParkourSM, Warning, TEXT("SetBlendStackAnimFromChooser: No BlendStack node set; implement Get_StateMachineBlendStackNode in AnimBP"));
            bLoggedMissingBlendStackNode = true;
        }
    }
}

void USOTS_ParkourAnimInstance::UpdateMotionMatchingData(const float DeltaSeconds)
{
    // Minimal scaffolding to mirror legacy ABP MM variables; replace with real GASP/MM integration later.
    MMDatabaseLOD = 0;
    CurrentSelectedDatabase = nullptr;
    CurrentDatabaseTags.Reset();
    CurrentDatabaseTagNames.Reset();
    NoValidAnim = false;
    SelectedAnimIsLooping = false;
    MMBlendTime = 0.5f;
    MMNotifyRecencyTimeout = 0.2f;
    MMInterruptMode = EPoseSearchInterruptMode::DoNotInterrupt;

    UpdateMotionMatchingTrajectory(DeltaSeconds);
    
    // If no database was selected, mirror the old "No Valid Anim" gate for the state machine.
    NoValidAnim = (CurrentSelectedDatabase == nullptr);

    // Placeholder: if we later store the chosen anim/sequence, update this based on its looping flag.
    // For now, mirror the legacy behavior: treat absence of selection as non-looping.
    SelectedAnimIsLooping = false;

    // --- Ported MM helpers (interrupt mode, blend time, notify recency) ---
    // Interrupt mode: interrupt when core locomotion state changes (movement state/gait/stance/mode deltas).
    const bool bMovementStateChanged = (MovementState != MovementState_LastFrame);
    const bool bGaitChanged = (Gait != Gait_LastFrame);
    const bool bStanceChanged = (Stance != Stance_LastFrame);
    const bool bMovementModeChanged = (MovementMode != MovementMode_LastFrame);
    const bool bCoreChanged = bMovementStateChanged || bGaitChanged || bStanceChanged || bMovementModeChanged;
    MMInterruptMode = bCoreChanged ? EPoseSearchInterruptMode::InterruptOnDatabaseChange : EPoseSearchInterruptMode::DoNotInterrupt;

    // Blend time: mirrors AnimBP Get_MMBlendTime logic (OnGround vs InAir cases).
    MMBlendTime = 0.5f;
    const bool bNowGround = (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking);
    const bool bWasAir = (MovementMode_LastFrame == MOVE_Falling || MovementMode_LastFrame == MOVE_Flying || MovementMode_LastFrame == MOVE_Swimming);
    if (bNowGround)
    {
        // Short blend on air -> ground to reduce visible stall.
        MMBlendTime = bWasAir ? 0.2f : 0.5f;
    }
    else if (MovementMode == MOVE_Falling || MovementMode == MOVE_Flying || MovementMode == MOVE_Swimming)
    {
        const float UpSpeed = Velocity.Z;
        MMBlendTime = (UpSpeed > 100.0f) ? 0.15f : 0.5f; // very short jump blend when rising fast
    }

    // Notify recency timeout: per-gait constants (copied from ABP literals: Walking/Jogging=0.2, Sprinting=0.16).
    MMNotifyRecencyTimeout = 0.2f;
    if (Gait == E_Gait::Sprinting)
    {
        MMNotifyRecencyTimeout = 0.16f;
    }

    // Post-selection bookkeeping owned in C++ (parity with BP Update_MotionMatching_PostSelection).
    UpdateMotionMatchingPostSelection();
}

void USOTS_ParkourAnimInstance::UpdateMotionMatchingPostSelection()
{
    CurrentDatabaseTags.Reset();

    const UPoseSearchDatabase* Database = Cast<UPoseSearchDatabase>(CurrentSelectedDatabase.Get());
    if (!Database)
    {
        return;
    }

    CurrentDatabaseTags.Reset();
    TArray<FName> DatabaseTags;
    UPoseSearchLibrary::GetDatabaseTags(Database, DatabaseTags);

    CurrentDatabaseTagNames = DatabaseTags;
    CurrentDatabaseTags = DatabaseTags;
}

void USOTS_ParkourAnimInstance::OnParkourResultUpdated_Implementation(const FSOTS_ParkourResult& Result)
{
    ApplyResultToAnimData(Result);
}

void USOTS_ParkourAnimInstance::OnParkourStateChanged_Implementation(ESOTS_ParkourState /*NewState*/)
{
    if (ParkourComponent)
    {
        ParkourState = ParkourComponent->GetParkourStateTag();
        ParkourLogicalState = ParkourComponent->GetParkourStateTag();
    }
}

void USOTS_ParkourAnimInstance::SetParkourStateTag_Implementation(FGameplayTag StateTag)
{
    ParkourState = StateTag;
    ParkourLogicalState = StateTag;
}

void USOTS_ParkourAnimInstance::SetParkourActionTag_Implementation(FGameplayTag ActionTag)
{
    ParkourAction = ActionTag;
}

void USOTS_ParkourAnimInstance::SetClimbStyleTag_Implementation(FGameplayTag ClimbStyleTag)
{
    ClimbStyle = ClimbStyleTag;
}

void USOTS_ParkourAnimInstance::SetClimbMovement_Implementation(const FVector& ClimbMovementWS)
{
    ClimbMovement = ClimbMovementWS;
}

void USOTS_ParkourAnimInstance::SetLeftHandLedgeLocation_Implementation(const FVector& Location)
{
    LeftHandLedgeLocation = Location;
}

void USOTS_ParkourAnimInstance::SetRightHandLedgeLocation_Implementation(const FVector& Location)
{
    RightHandLedgeLocation = Location;
}

void USOTS_ParkourAnimInstance::SetLeftHandLedgeRotation_Implementation(const FRotator& Rotation)
{
    LeftHandLedgeRotation = Rotation;
}

void USOTS_ParkourAnimInstance::SetRightHandLedgeRotation_Implementation(const FRotator& Rotation)
{
    RightHandLedgeRotation = Rotation;
}

void USOTS_ParkourAnimInstance::SetLeftFootLocation_Implementation(const FVector& Location)
{
    LeftFootLocation = Location;
}

void USOTS_ParkourAnimInstance::SetRightFootLocation_Implementation(const FVector& Location)
{
    RightFootLocation = Location;
}

void USOTS_ParkourAnimInstance::SetLeftClimbIK_Implementation(bool bEnable)
{
    bLeftClimbIK = bEnable;
}

void USOTS_ParkourAnimInstance::SetRightClimbIK_Implementation(bool bEnable)
{
    bRightClimbIK = bEnable;
}

float USOTS_ParkourAnimInstance::Get_LandVelocity() const
{
    return LandVelocity.Z;
}

float USOTS_ParkourAnimInstance::Get_TrajectoryTurnAngle() const
{
    const float FutureYaw = FutureVelocity.Rotation().Yaw;
    const float CurrentYaw = Velocity.Rotation().Yaw;
    return FMath::FindDeltaAngleDegrees(CurrentYaw, FutureYaw);
}

bool USOTS_ParkourAnimInstance::JustLanded_Heavy() const
{
    return JustLandedHeavy;
}

bool USOTS_ParkourAnimInstance::JustLanded_Light() const
{
    return JustLandedLight;
}

bool USOTS_ParkourAnimInstance::Get_TrajectoryCollision() const
{
    return TrajectoryCollision;
}

bool USOTS_ParkourAnimInstance::PlayLand() const
{
    const bool bWasAir = (MovementMode_LastFrame == MOVE_Falling || MovementMode_LastFrame == MOVE_Flying || MovementMode_LastFrame == MOVE_Swimming);
    const bool bNowGround = (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking);
    return bWasAir && bNowGround;
}

bool USOTS_ParkourAnimInstance::PlayMovingLand() const
{
    if (!PlayLand())
    {
        return false;
    }

    const float TurnAngleAbs = FMath::Abs(Get_TrajectoryTurnAngle());
    return TurnAngleAbs <= 120.0f;
}

bool USOTS_ParkourAnimInstance::IsPivotingBP() const
{
    return IsPivoting;
}

double USOTS_ParkourAnimInstance::Get_DynamicPlayRate(const FAnimNodeReference& BlendStackInput) const
{
    // Mirrors the AnimBP Get_DynamicPlayRate graph: sample curves on the current BlendStack anim.
    UAnimationAsset* AnimAsset = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAsset(BlendStackInput);
    if (!AnimAsset)
    {
        return 1.0;
    }

    const UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimAsset);
    if (!AnimSequence)
    {
        return 1.0;
    }

    const float AnimTime = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAssetTime(BlendStackInput);

    const auto SampleCurve = [AnimSequence](const FName& CurveName, float Time, float& OutValue) -> bool
    {
        return UAnimationWarpingLibrary::GetCurveValueFromAnimation(AnimSequence, CurveName, Time, OutValue);
    };

    float AlphaCurve = 0.0f;
    if (!SampleCurve(TEXT("Enable_PlayRateWarping"), AnimTime, AlphaCurve))
    {
        return 1.0;
    }

    float SpeedCurve = 0.0f;
    if (!SampleCurve(TEXT("MoveData_Speed"), AnimTime, SpeedCurve))
    {
        return 1.0;
    }

    float MaxDynamicPlayRate = 1.25f;
    SampleCurve(TEXT("MaxDynamicPlayRate"), AnimTime, MaxDynamicPlayRate);

    float MinDynamicPlayRate = 0.75f;
    SampleCurve(TEXT("MinDynamicPlayRate"), AnimTime, MinDynamicPlayRate);

    const double AnimSpeed = FMath::Clamp(static_cast<double>(SpeedCurve), 1.0, 999.0);
    const double Desired = FMath::Clamp(
        FMath::IsNearlyZero(AnimSpeed) ? 0.0 : static_cast<double>(Speed2D) / AnimSpeed,
        static_cast<double>(MinDynamicPlayRate),
        static_cast<double>(MaxDynamicPlayRate));

    return FMath::Lerp(1.0, Desired, static_cast<double>(AlphaCurve));
}

EOffsetRootBoneMode USOTS_ParkourAnimInstance::Get_OffsetRootRotationMode() const
{
    // Mirror BP: if DefaultSlot is active, release the offset; otherwise accumulate so warping can apply.
    const bool bSlotActive = IsSlotActive(TEXT("DefaultSlot"));
    return bSlotActive ? EOffsetRootBoneMode::Release : EOffsetRootBoneMode::Accumulate;
}

EOffsetRootBoneMode USOTS_ParkourAnimInstance::Get_OffsetRootTranslationMode() const
{
    // BP logic:
    // - If DefaultSlot active: Release (no translation warping during montage slot)
    // - Else if grounded: interpolate when moving, otherwise release
    // - Else (in air): release
    const bool bSlotActive = IsSlotActive(TEXT("DefaultSlot"));
    if (bSlotActive)
    {
        return EOffsetRootBoneMode::Release;
    }

    const bool bGrounded = (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking);
    if (bGrounded)
    {
        return IsMoving ? EOffsetRootBoneMode::Interpolate : EOffsetRootBoneMode::Release;
    }

    return EOffsetRootBoneMode::Release;
}

float USOTS_ParkourAnimInstance::Get_OffsetRootTranslationHalfLife() const
{
    // BP switch on movement state: Idle=0.1, Moving=0.3.
    return HasVelocity ? 0.3f : 0.1f;
}

float USOTS_ParkourAnimInstance::Get_OffsetRootTranslationRadius() const
{
    return OffsetRootTranslationRadius;
}

EOrientationWarpingSpace USOTS_ParkourAnimInstance::Get_OrientationWarpingWarpingSpace() const
{
    // BP: use RootBoneTransform when offset root is enabled; otherwise ComponentTransform.
    return OffsetRootBoneEnabled ? EOrientationWarpingSpace::RootBoneTransform : EOrientationWarpingSpace::ComponentTransform;
}

bool USOTS_ParkourAnimInstance::Enable_AO() const
{
    const FVector2D AO = Get_AOValue();
    const double AbsYaw = FMath::Abs(AO.X);

    // BP Select on movement state: Idle -> 115, Moving -> 180.
    const double YawThreshold = HasVelocity ? 180.0 : 115.0;

    const bool bYawGate = AbsYaw <= YawThreshold;
    const bool bRotationModeGate = (RotationMode == ESOTS_RotationMode::Strafe);
    const float SlotWeight = GetSlotMontageLocalWeight(TEXT("DefaultSlot"));
    const bool bSlotGate = SlotWeight < 0.5f;

    return bYawGate && bRotationModeGate && bSlotGate;
}

FVector2D USOTS_ParkourAnimInstance::Get_AOValue() const
{
    const bool bLocallyControlled = SandboxCharacter && SandboxCharacter->IsLocallyControlled();
    const FRotator ControlRotation = SandboxCharacter
        ? (bLocallyControlled ? SandboxCharacter->GetControlRotation() : SandboxCharacter->GetBaseAimRotation())
        : FRotator::ZeroRotator;

    const FRotator RootRotation = RootTransform.GetRotation().Rotator();

    FRotator Delta = ControlRotation - RootRotation;
    Delta.Normalize();

    const FVector DeltaAsVector(Delta.Yaw, Delta.Pitch, 0.0f);
    const float DisableAOAlpha = GetCurveValue(TEXT("Disable_AO"));
    const FVector Blended = FMath::Lerp(DeltaAsVector, FVector::ZeroVector, DisableAOAlpha);
    return FVector2D(Blended.X, Blended.Y);
}

float USOTS_ParkourAnimInstance::Get_AO_Yaw() const
{
    return (RotationMode == ESOTS_RotationMode::Strafe) ? Get_AOValue().X : 0.0f;
}

FVector USOTS_ParkourAnimInstance::CalculateRelativeAccelerationAmount() const
{
    const UCharacterMovementComponent* Move = CharacterMovement.Get();
    if (!Move)
    {
        return FVector::ZeroVector;
    }

    const float MaxAccelerationMagnitude = Move->GetMaxAcceleration();
    const float MaxBrakingMagnitude = Move->GetMaxBrakingDeceleration();

    if (MaxAccelerationMagnitude <= KINDA_SMALL_NUMBER || MaxBrakingMagnitude <= KINDA_SMALL_NUMBER)
    {
        return FVector::ZeroVector;
    }

    // Acceleration vs deceleration gate: dot(Acceleration, Velocity) mirrors the BP branch.
    const bool bIsAccelerating = FVector::DotProduct(Acceleration, Velocity) > 0.0f;
    const float MaxMagnitude = bIsAccelerating ? MaxAccelerationMagnitude : MaxBrakingMagnitude;

    const FVector Clamped = VelocityAcceleration.GetClampedToMaxSize(MaxMagnitude);
    const FVector Normalized = Clamped / MaxMagnitude;

    // Rotate by the actor transform (same as BP vector << rotator).
    const FQuat ActorRot = CharacterTransform.GetRotation();
    return ActorRot.RotateVector(Normalized);
}

FVector2D USOTS_ParkourAnimInstance::Get_LeanAmount() const
{
    const FVector RelativeAcceleration = CalculateRelativeAccelerationAmount();
    const double SpeedAlpha = FMath::GetMappedRangeValueClamped(FVector2D(200.0, 500.0), FVector2D(0.5, 1.0), Speed2D);

    const double LeanX = RelativeAcceleration.Y * SpeedAlpha;
    return FVector2D(LeanX, 0.0);
}

bool USOTS_ParkourAnimInstance::EnableSteering() const
{
    const bool bIsMoving = HasVelocity;
    const bool bIsSwimming = (MovementMode == MOVE_Swimming);
    return bIsMoving || bIsSwimming;
}

FQuat USOTS_ParkourAnimInstance::GetDesiredFacing() const
{
    if (UseExperimentalStateMachine)
    {
        // Experimental path mirrors the BP: target rotation plus the AnimGraph strafe offset (-90 yaw).
        const FRotator Adjusted = UKismetMathLibrary::ComposeRotators(TargetRotation, FRotator(0.0f, -90.0f, 0.0f));
        return Adjusted.Quaternion();
    }

    if (Trajectory.Samples.Num() > 0)
    {
        FTransformTrajectorySample TrajectorySample;
        UPoseSearchTrajectoryLibrary::GetTransformTrajectorySampleAtTime(Trajectory, 0.5f, TrajectorySample, false);
        return TrajectorySample.Facing;
    }

    return FQuat::Identity;
}

FQuat USOTS_ParkourAnimInstance::Get_DesiredFacing() const
{
    return GetDesiredFacing();
}

void USOTS_ParkourAnimInstance::OnMotionMatchingStateUpdated()
{
    // AnimGraph hook: mirrors the legacy BP OnMotionMatchingStateUpdated delegate.
    UpdateMotionMatchingPostSelection();
}

void USOTS_ParkourAnimInstance::SetMovementDirection(ESOTS_MovementDirection InDirection)
{
    MovementDirection_LastFrame = MovementDirection;
    MovementDirection = InDirection;
}

// Default implementations for state machine bindings (BP can override or replace calls).
void USOTS_ParkourAnimInstance::OnStateEntry_IdleLoop()
{
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("OnStateEntry_IdleLoop"));
    SetBlendStackAnimFromChooser(ESOTS_StateMachineState::IdleLoop, false);
}

void USOTS_ParkourAnimInstance::OnStateEntry_TransitionToIdleLoop()
{
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("OnStateEntry_TransitionToIdleLoop"));
    SetBlendStackAnimFromChooser(ESOTS_StateMachineState::TransitionToIdleLoop, true);
}

void USOTS_ParkourAnimInstance::OnStateEntry_LocomotionLoop()
{
    TargetRotationOnTransitionStart = TargetRotation;
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("OnStateEntry_LocomotionLoop TargetRotation=%s"), *TargetRotation.ToString());
    SetBlendStackAnimFromChooser(ESOTS_StateMachineState::LocomotionLoop, false);
}

void USOTS_ParkourAnimInstance::OnStateEntry_TransitionToLocomotionLoop()
{
    TargetRotationOnTransitionStart = TargetRotation;
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("OnStateEntry_TransitionToLocomotionLoop TargetRotation=%s"), *TargetRotation.ToString());
    SetBlendStackAnimFromChooser(ESOTS_StateMachineState::TransitionToLocomotionLoop, true);
}
void USOTS_ParkourAnimInstance::OnUpdate_TransitionToLocomotionLoop()
{
    const float DeltaSeconds = GetDeltaSeconds();
    TargetRotationOnTransitionStart = UKismetMathLibrary::RInterpTo(TargetRotationOnTransitionStart, TargetRotation, DeltaSeconds, 5.0f);
    UE_LOG(LogSOTSParkourSM, VeryVerbose, TEXT("OnUpdate_TransitionToLocomotionLoop Delta=%.4f Target=%s"), DeltaSeconds, *TargetRotationOnTransitionStart.ToString());
}
void USOTS_ParkourAnimInstance::OnStateEntry_InAirLoop()
{
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("OnStateEntry_InAirLoop"));
    SetBlendStackAnimFromChooser(ESOTS_StateMachineState::InAirLoop, false);
}

void USOTS_ParkourAnimInstance::OnStateEntry_TransitionToInAirLoop()
{
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("OnStateEntry_TransitionToInAirLoop"));
    SetBlendStackAnimFromChooser(ESOTS_StateMachineState::TransitionToInAirLoop, true);
}

void USOTS_ParkourAnimInstance::OnStateEntry_IdleBreak()
{
    UE_LOG(LogSOTSParkourSM, Verbose, TEXT("OnStateEntry_IdleBreak"));
    SetBlendStackAnimFromChooser(ESOTS_StateMachineState::IdleBreak, true);
}

FFootPlacementPlantSettings USOTS_ParkourAnimInstance::Get_FootPlacementPlantSettings() const
{
    const bool bUseStops = UseExperimentalStateMachine ? HasStopTag(BlendStackInputs.Tags) : HasStopTag(CurrentDatabaseTags);
    return bUseStops ? PlantSettings_Stops : PlantSettings_Default;
}

FFootPlacementInterpolationSettings USOTS_ParkourAnimInstance::Get_FootPlacementInterpolationSettings() const
{
    const bool bUseStops = UseExperimentalStateMachine ? HasStopTag(BlendStackInputs.Tags) : HasStopTag(CurrentDatabaseTags);
    return bUseStops ? InterpolationSettings_Stops : InterpolationSettings_Default;
}

void USOTS_ParkourAnimInstance::PrintParkourSnapshot() const
{
    UE_LOG(LogTemp, Log, TEXT("=== Parkour Anim Snapshot ==="));
    UE_LOG(LogTemp, Log, TEXT("State: %s  Logical: %s  Action: %s  ClimbStyle: %s  Direction: %s"),
        *ParkourState.ToString(),
        *ParkourLogicalState.ToString(),
        *ParkourAction.ToString(),
        *ClimbStyle.ToString(),
        *ClimbDirection.ToString());

    UE_LOG(LogTemp, Log, TEXT("IK: ParkourIK=%s  LegIK=%s  HidingOnBeam=%s  LeftIK=%s  RightIK=%s"),
        ParkourIK_Enabled ? TEXT("true") : TEXT("false"),
        LegIK ? TEXT("true") : TEXT("false"),
        IsHidingOnBeam ? TEXT("true") : TEXT("false"),
        bLeftClimbIK ? TEXT("true") : TEXT("false"),
        bRightClimbIK ? TEXT("true") : TEXT("false"));

    UE_LOG(LogTemp, Log, TEXT("Hands L(%s) R(%s)  Feet L(%s) R(%s)"),
        *LeftHandLedgeLocation.ToString(),
        *RightHandLedgeLocation.ToString(),
        *LeftFootLocation.ToString(),
        *RightFootLocation.ToString());

    UE_LOG(LogTemp, Log, TEXT("Movement: X=%.2f Y=%.2f  Forward=%s  Crouched=%s InCover=%s"),
        CurrentMovementX,
        CurrentMovementY,
        *PlayersForwardVector.ToString(),
        IsPlayerCrouched ? TEXT("true") : TEXT("false"),
        IsInCover_ABP ? TEXT("true") : TEXT("false"));
}
