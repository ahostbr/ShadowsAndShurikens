// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#include "Camera/UGC_PlayerCameraManager.h"

#include "Camera/Data/UGC_CameraData.h"
#include "Camera/Modifiers/UGC_CameraAnimationModifier.h"
#include "Camera/Modifiers/UGC_CameraModifier.h"
#include "Camera/Modifiers/UGC_PlayCameraAnimCallbackProxy.h"
#include "CameraAnimationSequence.h"
#include "DrawDebugHelpers.h"
#include "Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Kismet/KismetMathLibrary.h"
#include "Pawn/UGC_PawnInterface.h"

AUGC_PlayerCameraManager::AUGC_PlayerCameraManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AUGC_PlayerCameraManager::InitializeFor(APlayerController* PC)
{
    Super::InitializeFor(PC);
    RefreshLevelSequences();
}

void AUGC_PlayerCameraManager::RefreshLevelSequences()
{
    // This resets the array and gets all actors of class.
    QUICK_SCOPE_CYCLE_COUNTER(AUGC_PlayerCameraManager_RefreshLevelSequences);
    LevelSequences.Reset();

    for (TActorIterator<ALevelSequenceActor> It(GetWorld()); It; ++It)
    {
        ALevelSequenceActor* LevelSequence = *It;
        LevelSequences.Add(LevelSequence);

        LevelSequence->SequencePlayer->OnPlay.AddDynamic(this, &AUGC_PlayerCameraManager::OnLevelSequenceStarted);
        LevelSequence->SequencePlayer->OnPlayReverse.AddDynamic(this, &AUGC_PlayerCameraManager::OnLevelSequenceStarted);
        LevelSequence->SequencePlayer->OnStop.AddDynamic(this, &AUGC_PlayerCameraManager::OnLevelSequenceEnded);
    }
}

void AUGC_PlayerCameraManager::OnLevelSequenceStarted()
{
    ++NbrActiveLevelSequences;
    DoForEachUGCModifier(&UUGC_CameraModifier::OnAnyLevelSequenceStarted);
}

void AUGC_PlayerCameraManager::OnLevelSequenceEnded()
{
    --NbrActiveLevelSequences;
    ensure(NbrActiveLevelSequences >= 0);
    DoForEachUGCModifier(&UUGC_CameraModifier::OnAnyLevelSequenceEnded);
}

void AUGC_PlayerCameraManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateInternalVariables(DeltaTime);
}

bool AUGC_PlayerCameraManager::IsPlayingAnyCameraAnimation() const
{
    if (UUGC_CameraAnimationModifier const* CameraAnimModifier = FindCameraModifierOfType<UUGC_CameraAnimationModifier>())
    {
        return CameraAnimModifier && CameraAnimModifier->IsAnyCameraAnimationSequence();
    }
    return false;
}

void AUGC_PlayerCameraManager::PlayCameraAnimation(UCameraAnimationSequence* CameraSequence, FUGCCameraAnimationParams const& Params)
{
    if (UUGC_CameraAnimationModifier* CameraAnimModifier = FindCameraModifierOfType<UUGC_CameraAnimationModifier>())
    {
        CameraAnimModifier->PlaySingleCameraAnimation(CameraSequence, static_cast<FCameraAnimationParams>(Params), Params.ResetType);
    }
}

void AUGC_PlayerCameraManager::SetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams)
{
    auto OldPendingTarget = PendingViewTarget.Target;
    auto OldTarget = ViewTarget.Target;

    Super::SetViewTarget(NewViewTarget, TransitionParams);

    if (!OwnerPawn)
    {
        return;
    }

    bool const bAssignedNewTarget = ViewTarget.Target != OldTarget;
    bool const bBlendingToNewTarget = PendingViewTarget.Target != OldPendingTarget;
    if (bAssignedNewTarget || bBlendingToNewTarget)
    {
        bool const bWasImmediate = bAssignedNewTarget && !bBlendingToNewTarget;
        bool bNewTargetIsOwner = false;

        // If old character has been unpossessed, then our new taget is the owner!
        if (bWasImmediate && OwnerPawn->GetController() == nullptr)
        {
            bNewTargetIsOwner = true;
        }
        else
        {
            bNewTargetIsOwner = bWasImmediate ? ViewTarget.Target == OwnerPawn : PendingViewTarget.Target == OwnerPawn;
        }

        DoForEachUGCModifier([bNewTargetIsOwner, bWasImmediate](UUGC_CameraModifier* UGCModifier)
        {
            UGCModifier->OnSetViewTarget(bWasImmediate, bNewTargetIsOwner);
        });
    }
}

