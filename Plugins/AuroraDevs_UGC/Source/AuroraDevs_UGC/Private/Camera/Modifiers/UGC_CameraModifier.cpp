// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#include "Camera/Modifiers/UGC_CameraModifier.h"
#include "Camera/Modifiers/UGC_CameraAnimationModifier.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/UGC_PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

void UUGC_CameraModifier::EnableModifier()
{
    Super::EnableModifier();
    OnModifierEnabled(CameraOwner->GetCameraCacheView());
}

void UUGC_CameraModifier::DisableModifier(bool bImmediate)
{
    Super::DisableModifier(bImmediate);

    if (bImmediate && bDisabled && !bPendingDisable)
    {
        Alpha = 0.f;
    }
    OnModifierDisabled(CameraOwner->GetCameraCacheView(), bImmediate);
}

void UUGC_CameraModifier::OnModifierEnabled_Implementation(FMinimalViewInfo const& LastPOV)
{
}

void UUGC_CameraModifier::OnModifierDisabled_Implementation(FMinimalViewInfo const& LastPOV, bool bImmediate)
{
}

void UUGC_CameraModifier::ProcessBoomLengthAndFOV_Implementation(float DeltaTime, float InFOV, float InArmLength, FVector ViewLocation, FRotator ViewRotation, float& OutFOV, float& OutArmLength)
{
    OutFOV = InFOV;
    OutArmLength = InArmLength;
}

void UUGC_CameraModifier::ProcessBoomOffsets_Implementation(float DeltaTime, FVector InSocketOffset, FVector InTargetOffset, FVector ViewLocation, FRotator ViewRotation, FVector& OutSocketOffset, FVector& OutTargetOffset)
{
    OutSocketOffset = InSocketOffset;
    OutTargetOffset = InTargetOffset;
}

void UUGC_CameraModifier::OnAnyLevelSequenceStarted_Implementation()
{
}

void UUGC_CameraModifier::OnAnyLevelSequenceEnded_Implementation()
{
}

void UUGC_CameraModifier::OnSetViewTarget_Implementation(bool bImmediate, bool bNewTargetIsOwner)
{
}

bool UUGC_CameraModifier::IsDebugEnabled() const
{
    return bDebug;
}

void UUGC_CameraModifier::ToggleDebug(bool const bEnabled)
{
    bDebug = bEnabled;
}

bool UUGC_CameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    return Super::ModifyCamera(DeltaTime, InOutPOV);
}

void UUGC_CameraModifier::ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& OutViewLocation, FRotator& OutViewRotation, float& OutFOV)
{
    Super::ModifyCamera(DeltaTime, ViewLocation, ViewRotation, FOV, OutViewLocation, OutViewRotation, OutFOV);

    OutViewLocation = ViewLocation;
    OutViewRotation = ViewRotation;
    OutFOV = FOV;

    UpdateOwnerReferences();

    if (!UGCCameraManager)
    {
        return;
    }

    if (!bPlayDuringCameraAnimations)
    {
        if (UGCCameraManager->IsPlayingAnyCameraAnimation())
        {
            return;
        }
    }

    if (OwnerController && OwnerPawn && OwnerPawn->IsLocallyControlled())
    {
        UpdateInternalVariables(DeltaTime);
        if (SpringArm)
        {
            ProcessBoomLengthAndFOV(DeltaTime, FOV, CurrentArmLength, ViewLocation, ViewRotation, OutFOV, SpringArm->TargetArmLength);
            ProcessBoomOffsets(DeltaTime, CurrentSocketOffset, CurrentTargetOffset, ViewLocation, ViewRotation, SpringArm->SocketOffset, SpringArm->TargetOffset);
        }
    }
}

bool UUGC_CameraModifier::ProcessViewRotation(AActor* ViewTarget, float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot)
{
    bool bResult = Super::ProcessViewRotation(ViewTarget, DeltaTime, OutViewRotation, OutDeltaRot);

    if (!bPlayDuringCameraAnimations)
    {
        if (UGCCameraManager && UGCCameraManager->IsPlayingAnyCameraAnimation())
        {
            return bResult;
        }
    }

    if (ViewTarget && CameraOwner && CameraOwner->GetOwningPlayerController())
    {
        FVector const CameraLocation = CameraOwner->GetCameraLocation();
        FRotator const ControlRotation = CameraOwner->GetOwningPlayerController()->GetControlRotation();
        FRotator const OwnerRotation = ViewTarget ? ViewTarget->GetActorRotation() : FRotator::ZeroRotator;
        FRotator InLocalControlRotation = ControlRotation - OwnerRotation;
        InLocalControlRotation.Normalize();
        bResult = ProcessControlRotation(ViewTarget, DeltaTime, CameraLocation, OutViewRotation, InLocalControlRotation, OutDeltaRot, OutDeltaRot);
    }

    return bResult;
}

