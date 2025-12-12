// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "UGC_CameraModifier.generated.h"

/**
 * Base Camera Modifier Class
 */
UCLASS(abstract, ClassGroup = "UGC|Camera Modifiers")
class AURORADEVS_UGC_API UUGC_CameraModifier : public UCameraModifier
{
    GENERATED_BODY()

public:
    virtual void EnableModifier() override;
    virtual void DisableModifier(bool bImmediate) override;

    /**
     *  Function called once this modifier gets enabled.
     *  @param  LastPOV  - the last view POV of the camera.
    */
    UFUNCTION(BlueprintNativeEvent, Category = "UGC|Camera Modifier")
    void OnModifierEnabled(FMinimalViewInfo const& LastPOV);

    /**
     *  Function called once this modifier gets disabled.
     *  @param  bWasImmediate  - true if modifier was disabled without a blend out.
     *  @param  LastPOV  - the last view POV of the camera.
    */
    UFUNCTION(BlueprintNativeEvent, Category = "UGC|Camera Modifier")
    void OnModifierDisabled(FMinimalViewInfo const& LastPOV, bool bWasImmediate);

    /**
     * Called to give modifiers a chance to adjust view rotation updates before they are applied.
     *
     * Default just returns ViewRotation unchanged
     * @param ViewTarget - Current view target.
     * @param InLocalControlRotation - The difference between the actor rotation and the control rotation.
     * @param DeltaTime - Frame time in seconds.
     * @param InViewLocation - In. The view location of the camera.
     * @param InViewRotation - In. The view rotation of the camera.
     * @param InDeltaRot - In/out. How much the rotation changed this frame.
     * @param OutDeltaRot - Out. How much the control rotation should change this frame.
     * @return Return true to prevent subsequent (lower priority) modifiers to further adjust rotation, false otherwise.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "UGC|Camera Modifier")
    bool ProcessControlRotation(AActor* ViewTarget, float DeltaTime, FVector InViewLocation, FRotator InViewRotation, FRotator InLocalControlRotation, FRotator InDeltaRot, FRotator& OutDeltaRot);

    /**
     * Called to give modifiers a chance to adjust arm length and FOV before they are applied.
     *
     * @param DeltaTime - Frame time in seconds.
     * @param InFOV - The Current FOV of the camera.
     * @param InArmLength - The Current Arm Length of the camera.
     * @param ViewLocation - The view location of the camera.
     * @param ViewRotation - The view rotation of the camera.
     * @param OutFOV - The New FOV of the camera.
     * @param OutArmLength - The New Arm Length of the camera.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UGC|Camera Modifier")
    void ProcessBoomLengthAndFOV(float DeltaTime, float InFOV, float InArmLength, FVector ViewLocation, FRotator ViewRotation, float& OutFOV, float& OutArmLength);

    /**
     * Called to give modifiers a chance to adjust arm offsets before they are applied.
     *
     * @param DeltaTime - Frame time in seconds.
     * @param InSocketOffset - The Current Socket Offset of the camera.
     * @param InTargetOffset - The Current Target Offset of the camera.
     * @param ViewLocation - The view location of the camera.
     * @param ViewRotation - The view rotation of the camera.
     * @param OutSocketOffset - New Socket Offset of the camera.
     * @param OutTargetOffset - New Target Offset of the camera.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UGC|Camera Modifier")
    void ProcessBoomOffsets(float DeltaTime, FVector InSocketOffset, FVector InTargetOffset, FVector ViewLocation, FRotator ViewRotation, FVector& OutSocketOffset, FVector& OutTargetOffset);

    UFUNCTION(BlueprintNativeEvent, Category = "UGC|Camera Modifier")
    void OnAnyLevelSequenceStarted();

    UFUNCTION(BlueprintNativeEvent, Category = "UGC|Camera Modifier")
    void OnAnyLevelSequenceEnded();

    UFUNCTION(BlueprintNativeEvent, Category = "UGC|Camera Modifier")
    void OnSetViewTarget(bool bImmediate, bool bNewTargetIsOwner);

    UFUNCTION(BlueprintPure, Category = "UGC|Camera Modifier")
    bool IsDebugEnabled() const;

    UFUNCTION(BlueprintCallable, Category = "UGC|Camera Modifier")
    void ToggleDebug(bool const bEnabled);

    UFUNCTION(BlueprintCallable, Category = "UGC|Camera Modifier")
    void SetAlpha(float InAlpha) { Alpha = InAlpha; }

    /**
     * Called to give modifiers a chance to adjust both the yaw turn rate and pitch turn rate. However the input for looking needs to have UGC_CameraTurnRateModifier.
     *
     * @param DeltaTime - Frame time in seconds.
     * @param InLocalControlRotation - The difference between the actor rotation and the control rotation.
     * @param OutPitchTurnRate - Out. New value of the pitch turn rate (between 0 and 1).
     * @param OutYawTurnRate - Out. New value of the yaw turn rate (between 0 and 1).
     * @return Return true to prevent subsequent (lower priority) modifiers to further adjust rotation, false otherwise.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "UGC|Camera Modifier")
    bool ProcessTurnRate(float DeltaTime, FRotator InLocalControlRotation, float InPitchTurnRate, float InYawTurnRate, float& OutPitchTurnRate, float& OutYawTurnRate);

    bool CanPlayDuringCameraAnimation() const { return bPlayDuringCameraAnimations; }

    UFUNCTION(BlueprintPure, Category = "UGC|Camera Modifier|Movement")
    FVector GetOwnerVelocity() const;

    UFUNCTION(BlueprintPure, Category = "UGC|Camera Modifier|Movement")
    bool IsOwnerFalling() const;

    UFUNCTION(BlueprintPure, Category = "UGC|Camera Modifier|Movement")
    bool IsOwnerStrafing() const;

    UFUNCTION(BlueprintPure, Category = "UGC|Camera Modifier|Movement")
    bool IsOwnerMovingOnGround() const;

    UFUNCTION(BlueprintPure, Category = "UGC|Camera Modifier|Movement")
    void ComputeOwnerFloorDistance(float SweepDistance, float CapsuleRadius, bool& bOutFloorExists, float& OutFloorDistance) const;

    UFUNCTION(BlueprintPure, Category = "UGC|Camera Modifier|Movement")
    void ComputeOwnerFloorNormal(float SweepDistance, float CapsuleRadius, bool& bOutFloorExists, FVector& OutFloorNormal) const;

    UFUNCTION(BlueprintPure, Category = "UGC|Camera Modifier|Movement")
    void ComputeOwnerSlopeAngle(float& OutSlopePitchDegrees, float& OutSlopeRollDegrees);

    /*
     * Returns value betwen 1 (the character is looking where they're moving) or -1 (looking in the opposite direction they're moving).
     * Will return 0 if the character isn't moving.
     */
    UFUNCTION(BlueprintPure, Category = "UGC|Camera Modifier|Movement")
    float ComputeOwnerLookAndMovementDot();

protected:
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;
    virtual void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& OutViewLocation, FRotator& OutViewRotation, float& OutFOV) override;
    virtual bool ProcessViewRotation(AActor* ViewTarget, float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot) override;

    template<typename T>
    T* GetViewTargetAs() const { return Cast<T>(GetViewTarget()); }

    void UpdateOwnerReferences();

    void UpdateInternalVariables(float DeltaTime);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier Settings")
    bool bPlayDuringCameraAnimations = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    TObjectPtr<class AUGC_PlayerCameraManager> UGCCameraManager = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    TObjectPtr<class APlayerController> OwnerController = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    TObjectPtr<class ACharacter> OwnerCharacter = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    TObjectPtr<class APawn> OwnerPawn = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    TObjectPtr<class USpringArmComponent> SpringArm = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    TObjectPtr<class UCharacterMovementComponent> MovementComponent = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    FVector CurrentSocketOffset;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    FVector CurrentTargetOffset;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    float CurrentArmLength;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    bool bHasMovementInput;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    FVector PreviousMovementInput;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    FVector MovementInput;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    float TimeSinceMovementInput;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    bool bHasRotationInput;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    FRotator RotationInput;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UGC|Camera Modifier|Internal")
    float TimeSinceRotationInput;
};

/**
 * Base Camera Modifier Class for Add-on modifiers
 */
UCLASS(abstract, ClassGroup = "UGC|Add On|Camera Modifiers")
class AURORADEVS_UGC_API UUGC_CameraAddOnModifier : public UUGC_CameraModifier
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UGC|Add On|Camera Modifiers")
    void SetSettings(class UUGC_CameraAddOnModifierSettings* InSettings);

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Settings")
    TObjectPtr<UUGC_CameraAddOnModifierSettings> Settings;
};