UCameraModifier* AUGC_PlayerCameraManager::FindCameraModifierOfClass(TSubclassOf<UCameraModifier> ModifierClass, bool bIncludeInherited)
{
    for (UCameraModifier* Mod : ModifierList)
    {
        if (bIncludeInherited)
        {
            if (Mod->GetClass()->IsChildOf(ModifierClass))
            {
                return Mod;
            }
        }
        else
        {
            if (Mod->GetClass() == ModifierClass)
            {
                return Mod;
            }
        }
    }

    return nullptr;
}

UCameraModifier const* AUGC_PlayerCameraManager::FindCameraModifierOfClass(TSubclassOf<UCameraModifier> ModifierClass, bool bIncludeInherited) const
{
    for (UCameraModifier* Mod : ModifierList)
    {
        if (bIncludeInherited)
        {
            if (Mod->GetClass()->IsChildOf(ModifierClass))
            {
                return Mod;
            }
        }
        else
        {
            if (Mod->GetClass() == ModifierClass)
            {
                return Mod;
            }
        }
    }

    return nullptr;
}

void AUGC_PlayerCameraManager::ToggleUGCCameraModifiers(bool const bEnabled, bool const bImmediate)
{
    DoForEachUGCModifier([bEnabled, bImmediate](UUGC_CameraModifier* UGCModifier)
    {
        if (bEnabled)
        {
            UGCModifier->EnableModifier();
        }
        else
        {
            UGCModifier->DisableModifier(bImmediate);
        }
    });
}

void AUGC_PlayerCameraManager::ToggleCameraModifiers(bool const bEnabled, bool const bImmediate)
{
    for (int32 ModifierIdx = 0; ModifierIdx < ModifierList.Num(); ModifierIdx++)
    {
        if (ModifierList[ModifierIdx] != nullptr)
        {
            if (bEnabled)
            {
                ModifierList[ModifierIdx]->EnableModifier();
            }
            else
            {
                ModifierList[ModifierIdx]->DisableModifier(bImmediate);
            }
        }
    }
}

void AUGC_PlayerCameraManager::ToggleAllUGCModifiersDebug(bool const bEnabled)
{
    DoForEachUGCModifier([bEnabled](UUGC_CameraModifier* UGCModifier)
    {
        if (!UGCModifier->IsDisabled())
        {
            UGCModifier->bDebug = bEnabled;
        }
    });
}

void AUGC_PlayerCameraManager::ToggleAllModifiersDebug(bool const bEnabled)
{
    for (int32 ModifierIdx = 0; ModifierIdx < ModifierList.Num(); ModifierIdx++)
    {
        if (ModifierList[ModifierIdx] != nullptr && !ModifierList[ModifierIdx]->IsDisabled())
        {
            ModifierList[ModifierIdx]->bDebug = bEnabled;
        }
    }
}

void AUGC_PlayerCameraManager::PushCameraData(UUGC_CameraDataAssetBase* CameraDA)
{
    CameraDataStack.Push(CameraDA);
    OnCameraDataStackChanged(CameraDA);
}

void AUGC_PlayerCameraManager::PopCameraDataHead()
{
    CameraDataStack.Pop();
    OnCameraDataStackChanged(CameraDataStack.IsEmpty() ? nullptr : CameraDataStack[CameraDataStack.Num() - 1]);
}

void AUGC_PlayerCameraManager::PopCameraData(UUGC_CameraDataAssetBase* CameraDA)
{
    if (CameraDataStack.IsEmpty())
    {
        return;
    }

    if (GetCurrentCameraDataAsset() == CameraDA)
    {
        PopCameraDataHead();
    }

    CameraDataStack.Remove(CameraDA);
}

void AUGC_PlayerCameraManager::OnCameraDataStackChanged_Implementation(UUGC_CameraDataAssetBase* CameraDA)
{
}

void AUGC_PlayerCameraManager::ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot)
{
    Super::ProcessViewRotation(DeltaTime, OutViewRotation, OutDeltaRot);

    if (PCOwner && ViewTarget.Target)
    {
        FRotator const ControlRotation = PCOwner->GetControlRotation();
        FRotator const OwnerRotation = ViewTarget.Target->GetActorRotation();
        FRotator InLocalControlRotation = ControlRotation - OwnerRotation;
        InLocalControlRotation.Normalize();

        float OutPitchTurnRate = PitchTurnRate;
        float OutYawTurnRate = YawTurnRate;

        ProcessTurnRate(DeltaTime, InLocalControlRotation, OutPitchTurnRate, OutYawTurnRate);

        PitchTurnRate = FMath::Clamp(OutPitchTurnRate, 0.f, 1.f);
        YawTurnRate = FMath::Clamp(OutYawTurnRate, 0.f, 1.f);
    }
}

