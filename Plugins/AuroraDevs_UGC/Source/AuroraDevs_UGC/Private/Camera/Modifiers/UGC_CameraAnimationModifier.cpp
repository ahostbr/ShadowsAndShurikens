// Copyright(c) Aurora Devs 2024. All Rights Reserved.

#include "Camera/Modifiers/UGC_CameraAnimationModifier.h"
#include "Camera/CameraAnimationHelper.h"
#include "Camera/PlayerCameraManager.h"
#include "CameraAnimationSequence.h"
#include "CameraAnimationSequencePlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "DisplayDebugHelpers.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EntitySystem/MovieSceneEntitySystemLinker.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MovieSceneFwd.h"
#include "Camera/UGC_PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProfilingDebugging/CountersTrace.h"
#include "UObject/UObjectGlobals.h"
#include "Runtime/Launch/Resources/Version.h"

namespace UGCCameraAnimationHelper
{
    FCameraAnimationHandle const UGCInvalid(MAX_int16, 0);
}

FCameraAnimationHandle UUGC_CameraAnimationModifier::PlaySingleCameraAnimation(UCameraAnimationSequence* Sequence, FCameraAnimationParams Params, ECameraAnimationResetType ResetType)
{
    if (!ensure(Sequence))
    {
        return UGCCameraAnimationHelper::UGCInvalid;
    }

    UCameraAnimationSequence* InterruptedSequence = nullptr;
    if (ActiveAnimations.Num() == 1)
    {
        if (ActiveAnimations[0].IsValid() && ActiveAnimations[0].Player != nullptr && ActiveAnimations[0].Player->GetPlaybackStatus() == EMovieScenePlayerStatus::Playing)
        {
            InterruptedSequence = ActiveAnimations[0].Sequence;
        }
    }
    else if (ActiveAnimations.IsEmpty())
    {
        ActiveAnimations.Emplace();
    }

    // Always play one animation only
    int32 const NewIndex = 0;
    check(NewIndex < MAX_uint16);

    const uint16 InstanceSerial = NextInstanceSerialNumber++;
    FCameraAnimationHandle InstanceHandle{ (uint16) NewIndex, InstanceSerial };

    FActiveCameraAnimationInfo& NewCameraAnimation = ActiveAnimations[NewIndex];
    CurrentResetType = ResetType;
    NewCameraAnimation.Sequence = Sequence;
    NewCameraAnimation.Params = Params;
    NewCameraAnimation.Handle = InstanceHandle;

    const FName PlayerName = MakeUniqueObjectName(this, UCameraAnimationSequencePlayer::StaticClass(), TEXT("CameraAnimationPlayer"));
    NewCameraAnimation.Player = NewObject<UCameraAnimationSequencePlayer>(this, PlayerName);
    const FName CameraStandInName = MakeUniqueObjectName(this, UCameraAnimationSequenceCameraStandIn::StaticClass(), TEXT("CameraStandIn"));
    NewCameraAnimation.CameraStandIn = NewObject<UCameraAnimationSequenceCameraStandIn>(this, CameraStandInName);

    // Start easing in immediately if there's any defined.
    NewCameraAnimation.bIsEasingIn = (Params.EaseInDuration > 0.f);
    NewCameraAnimation.EaseInCurrentTime = 0.f;
    NewCameraAnimation.bIsEasingOut = false;
    NewCameraAnimation.EaseOutCurrentTime = 0.f;

    // Initialize our stand-in object.
    NewCameraAnimation.CameraStandIn->Initialize(Sequence);

    // Make the player always use our stand-in object whenever a sequence wants to spawn or possess an object.
    NewCameraAnimation.Player->SetBoundObjectOverride(NewCameraAnimation.CameraStandIn);

    // Initialize it and start playing.
    NewCameraAnimation.Player->Initialize(Sequence);
    NewCameraAnimation.Player->Play(Params.bLoop, Params.bRandomStartTime);

    if (InterruptedSequence)
    {
        static bool constexpr bInterrupt = true;
        OnAnimationEnded.ExecuteIfBound(InterruptedSequence, bInterrupt);
    }

    return InstanceHandle;
}

void UUGC_CameraAnimationModifier::StopCameraAnimationSequence(UCameraAnimationSequence* Sequence, bool bImmediate)
{
    if (!ActiveAnimations.IsEmpty() && ActiveAnimations[0].IsValid() && (Sequence == nullptr || ActiveAnimations[0].Sequence == Sequence))
    {
        if (UCameraAnimationSequence* InterruptedSequence = ActiveAnimations[0].Sequence)
        {
            StopCameraAnimation(ActiveAnimations[0].Handle, bImmediate);
            static bool constexpr bInterrupt = true;
            OnAnimationEnded.ExecuteIfBound(InterruptedSequence, bInterrupt);
        }
    }
}

