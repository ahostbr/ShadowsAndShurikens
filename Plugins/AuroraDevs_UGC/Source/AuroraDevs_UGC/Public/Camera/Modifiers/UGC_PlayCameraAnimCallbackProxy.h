// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"
#include "UGC_CameraAnimationModifier.h"
#include "UGC_PlayCameraAnimCallbackProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCameraAnimationPlayDelegate);

/** Parameter struct for adding new camera animations to UGCCameraAnimationCameraModifier */
USTRUCT(BlueprintType)
struct FUGCCameraAnimationParams
{
    GENERATED_BODY()

    /** Time scale for playing the new camera animation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Animation")
    float PlayRate = 1.f;

    /** Ease-in function type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Animation")
    ECameraAnimationEasingType EaseInType = ECameraAnimationEasingType::Linear;
    /** Ease-in duration in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Animation")
    float EaseInDuration = 0.f;

    /** Ease-out function type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Animation")
    ECameraAnimationEasingType EaseOutType = ECameraAnimationEasingType::Linear;
    /** Ease-out duration in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Animation")
    float EaseOutDuration = 0.f;

    /** How should the camera behave after the animation is over. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Animation")
    ECameraAnimationResetType ResetType = ECameraAnimationResetType::ResetToZero;

    explicit operator FCameraAnimationParams() const;
};

UCLASS()
class AURORADEVS_UGC_API UUGC_PlayCameraAnimCallbackProxy : public UObject
{
    GENERATED_UCLASS_BODY()

    // Called when Camera Animation finished playing and wasn't interrupted
    UPROPERTY(BlueprintAssignable)
    FOnCameraAnimationPlayDelegate OnCompleted;

    // Called when Camera Animation starts blending out and is not interrupted
    UPROPERTY(BlueprintAssignable)
    FOnCameraAnimationPlayDelegate OnEaseOut;

    // Called when Camera Animation has been interrupted (or failed to play)
    UPROPERTY(BlueprintAssignable)
    FOnCameraAnimationPlayDelegate OnInterrupted;

    // Called to perform the query internally
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
        static UUGC_PlayCameraAnimCallbackProxy* CreateProxyObjectForPlayCameraAnim(
            class APlayerCameraManager* InPlayerCameraManager,
            TSubclassOf<class UUGC_CameraAnimationModifier> ModifierClass,
            class UCameraAnimationSequence* CameraSequence,
            FUGCCameraAnimationParams Params,
            struct FCameraAnimationHandle& Handle);

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
        static UUGC_PlayCameraAnimCallbackProxy* CreateProxyObjectForPlayCameraAnimForModifier(
            class UUGC_CameraAnimationModifier* CameraAnimationModifier,
            class UCameraAnimationSequence* CameraSequence,
            FUGCCameraAnimationParams Params,
            struct FCameraAnimationHandle& Handle);

protected:
    UFUNCTION()
        void OnCameraAnimationEasingOut(UCameraAnimationSequence* CameraAnimation);

    UFUNCTION()
        void OnCameraAnimationEnded(UCameraAnimationSequence* CameraAnimation, bool bInterrupted);

private:
    TWeakObjectPtr<class UUGC_CameraAnimationModifier> CameraAnimationModifierPtr;

    bool bInterruptedCalledBeforeBlendingOut = false;

    FOnCameraAnimationEaseOutStarted CameraAnimationEasingOutDelegate;
    FOnCameraAnimationEnded CameraAnimationEndedDelegate;

    void PlayCameraAnimation(
        class APlayerCameraManager* InPlayerCameraManager,
        TSubclassOf<class UUGC_CameraAnimationModifier> ModifierClass,
        class UCameraAnimationSequence* CameraSequence,
        FUGCCameraAnimationParams Params,
        struct FCameraAnimationHandle& Handle);

    void PlayCameraAnimation(
        class UUGC_CameraAnimationModifier* CameraAnimationModifier,
        class UCameraAnimationSequence* CameraSequence,
        FUGCCameraAnimationParams Params,
        struct FCameraAnimationHandle& Handle);
};