void AUGC_PlayerCameraManager::ProcessTurnRate(float DeltaTime, FRotator InLocalControlRotation, float& OutPitchTurnRate, float& OutYawTurnRate)
{
    DoForEachUGCModifierWithBreak([&](UUGC_CameraModifier* UGCModifier) -> bool
    {
        if (!UGCModifier->IsDisabled())
        {
            if (!UGCModifier->CanPlayDuringCameraAnimation())
            {
                if (IsPlayingAnyCameraAnimation())
                {
                    return false;
                }
            }

            return UGCModifier->ProcessTurnRate(DeltaTime, InLocalControlRotation, PitchTurnRate, YawTurnRate, OutPitchTurnRate, OutYawTurnRate);
        }
        return false; // Don't break
    });
}

void AUGC_PlayerCameraManager::UpdateInternalVariables_Implementation(float DeltaTime)
{
    AspectRatio = GetCameraCacheView().AspectRatio;
    HorizontalFOV = GetFOVAngle();
    ensureAlways(!FMath::IsNearlyZero(AspectRatio));
    VerticalFOV = FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(HorizontalFOV) * 0.5f) / AspectRatio));

    if (PCOwner && PCOwner->GetPawn())
    {
        APawn* OwnerPawnNew = PCOwner->GetPawn();
        if (!OwnerPawn || OwnerPawnNew != OwnerPawn)
        {
            // Either initialising reference, or we have possessed a new character.
            OwnerPawn = OwnerPawnNew;
            if (OwnerCharacter = PCOwner->GetPawn<ACharacter>())
            {
                MovementComponent = OwnerCharacter->GetCharacterMovement();
            }
            CameraArm = OwnerPawn->FindComponentByClass<USpringArmComponent>();

        }

        if (OwnerPawn)
        {
            MovementInput = GetMovementControlInput();
            bHasMovementInput = !MovementInput.IsZero();
            TimeSinceMovementInput = bHasMovementInput ? 0.f : TimeSinceMovementInput + DeltaTime;

            RotationInput = GetRotationInput();
            bHasRotationInput = !RotationInput.IsZero();
            TimeSinceRotationInput = bHasRotationInput ? 0.f : TimeSinceRotationInput + DeltaTime;
        }
    }
}

FRotator AUGC_PlayerCameraManager::GetRotationInput_Implementation() const
{
    FRotator RotInput = FRotator::ZeroRotator;

    if (OwnerPawn && OwnerPawn->GetClass()->ImplementsInterface(UUGC_PawnInterface::StaticClass()))
    {
        RotInput = IUGC_PawnInterface::Execute_GetRotationInput(OwnerPawn);
    }
    return RotInput;
}

FVector AUGC_PlayerCameraManager::GetMovementControlInput_Implementation() const
{
    FVector MovInput = FVector::ZeroVector;

    if (OwnerPawn && OwnerPawn->GetClass()->ImplementsInterface(UUGC_PawnInterface::StaticClass()))
    {
        MovInput = IUGC_PawnInterface::Execute_GetMovementInput(OwnerPawn);
    }
    return MovInput;
}

// Limit the view yaw in local space instead of world space.
void AUGC_PlayerCameraManager::LimitViewYaw(FRotator& ViewRotation, float InViewYawMin, float InViewYawMax)
{
    if (PCOwner && PCOwner->GetPawn())
    {
        FRotator ActorRotation = PCOwner->GetPawn()->GetActorRotation();
        ViewRotation.Yaw = FMath::ClampAngle(ViewRotation.Yaw, ActorRotation.Yaw + InViewYawMin, ActorRotation.Yaw + InViewYawMax);
        ViewRotation.Yaw = FRotator::ClampAxis(ViewRotation.Yaw);
    }
}

/** Draw a debug camera shape. */
void AUGC_PlayerCameraManager::DrawDebugCamera(float Duration, bool bDrawCamera, FLinearColor CameraColor, bool bDrawSpringArm, FLinearColor SpringArmColor, float Thickness)
{
#if ENABLE_DRAW_DEBUG
    if (bDrawSpringArm && CameraArm)
    {
        FVector const SafeLocation = CameraArm->GetComponentLocation() + CameraArm->TargetOffset;
        ::DrawDebugLine(GetWorld(), GetCameraCacheView().Location, SafeLocation, SpringArmColor.ToFColor(true), false, Duration, 0, Thickness);
    }

    if (bDrawCamera)
    {
        ::DrawDebugCamera(GetWorld(), GetCameraCacheView().Location, GetCameraCacheView().Rotation, GetCameraCacheView().FOV, 1.0f, CameraColor.ToFColor(true), false, Duration);
    }
#endif
}