UCameraAnimationSequence* UUGC_CameraAnimationModifier::GetCurrentCameraAnimation() const
{
    return !ActiveAnimations.IsEmpty() && ActiveAnimations[0].IsValid() ? ActiveAnimations[0].Sequence : nullptr;
}

bool UUGC_CameraAnimationModifier::IsCameraAnimationSequenceActive(UCameraAnimationSequence* Sequence) const
{
    return !ActiveAnimations.IsEmpty() && ActiveAnimations[0].IsValid() && ActiveAnimations[0].Sequence == Sequence;
}

bool UUGC_CameraAnimationModifier::IsAnyCameraAnimationSequence() const
{
    return !ActiveAnimations.IsEmpty() && ActiveAnimations[0].IsValid() && ActiveAnimations[0].Player && ActiveAnimations[0].Player->GetPlaybackStatus() == EMovieScenePlayerStatus::Playing;
}

bool UUGC_CameraAnimationModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    UCameraModifier::ModifyCamera(DeltaTime, InOutPOV);
    UGCTickActiveAnimation(DeltaTime, InOutPOV);
    return false;
}

void UUGC_CameraAnimationModifier::CameraAnimation_SetEasingOutDelegate(FOnCameraAnimationEaseOutStarted& InOnAnimationEaseOutStarted, FCameraAnimationHandle AnimationHandle)
{
    if (AnimationHandle.IsValid() && IsCameraAnimationActive(AnimationHandle))
    {
        OnAnimationEaseOutStarted = InOnAnimationEaseOutStarted;
    }
}

void UUGC_CameraAnimationModifier::CameraAnimation_SetEndedDelegate(FOnCameraAnimationEnded& InOnAnimationEnded, FCameraAnimationHandle AnimationHandle)
{
    if (AnimationHandle.IsValid() && IsCameraAnimationActive(AnimationHandle))
    {
        OnAnimationEnded = InOnAnimationEnded;
    }
}

void UUGC_CameraAnimationModifier::UGCDeactivateCameraAnimation(FActiveCameraAnimationInfo& ActiveAnimation)
{
    for (auto& a : ActiveAnimations)
    {
        if (a.Handle == ActiveAnimation.Handle)
        {
            if (a.Player && !ensure(a.Player->GetPlaybackStatus() == EMovieScenePlayerStatus::Stopped))
            {
                a.Player->Stop();
            }

            a = FActiveCameraAnimationInfo();
        }
    }
}

void UUGC_CameraAnimationModifier::UGCTickActiveAnimation(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    UGCCameraManager = Cast<AUGC_PlayerCameraManager>(CameraOwner);
    ensureMsgf(UGCCameraManager, TEXT("Please use UGC Camera Modifiers only with a player camera manager inheriting from UGC_PlayerCameraManager."));
    if (!UGCCameraManager)
    {
        return;
    }

    ensureMsgf(ActiveAnimations.Num() <= 1, TEXT("UUGC_CameraAnimationModifier: Trying to play multiple camera animations at the same time. Please use PlaySingleCameraAnimation!"));
    if (ActiveAnimations.Num() >= 1)
    {
        FActiveCameraAnimationInfo& ActiveAnimation = ActiveAnimations[0];
        if (ActiveAnimation.IsValid())
        {
#if ENABLE_DRAW_DEBUG
            UGCDebugAnimation(ActiveAnimation, DeltaTime);
#endif
            // float const Dilation = UGameplayStatics::GetGlobalTimeDilation(GetWorld());
            // float const UndilatedDeltaTime = FMath::IsNearlyZero(Dilation) ? 0.f : DeltaTime / Dilation;
            UGCTickAnimation(ActiveAnimation, DeltaTime, InOutPOV);

            if (ActiveAnimation.Player->GetPlaybackStatus() == EMovieScenePlayerStatus::Stopped)
            {
                // Here animation has just finished (ease out has completed as well)
                UGCDeactivateCameraAnimation(ActiveAnimation);
                static bool constexpr bInterrupt = false;
                OnAnimationEnded.ExecuteIfBound(ActiveAnimation.Sequence, bInterrupt);
            }
        }
    }
}

