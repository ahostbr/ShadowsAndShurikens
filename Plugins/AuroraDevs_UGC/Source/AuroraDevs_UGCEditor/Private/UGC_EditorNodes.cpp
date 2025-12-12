// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#include "UGC_EditorNodes.h"

#include "AuroraDevs_UGC/Public/Camera/Modifiers/UGC_PlayCameraAnimCallbackProxy.h"

UUGC_PlayCameraAnimation::UUGC_PlayCameraAnimation(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UUGC_PlayCameraAnimCallbackProxy, CreateProxyObjectForPlayCameraAnim);
    ProxyFactoryClass = UUGC_PlayCameraAnimCallbackProxy::StaticClass();
    ProxyClass = UUGC_PlayCameraAnimCallbackProxy::StaticClass();
}

FText UUGC_PlayCameraAnimation::GetTooltipText() const
{
    return FText::AsCultureInvariant("Plays a single new camera animation sequence. Any subsequent calls while this animation runs will interrupt the current animation. This will try to look for the UGC Animation Camera Modifier through the passed in class (exact match only).");
}

FText UUGC_PlayCameraAnimation::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::AsCultureInvariant("Play Camera Animation From Modifier Class");
}

FText UUGC_PlayCameraAnimation::GetMenuCategory() const
{
    return FText::AsCultureInvariant("UGC Camera Animations");
}

UUGC_PlayCameraAnimationWithModifier::UUGC_PlayCameraAnimationWithModifier(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UUGC_PlayCameraAnimCallbackProxy, CreateProxyObjectForPlayCameraAnimForModifier);
    ProxyFactoryClass = UUGC_PlayCameraAnimCallbackProxy::StaticClass();
    ProxyClass = UUGC_PlayCameraAnimCallbackProxy::StaticClass();
}

FText UUGC_PlayCameraAnimationWithModifier::GetTooltipText() const
{
    return FText::AsCultureInvariant("Plays a single new camera animation sequence. Any subsequent calls while this animation runs will interrupt the current animation.");
}

FText UUGC_PlayCameraAnimationWithModifier::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::AsCultureInvariant("Play Camera Animation");
}

FText UUGC_PlayCameraAnimationWithModifier::GetMenuCategory() const
{
    return FText::AsCultureInvariant("UGC Camera Animations");
}