void AUGC_PlayerCameraManager::DoForEachUGCModifier(TFunction<void(UUGC_CameraModifier*)> const& Function)
{
    if (Function)
    {
        for (int32 ModifierIdx = 0; ModifierIdx < UGCModifiersList.Num(); ++ModifierIdx)
        {
            ensure(UGCModifiersList[ModifierIdx]);

            if (UGCModifiersList[ModifierIdx])
            {
                Function(UGCModifiersList[ModifierIdx]);
            }
        }
    }
}

void AUGC_PlayerCameraManager::DoForEachUGCModifierWithBreak(TFunction<bool(UUGC_CameraModifier*)> const& Function)
{
    if (Function)
    {
        for (int32 ModifierIdx = 0; ModifierIdx < UGCModifiersList.Num(); ++ModifierIdx)
        {
            ensure(UGCModifiersList[ModifierIdx]);

            if (UGCModifiersList[ModifierIdx])
            {
                if (Function(UGCModifiersList[ModifierIdx]))
                {
                    break;
                }
            }
        }
    }
}

UCameraModifier* AUGC_PlayerCameraManager::AddNewCameraModifier(TSubclassOf<UCameraModifier> ModifierClass)
{
    UCameraModifier* AddedModifier = Super::AddNewCameraModifier(ModifierClass);
    if (AddedModifier)
    {
        if (UUGC_CameraModifier* UGCModifier = Cast<UUGC_CameraModifier>(AddedModifier))
        {
            UGCModifiersList.Add(UGCModifier);
            if (UUGC_CameraAddOnModifier* UGCAddOnModifier = Cast<UUGC_CameraAddOnModifier>(AddedModifier))
            {
                UGCAddOnModifiersList.Add(UGCAddOnModifier);
            }
        }
    }
    return AddedModifier;
}

bool AUGC_PlayerCameraManager::RemoveCameraModifier(UCameraModifier* ModifierToRemove)
{
    if (ModifierToRemove)
    {
        if (UUGC_CameraModifier* UGCModifierToRemove = Cast<UUGC_CameraModifier>(ModifierToRemove))
        {
            // Loop through each modifier in camera
            for (int32 ModifierIdx = 0; ModifierIdx < UGCModifiersList.Num(); ++ModifierIdx)
            {
                // If we found ourselves, remove ourselves from the list and return
                if (UGCModifiersList[ModifierIdx] == UGCModifierToRemove)
                {
                    UGCModifiersList.RemoveAt(ModifierIdx, 1);
                    break;
                }
            }

            if (UUGC_CameraAddOnModifier* UGCAddOnModifier = Cast<UUGC_CameraAddOnModifier>(ModifierToRemove))
            {
                // Loop through each modifier in camera
                for (int32 ModifierIdx = 0; ModifierIdx < UGCAddOnModifiersList.Num(); ++ModifierIdx)
                {
                    // If we found ourselves, remove ourselves from the list and return
                    if (UGCAddOnModifiersList[ModifierIdx] == UGCModifierToRemove)
                    {
                        UGCAddOnModifiersList.RemoveAt(ModifierIdx, 1);
                        break;
                    }
                }
            }
        }
    }
    return Super::RemoveCameraModifier(ModifierToRemove);
}

FVector AUGC_PlayerCameraManager::GetOwnerVelocity() const
{
    FVector Velocity = FVector::ZeroVector;
    if (MovementComponent)
    {
        Velocity = MovementComponent->Velocity;
    }
    else if (OwnerPawn && OwnerPawn->GetClass()->ImplementsInterface(UUGC_PawnMovementInterface::StaticClass()))
    {
        Velocity = IUGC_PawnMovementInterface::Execute_GetOwnerVelocity(OwnerPawn);
    }
    return Velocity;
}

bool AUGC_PlayerCameraManager::IsOwnerFalling() const
{
    bool bIsFalling = false;
    if (MovementComponent)
    {
        bIsFalling = MovementComponent->IsFalling();
    }
    else if (OwnerPawn && OwnerPawn->GetClass()->ImplementsInterface(UUGC_PawnMovementInterface::StaticClass()))
    {
        bIsFalling = IUGC_PawnMovementInterface::Execute_IsOwnerFalling(OwnerPawn);
    }
    return bIsFalling;
}