void UUGC_CameraAnimationModifier::UGCTickAnimation(FActiveCameraAnimationInfo& CameraAnimation, float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    check(CameraAnimation.Player);
    check(CameraAnimation.CameraStandIn);

    const FCameraAnimationParams Params = CameraAnimation.Params;
    UCameraAnimationSequencePlayer* Player = CameraAnimation.Player;
    UCameraAnimationSequenceCameraStandIn* CameraStandIn = CameraAnimation.CameraStandIn;

    const FFrameRate InputRate = Player->GetInputRate();
    const FFrameTime CurrentPosition = Player->GetCurrentPosition();
    const float CurrentTime = InputRate.AsSeconds(CurrentPosition);
    const float DurationTime = InputRate.AsSeconds(Player->GetDuration()) * Params.PlayRate;

    const float ScaledDeltaTime = DeltaTime * Params.PlayRate;

    const float NewTime = CurrentTime + ScaledDeltaTime;
    const FFrameTime NewPosition = CurrentPosition + DeltaTime * Params.PlayRate * InputRate;

    // Advance any easing times.
    if (CameraAnimation.bIsEasingIn)
    {
        CameraAnimation.EaseInCurrentTime += DeltaTime;
    }
    if (CameraAnimation.bIsEasingOut)
    {
        CameraAnimation.EaseOutCurrentTime += DeltaTime;
    }

    // Start easing out if we're nearing the end.
    // CameraAnimation may already be easing out if StopCameraAnimation has been called.
    if (!Player->GetIsLooping() && !CameraAnimation.bIsEasingOut)
    {
        const float BlendOutStartTime = DurationTime - Params.EaseOutDuration;
        if (NewTime > BlendOutStartTime)
        {
            CameraAnimation.bIsEasingOut = true;
            CameraAnimation.EaseOutCurrentTime = NewTime - BlendOutStartTime;

            if (!bWasEasingOut)
            {
                // Here animation has just started easing out but hasn't finished yet
                OnAnimationEaseOutStarted.ExecuteIfBound(CameraAnimation.Sequence);
            }
        }
    }

    // Check if we're done easing in or out.
    bool bIsDoneEasingOut = false;
    if (CameraAnimation.bIsEasingIn)
    {
        if (CameraAnimation.EaseInCurrentTime > Params.EaseInDuration || Params.EaseInDuration == 0.f)
        {
            CameraAnimation.bIsEasingIn = false;
        }
    }
    if (CameraAnimation.bIsEasingOut)
    {
        if (CameraAnimation.EaseOutCurrentTime > Params.EaseOutDuration)
        {
            bIsDoneEasingOut = true;
        }
    }

    // Figure out the final easing weight.
    const float EasingInT = FMath::Clamp((CameraAnimation.EaseInCurrentTime / Params.EaseInDuration), 0.f, 1.f);
    const float EasingInWeight = CameraAnimation.bIsEasingIn ?
        EvaluateEasing(Params.EaseInType, EasingInT) : 1.f;

    const float EasingOutT = FMath::Clamp((1.f - CameraAnimation.EaseOutCurrentTime / Params.EaseOutDuration), 0.f, 1.f);
    const float EasingOutWeight = CameraAnimation.bIsEasingOut ?
        EvaluateEasing(Params.EaseOutType, EasingOutT) : 1.f;

    const float TotalEasingWeight = FMath::Min(EasingInWeight, EasingOutWeight);

    // We might be done playing. Normally the player will stop on its own, but there are other situation in which
    // the responsibility falls to this code:
    // - If the animation is looping and waiting for an explicit Stop() call on us.
    // - If there was a Stop() call with bImmediate=false to let an animation blend out.
    if (bIsDoneEasingOut || TotalEasingWeight <= 0.f)
    {
        Player->Stop();
        return;
    }

    UMovieSceneEntitySystemLinker* Linker = Player->GetEvaluationTemplate().GetEntitySystemLinker();
    CameraStandIn->Reset(InOutPOV, Linker);

    // Get the "unanimated" properties that need to be treated additively.
    const float OriginalFieldOfView = CameraStandIn->FieldOfView;

    // Update the sequence.
    Player->Update(NewPosition);

    // Recalculate properties that might be invalidated by other properties having been animated.
    CameraStandIn->RecalcDerivedData();

    // Grab the final animated (animated) values, figure out the delta, apply scale, and feed that into the result.
    // Transform is always treated as a local, additive value. The data better be good.
    const float Scale = Params.Scale * TotalEasingWeight;
    const FTransform AnimatedTransform = CameraStandIn->GetTransform();
    FVector AnimatedLocation = AnimatedTransform.GetLocation() * Scale;
    FRotator AnimatedRotation = AnimatedTransform.GetRotation().Rotator() * Scale;
    const FCameraAnimationHelperOffset CameraOffset{ AnimatedLocation, AnimatedRotation };

    FVector OwnerLocation = GetViewTarget()->GetActorLocation();

    // If using a character, camera should start from the pivot location of the mesh.
    {
        ACharacter* OwnerCharacter = GetViewTargetAs<ACharacter>();
        if (OwnerCharacter && OwnerCharacter->GetMesh())
        {
            OwnerLocation = OwnerCharacter->GetMesh()->GetComponentLocation();
        }
    }

    FRotator const OwnerRot = FRotator(0.f, GetViewTarget()->GetActorRotation().Yaw, 0.f);
    const FMatrix OwnerRotationMatrix = FRotationMatrix(OwnerRot);

    FMinimalViewInfo InPOV = InOutPOV;

    // Blend from current camera location to actor location
    InPOV.Location = FMath::Lerp(InOutPOV.Location, OwnerLocation, Scale);
    InPOV.Rotation = FMath::Lerp(InOutPOV.Rotation, OwnerRot, Scale);

    FCameraAnimationHelper::ApplyOffset(OwnerRotationMatrix, InPOV, CameraOffset, AnimatedLocation, AnimatedRotation);

    InOutPOV.Location = AnimatedLocation;
    InOutPOV.Rotation = AnimatedRotation;

    // Blend back depending on reset type
    if (CurrentResetType != ECameraAnimationResetType::BackToStart && CameraOwner && CameraOwner->GetOwningPlayerController() && CameraAnimation.bIsEasingOut && !bWasEasingOut)
    {
        FRotator TargetRot = OwnerRot;
        bool const bIsStrafing = UGCCameraManager->IsOwnerStrafing();
        if (!bIsStrafing && CurrentResetType == ECameraAnimationResetType::ContinueFromEnd)
        {
            TargetRot = FRotator(InOutPOV.Rotation.Pitch, InOutPOV.Rotation.Yaw, 0.f);
        }
        CameraOwner->GetOwningPlayerController()->SetControlRotation(TargetRot);
    }

    // FieldOfView follows the current camera's value every frame, so we can compute how much the animation is
    // changing it.
    const float AnimatedFieldOfView = CameraStandIn->FieldOfView;
    const float DeltaFieldOfView = AnimatedFieldOfView - OriginalFieldOfView;
    InOutPOV.FOV = OriginalFieldOfView + DeltaFieldOfView * Scale;

    // Add the post-process settings.
    if (CameraOwner != nullptr && CameraStandIn->PostProcessBlendWeight > 0.f)
    {
        CameraOwner->AddCachedPPBlend(CameraStandIn->PostProcessSettings, CameraStandIn->PostProcessBlendWeight);
    }

    bWasEasingOut = CameraAnimation.bIsEasingOut;
}

