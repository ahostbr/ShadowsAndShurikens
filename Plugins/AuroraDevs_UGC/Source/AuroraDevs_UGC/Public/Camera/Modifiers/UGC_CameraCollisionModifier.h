// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "UGC_CameraModifier.h"
#include "Camera/Data/UGC_CameraData.h"
#include "Camera/CameraTypes.h"
#include "DrawDebugHelpers.h"
#include "UGC_CameraCollisionModifier.generated.h"

/**
 * Camera Modifier which does camera avoidance using predictive collision feelers.
 */
UCLASS(abstract, ClassGroup = "UGC Camera Modifiers")
class AURORADEVS_UGC_API UUGC_CameraCollisionModifier : public UUGC_CameraModifier
{
    GENERATED_BODY()

    UUGC_CameraCollisionModifier();

protected:
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

    void UpdatePreventPenetration(float DeltaTime, FMinimalViewInfo& InOutPOV);

    void PreventCameraPenetration(class AActor const& ViewTarget, FVector const& SafeLoc, FVector& OutCameraLoc, float const& DeltaTime, float& OutDistBlockedPct, bool bSingleRayOnly);

    // virtual void ProcessBoomLengthAndFOV_Implementation(float DeltaTime, float InFOV, float InArmLength, FVector ViewLocation, FRotator ViewRotation, float& OutFOV, float& OutArmLength) override;
    // void PreventCameraPenetration2(class AActor const& ViewTarget, FVector const& SafeLoc, FVector const& CameraLoc, float& OutArmLength, float const& DeltaTime, float& OutDistBlockedPct, bool bSingleRayOnly);

    FVector GetTraceSafeLocation(FMinimalViewInfo const& POV);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC|Modifiers|Collision")
    FCameraCollisionSettings CollisionSettings;

    // If you don't want the camera to start close to the character and smoothly pan out once your character is spawned, default-initialize this variable to 1.f.
    UPROPERTY(Transient)
    float AimLineToDesiredPosBlockedPct = 1.f;

    UPROPERTY(Transient)
    TArray<TObjectPtr<const AActor>> DebugActorsHitDuringCameraPenetration;

protected:
#if ENABLE_DRAW_DEBUG
    mutable float LastDrawDebugTime = -MAX_FLT;
#endif
};