bool AUGC_PlayerCameraManager::IsOwnerStrafing() const
{
    bool bIsStrafing = false;
    if (MovementComponent && OwnerPawn)
    {
        bIsStrafing = OwnerPawn->bUseControllerRotationYaw || (MovementComponent->bUseControllerDesiredRotation && !MovementComponent->bOrientRotationToMovement);
    }
    else if (OwnerPawn)
    {
        if (OwnerPawn->GetClass()->ImplementsInterface(UUGC_PawnMovementInterface::StaticClass()))
        {
            bIsStrafing = IUGC_PawnMovementInterface::Execute_IsOwnerStrafing(OwnerPawn);
        }
        else
        {
            bIsStrafing = OwnerPawn->bUseControllerRotationYaw;
        }
    }
    return bIsStrafing;
}

bool AUGC_PlayerCameraManager::IsOwnerMovingOnGround() const
{
    bool bIsMovingOnGround = false;
    if (MovementComponent)
    {
        bIsMovingOnGround = MovementComponent->IsMovingOnGround();
    }
    else if (OwnerPawn && OwnerPawn->GetClass()->ImplementsInterface(UUGC_PawnMovementInterface::StaticClass()))
    {
        bIsMovingOnGround = IUGC_PawnMovementInterface::Execute_IsOwnerMovingOnGround(OwnerPawn);
    }
    return bIsMovingOnGround;
}

void AUGC_PlayerCameraManager::ComputeOwnerFloorDist(float SweepDistance, float CapsuleRadius, bool& bOutFloorExists, float& OutFloorDistance) const
{
    if (MovementComponent && OwnerCharacter)
    {
        CapsuleRadius = FMath::Max(CapsuleRadius, OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius());
        FFindFloorResult OutFloorResult;
        MovementComponent->ComputeFloorDist(OwnerPawn->GetActorLocation(), SweepDistance, SweepDistance, OutFloorResult, CapsuleRadius);
        bOutFloorExists = OutFloorResult.bBlockingHit;
        OutFloorDistance = bOutFloorExists ? OutFloorResult.FloorDist : 0.f;
    }
    else
    {
        FHitResult OutHit;
        bOutFloorExists = GetWorld()->SweepSingleByChannel(OutHit
            , OwnerPawn->GetActorLocation()
            , OwnerPawn->GetActorLocation() - SweepDistance * FVector::UpVector
            , FQuat::Identity
            , ECollisionChannel::ECC_Visibility
            , FCollisionShape::MakeSphere(CapsuleRadius));

        OutFloorDistance = bOutFloorExists ? OutHit.Distance : 0.f;
    }
}

void AUGC_PlayerCameraManager::ComputeOwnerFloorNormal(float SweepDistance, float CapsuleRadius, bool& bOutFloorExists, FVector& OutFloorNormal) const
{
    if (MovementComponent && OwnerCharacter)
    {
        bOutFloorExists = MovementComponent->CurrentFloor.IsWalkableFloor();
        OutFloorNormal = MovementComponent->CurrentFloor.HitResult.ImpactNormal;
    }
    else
    {
        FHitResult OutHit;
        bOutFloorExists = GetWorld()->SweepSingleByChannel(OutHit
            , OwnerPawn->GetActorLocation()
            , OwnerPawn->GetActorLocation() - SweepDistance * FVector::UpVector
            , FQuat::Identity
            , ECollisionChannel::ECC_Visibility
            , FCollisionShape::MakeSphere(CapsuleRadius));

        bOutFloorExists = OutHit.bBlockingHit;
        OutFloorNormal = bOutFloorExists ? OutHit.ImpactNormal : FVector::ZeroVector;
    }
}

void AUGC_PlayerCameraManager::ComputeOwnerSlopeAngle(float& OutSlopePitchDegrees, float& OutSlopeRollDegrees)
{
    bool bOutFloorExists = false;
    FVector OutFloorNormal = FVector::ZeroVector;
    ComputeOwnerFloorNormal(96.f, 64.f, bOutFloorExists, OutFloorNormal);
    UKismetMathLibrary::GetSlopeDegreeAngles(OwnerPawn->GetActorRightVector(), OutFloorNormal, OwnerPawn->GetActorUpVector(), OutSlopePitchDegrees, OutSlopeRollDegrees);
}

float AUGC_PlayerCameraManager::ComputeOwnerLookAndMovementDot()
{
    if (IsOwnerStrafing())
    {
        return 1.f;
    }

    FVector const Velocity = GetOwnerVelocity();
    if (Velocity.IsNearlyZero())
    {
        return 0.f;
    }

    float const Dot = Velocity | OwnerPawn->GetControlRotation().Vector();
    return Dot;
}