void UUGC_CameraModifier::UpdateOwnerReferences()
{
    UGCCameraManager = Cast<AUGC_PlayerCameraManager>(CameraOwner);
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    if (!UGCCameraManager)
    {
        return;
    }

    OwnerController = UGCCameraManager->GetOwningPlayerController();
    OwnerCharacter = UGCCameraManager->OwnerCharacter;
    OwnerPawn = UGCCameraManager->OwnerPawn;
    if (OwnerPawn && OwnerPawn->IsLocallyControlled())
    {
        SpringArm = UGCCameraManager->CameraArm;
        MovementComponent = UGCCameraManager->MovementComponent;
    }
}

void UUGC_CameraModifier::UpdateInternalVariables(float DeltaTime)
{
    if (!UGCCameraManager)
    {
        return;
    }

    bHasMovementInput = UGCCameraManager->bHasMovementInput;
    PreviousMovementInput = MovementInput;
    MovementInput = UGCCameraManager->MovementInput;
    TimeSinceMovementInput = UGCCameraManager->TimeSinceMovementInput;
    bHasRotationInput = UGCCameraManager->bHasRotationInput;
    RotationInput = UGCCameraManager->RotationInput;
    TimeSinceRotationInput = UGCCameraManager->TimeSinceRotationInput;

    if (SpringArm)
    {
        CurrentSocketOffset = SpringArm->SocketOffset;
        CurrentTargetOffset = SpringArm->TargetOffset;
        CurrentArmLength = SpringArm->TargetArmLength;
    }
}

bool UUGC_CameraModifier::ProcessTurnRate_Implementation(float DeltaTime, FRotator InLocalControlRotation, float InPitchTurnRate, float InYawTurnRate, float& OutPitchTurnRate, float& OutYawTurnRate)
{
    return false;
}

bool UUGC_CameraModifier::ProcessControlRotation_Implementation(AActor* ViewTarget, float DeltaTime, FVector InViewLocation, FRotator InViewRotation, FRotator InLocalControlRotation, FRotator InDeltaRot, FRotator& OutDeltaRot)
{
    return false;
}

FVector UUGC_CameraModifier::GetOwnerVelocity() const
{
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    return UGCCameraManager->GetOwnerVelocity();
}

bool UUGC_CameraModifier::IsOwnerFalling() const
{
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    return UGCCameraManager->IsOwnerFalling();
}

bool UUGC_CameraModifier::IsOwnerStrafing() const
{
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    return UGCCameraManager->IsOwnerStrafing();
}

bool UUGC_CameraModifier::IsOwnerMovingOnGround() const
{
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    return UGCCameraManager->IsOwnerMovingOnGround();
}

void UUGC_CameraModifier::ComputeOwnerFloorDistance(float SweepDistance, float CapsuleRadius, bool& bOutFloorExists, float& OutFloorDistance) const
{
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    return UGCCameraManager->ComputeOwnerFloorDist(SweepDistance, CapsuleRadius, bOutFloorExists, OutFloorDistance);
}

void UUGC_CameraModifier::ComputeOwnerFloorNormal(float SweepDistance, float CapsuleRadius, bool& bOutFloorExists,FVector& OutFloorNormal) const
{
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    return UGCCameraManager->ComputeOwnerFloorNormal(SweepDistance, CapsuleRadius, bOutFloorExists, OutFloorNormal);
}

void UUGC_CameraModifier::ComputeOwnerSlopeAngle(float& OutSlopePitchDegrees, float& OutSlopeRollDegrees)
{
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    return UGCCameraManager->ComputeOwnerSlopeAngle(OutSlopePitchDegrees, OutSlopeRollDegrees);
}

float UUGC_CameraModifier::ComputeOwnerLookAndMovementDot()
{
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    return UGCCameraManager->ComputeOwnerLookAndMovementDot();
}

void UUGC_CameraAddOnModifier::SetSettings_Implementation(class UUGC_CameraAddOnModifierSettings* InSettings)
{
    Settings = InSettings;
}