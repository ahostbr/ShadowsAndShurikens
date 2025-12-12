// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#include "Camera/Modifiers/UGC_PlayCameraAnimCallbackProxy.h"
#include "Camera/PlayerCameraManager.h"

namespace PlayCameraAnimCallbackProxyHelper
{
    FCameraAnimationHandle const UGCInvalid(MAX_int16, 0);
}

UUGC_PlayCameraAnimCallbackProxy::UUGC_PlayCameraAnimCallbackProxy(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bInterruptedCalledBeforeBlendingOut(false)
{
}

FUGCCameraAnimationParams::operator FCameraAnimationParams() const
{
    FCameraAnimationParams Params;
    Params.PlayRate = PlayRate;
    Params.EaseInDuration = EaseInDuration;
    Params.EaseOutDuration = EaseOutDuration;
    Params.EaseInType = EaseInType;
    Params.EaseOutType = EaseOutType;
    Params.bLoop = false;

    return Params;
}

UUGC_PlayCameraAnimCallbackProxy* UUGC_PlayCameraAnimCallbackProxy::CreateProxyObjectForPlayCameraAnim(APlayerCameraManager* InPlayerCameraManager, TSubclassOf<UUGC_CameraAnimationModifier> ModifierClass, UCameraAnimationSequence* CameraSequence, FUGCCameraAnimationParams Params, FCameraAnimationHandle& Handle)
{
    UUGC_PlayCameraAnimCallbackProxy* Proxy = NewObject<UUGC_PlayCameraAnimCallbackProxy>();
    Proxy->SetFlags(RF_StrongRefOnFrame);
    Proxy->PlayCameraAnimation(InPlayerCameraManager, ModifierClass, CameraSequence, Params, Handle);
    return Proxy;
}

UUGC_PlayCameraAnimCallbackProxy* UUGC_PlayCameraAnimCallbackProxy::CreateProxyObjectForPlayCameraAnimForModifier(UUGC_CameraAnimationModifier* CameraAnimationModifier, UCameraAnimationSequence* CameraSequence, FUGCCameraAnimationParams Params, FCameraAnimationHandle& Handle)
{
    UUGC_PlayCameraAnimCallbackProxy* Proxy = NewObject<UUGC_PlayCameraAnimCallbackProxy>();
    Proxy->SetFlags(RF_StrongRefOnFrame);
    Proxy->PlayCameraAnimation(CameraAnimationModifier, CameraSequence, Params, Handle);
    return Proxy;
}

void UUGC_PlayCameraAnimCallbackProxy::PlayCameraAnimation(UUGC_CameraAnimationModifier* CameraAnimModifier, UCameraAnimationSequence* CameraSequence, FUGCCameraAnimationParams Params, FCameraAnimationHandle& Handle)
{
    Handle = PlayCameraAnimCallbackProxyHelper::UGCInvalid;
    bool bPlayedSuccessfully = false;

    if (CameraAnimModifier)
    {
        Handle = CameraAnimModifier->PlaySingleCameraAnimation(CameraSequence, static_cast<FCameraAnimationParams>(Params), Params.ResetType);
        bPlayedSuccessfully = Handle.IsValid();

        if (bPlayedSuccessfully)
        {
            CameraAnimationModifierPtr = CameraAnimModifier;

            CameraAnimationEasingOutDelegate.BindUObject(this, &UUGC_PlayCameraAnimCallbackProxy::OnCameraAnimationEasingOut);
            CameraAnimationModifierPtr->CameraAnimation_SetEasingOutDelegate(CameraAnimationEasingOutDelegate, Handle);

            CameraAnimationEndedDelegate.BindUObject(this, &UUGC_PlayCameraAnimCallbackProxy::OnCameraAnimationEnded);
            CameraAnimationModifierPtr->CameraAnimation_SetEndedDelegate(CameraAnimationEndedDelegate, Handle);
        }
    }

    if (!bPlayedSuccessfully)
    {
        OnInterrupted.Broadcast();
    }
}

void UUGC_PlayCameraAnimCallbackProxy::PlayCameraAnimation(APlayerCameraManager* InPlayerCameraManager, TSubclassOf<UUGC_CameraAnimationModifier> ModifierClass, UCameraAnimationSequence* CameraSequence, FUGCCameraAnimationParams Params, FCameraAnimationHandle& Handle)
{
    if (InPlayerCameraManager)
    {
        if (UUGC_CameraAnimationModifier* CameraAnimModifier = Cast<UUGC_CameraAnimationModifier>(InPlayerCameraManager->FindCameraModifierByClass(ModifierClass)))
        {
            PlayCameraAnimation(CameraAnimModifier, CameraSequence, Params, Handle);
        }
    }
}

void UUGC_PlayCameraAnimCallbackProxy::OnCameraAnimationEasingOut(UCameraAnimationSequence* CameraAnimation)
{
    OnEaseOut.Broadcast();
}

void UUGC_PlayCameraAnimCallbackProxy::OnCameraAnimationEnded(UCameraAnimationSequence* CameraAnimation, bool bInterrupted)
{
    if (!bInterrupted)
    {
        OnCompleted.Broadcast();
    }
    else if (!bInterruptedCalledBeforeBlendingOut)
    {
        OnInterrupted.Broadcast();
    }
}