#if ENABLE_DRAW_DEBUG
void UUGC_CameraAnimationModifier::UGCDebugAnimation(FActiveCameraAnimationInfo& ActiveAnimation, float DeltaTime)
{
    if (bDebug && ActiveAnimation.IsValid() && GEngine)
    {
        const FFrameRate InputRate = ActiveAnimation.Player->GetInputRate();
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3
        const FFrameTime DurationFrames = ActiveAnimation.Player->GetDuration();
#elif ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION < 3
        const FFrameNumber DurationFrames = ActiveAnimation.Player->GetDuration();
#endif
        const FFrameTime CurrentPosition = ActiveAnimation.Player->GetCurrentPosition();

        const float CurrentTime = InputRate.AsSeconds(CurrentPosition);
        const float DurationSeconds = InputRate.AsSeconds(DurationFrames);

        const FString LoopString = ActiveAnimation.Params.bLoop ? TEXT(" - Looping") : TEXT("");
        const FString EaseInString = ActiveAnimation.bIsEasingIn ? FString::Printf(TEXT(" - Easing In: %f / %f"), ActiveAnimation.EaseInCurrentTime, ActiveAnimation.Params.EaseInDuration) : TEXT("");
        const FString EaseOutString = ActiveAnimation.bIsEasingOut ? FString::Printf(TEXT(" - Easing Out: %f / %f"), ActiveAnimation.EaseOutCurrentTime, ActiveAnimation.Params.EaseOutDuration) : TEXT("");
        const FString DebugText = FString::Printf(TEXT("UGC_CameraAnimationModifier: %s - PlayRate: %f%s - Duration: %f - Elapsed: %f%s%s"), *GetNameSafe(ActiveAnimation.Sequence), ActiveAnimation.Params.PlayRate, *LoopString, DurationSeconds, CurrentTime, *EaseInString, *EaseOutString);

        GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor(49, 61, 255), DebugText);
    }
}
#endif