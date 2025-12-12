// Copyright(c) Aurora Devs 2024. All Rights Reserved.


#include "Camera/Modifiers/UGC_CameraCollisionModifier.h"

#include "Camera/PlayerCameraManager.h"
#include "Camera/UGC_PlayerCameraManager.h"
#include "CineCameraActor.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CameraBlockingVolume.h"
#include "GameFramework/SpringArmComponent.h"
#include "Runtime/Launch/Resources/Version.h"

UUGC_CameraCollisionModifier::UUGC_CameraCollisionModifier()
{
    Priority = 134;
    bExclusive = true;
}

bool UUGC_CameraCollisionModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    Super::ModifyCamera(DeltaTime, InOutPOV);

    if (!bPlayDuringCameraAnimations)
    {
        if (UGCCameraManager && UGCCameraManager->IsPlayingAnyCameraAnimation())
        {
            return false;
        }
    }

    if (SpringArm)
    {
        // Don't do collision tests is spring arm is taking care of them.
        if (SpringArm->bDoCollisionTest && CollisionSettings.bPreventPenetration)
        {
#if ENABLE_DRAW_DEBUG
            if (GEngine != nullptr)
            {
                FString const& CameraManagerName = GetNameSafe(CameraOwner);
                FString const& ActorName = GetNameSafe(GetViewTarget());
                FString const& SpringName = GetNameSafe(SpringArm);
                FString const& DebugText = FString::Printf(TEXT("Actor %s has Collision Checks enabled on SpringArm %s but Camera Manager %s has Camera Modifier UGC_CameraCollisionModifier. This is not allowed so the modifier will be aborted.")
                                                           , *ActorName, *SpringName, *CameraManagerName);
                GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, DebugText);
            }
#endif
            return false;
        }
    }

    // Do collision checks
    UpdatePreventPenetration(DeltaTime, InOutPOV);

    return false;
}

void UUGC_CameraCollisionModifier::UpdatePreventPenetration(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    if (!CollisionSettings.bPreventPenetration)
    {
        return;
    }

//    // If we're playing a level sequence, don't do anything as long as we're not blending back to gameplay.
//    if (CollisionSettings.bDisableDuringCinematics && UGCCameraManager && UGCCameraManager->IsPlayingAnyLevelSequence())
//    {
//        // This condition is *false* when blending out of Cinematics back to Gameplay.
//        if (OwnerPawn != CameraOwner->PendingViewTarget.Target)
//        {
//#if ENABLE_DRAW_DEBUG
//            if (bDebug && GEngine != nullptr)
//            {
//                GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Turquoise
//                    , FString::Printf(TEXT("UGC_CameraCollisionModifier: View Target is not player character (in Level Sequence). No collision checks.")));
//            }
//#endif
//            return;
//        }
//    }

    FVector const SafeLocation = GetTraceSafeLocation(InOutPOV);

    // Then aim line to desired camera position
    bool const bSingleRayPenetrationCheck = !CollisionSettings.bDoPredictiveAvoidance;
    if (AActor* TargetActor = GetViewTarget())
    {
        PreventCameraPenetration(*TargetActor, SafeLocation, InOutPOV.Location, DeltaTime, AimLineToDesiredPosBlockedPct, bSingleRayPenetrationCheck);
    }
}

FVector UUGC_CameraCollisionModifier::GetTraceSafeLocation(FMinimalViewInfo const& InPOV)
{
    AActor* TargetActor = GetViewTarget();
    FVector SafeLocation = TargetActor ? TargetActor->GetActorLocation() : FVector::Zero();
    if (TargetActor)
    {
        if (SpringArm)
        {
            SafeLocation = SpringArm->GetComponentLocation() + SpringArm->TargetOffset;
        }
        else if (UPrimitiveComponent const* PPActorRootComponent = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
        {
            // Attempt at picking SafeLocation automatically, so we reduce camera translation when aiming.
            // Our camera is our reticle, so we want to preserve our aim and keep that as steady and smooth as possible.
            // Pick closest point on capsule to our aim line.
            FVector ClosestPointOnLineToCapsuleCenter;
            FMath::PointDistToLine(SafeLocation, InPOV.Rotation.Vector(), InPOV.Location, ClosestPointOnLineToCapsuleCenter);

            // Adjust Safe distance height to be same as aim line, but within capsule.
            float const PushInDistance = CollisionSettings.PenetrationAvoidanceFeelers[0].ProbeRadius;
            float const MaxHalfHeight = TargetActor->GetSimpleCollisionHalfHeight() - PushInDistance;
            SafeLocation.Z = FMath::Clamp(ClosestPointOnLineToCapsuleCenter.Z, SafeLocation.Z - MaxHalfHeight, SafeLocation.Z + MaxHalfHeight);

            float DistanceSqr = 0.f;
            PPActorRootComponent->GetSquaredDistanceToCollision(ClosestPointOnLineToCapsuleCenter, DistanceSqr, SafeLocation);
            // Push back inside capsule to avoid initial penetration when doing line checks.
            if (CollisionSettings.PenetrationAvoidanceFeelers.Num() > 0)
            {
                SafeLocation += (SafeLocation - ClosestPointOnLineToCapsuleCenter).GetSafeNormal() * PushInDistance;
            }
        }
    }
    return SafeLocation;
}

void UUGC_CameraCollisionModifier::PreventCameraPenetration(AActor const& ViewTarget, FVector const& SafeLoc, FVector& OutCameraLoc, float const& DeltaTime, float& OutDistBlockedPct, bool bSingleRayOnly)
{
#if ENABLE_DRAW_DEBUG
    DebugActorsHitDuringCameraPenetration.Reset();
#endif

    FVector const& CameraLoc = OutCameraLoc;
    float HardBlockedPct = OutDistBlockedPct;
    float SoftBlockedPct = OutDistBlockedPct;

    FVector BaseRay = CameraLoc - SafeLoc;
    FRotationMatrix BaseRayMatrix(BaseRay.Rotation());
    FVector BaseRayLocalUp, BaseRayLocalFwd, BaseRayLocalRight;

    BaseRayMatrix.GetScaledAxes(BaseRayLocalFwd, BaseRayLocalRight, BaseRayLocalUp);

    float DistBlockedPctThisFrame = 1.f;

    int32 const NumRaysToShoot = bSingleRayOnly ? FMath::Min(1, CollisionSettings.PenetrationAvoidanceFeelers.Num()) : CollisionSettings.PenetrationAvoidanceFeelers.Num();
    FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(CameraPen), false, nullptr/*PlayerCamera*/);

    SphereParams.AddIgnoredActor(&ViewTarget);

    FCollisionShape SphereShape = FCollisionShape::MakeSphere(0.f);
    UWorld* World = GetWorld();

    for (int32 RayIdx = 0; RayIdx < NumRaysToShoot; ++RayIdx)
    {
        FPenetrationAvoidanceFeeler& Feeler = CollisionSettings.PenetrationAvoidanceFeelers[RayIdx];
        FRotator AdjustmentRot = Feeler.AdjustmentRot;

        if (RayIdx == 0 && Feeler.AdjustmentRot != FRotator::ZeroRotator)
        {
#if ENABLE_DRAW_DEBUG
            if (GEngine != nullptr)
            {
                GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, TEXT("UGC_CameraCollisionModifier: First Penetration Avoidance Feeler should always have an adjustment roation equal to 0,0,0!."));
            }
#endif
            AdjustmentRot = FRotator::ZeroRotator;
        }

            // calc ray target
            FVector RayTarget;
            {
                FVector RotatedRay = BaseRay.RotateAngleAxis(AdjustmentRot.Yaw, BaseRayLocalUp);
                RotatedRay = RotatedRay.RotateAngleAxis(AdjustmentRot.Pitch, BaseRayLocalRight);
                RayTarget = SafeLoc + RotatedRay;
            }

            // Cast for world and pawn hits separately.  this is so we can safely ignore the camera's target pawn.
            SphereShape.Sphere.Radius = Feeler.ProbeRadius;
            ECollisionChannel TraceChannel = ECC_Camera;

            // Do multi-line check to make sure the hits we throw out aren't masking real hits behind (these are important rays).
            // Passing camera as actor so that camerablockingvolumes know when it's the camera doing traces.
            FHitResult Hit;
            const bool bHit = World->SweepSingleByChannel(Hit, SafeLoc, RayTarget, FQuat::Identity, TraceChannel, SphereShape, SphereParams);
#if ENABLE_DRAW_DEBUG
            if (World->TimeSince(LastDrawDebugTime) < 1.f)
            {
                DrawDebugSphere(World, SafeLoc, SphereShape.Sphere.Radius, 8, FColor::Red);
                DrawDebugSphere(World, bHit ? Hit.Location : RayTarget, SphereShape.Sphere.Radius, 8, FColor::Red);
                DrawDebugLine(World, SafeLoc, bHit ? Hit.Location : RayTarget, FColor::Red);
            }
#endif // ENABLE_DRAW_DEBUG

        const AActor* HitActor = Hit.GetActor();

        if (bHit && HitActor)
        {
            bool bIgnoreHit = false;

            if (HitActor->ActorHasTag(CollisionSettings.IgnoreCameraCollisionTag))
            {
                bIgnoreHit = true;
                SphereParams.AddIgnoredActor(HitActor);
            }

            // Ignore CameraBlockingVolume hits that occur in front of the ViewTarget.
            if (!bIgnoreHit && HitActor->IsA<ACameraBlockingVolume>())
            {
                const FVector ViewTargetForwardXY = ViewTarget.GetActorForwardVector().GetSafeNormal2D();
                const FVector ViewTargetLocation = ViewTarget.GetActorLocation();
                const FVector HitOffset = Hit.Location - ViewTargetLocation;
                const FVector HitDirectionXY = HitOffset.GetSafeNormal2D();
                const float DotHitDirection = FVector::DotProduct(ViewTargetForwardXY, HitDirectionXY);
                if (DotHitDirection > 0.0f)
                {
                    bIgnoreHit = true;
                    // Ignore this CameraBlockingVolume on the remaining sweeps.
                    SphereParams.AddIgnoredActor(HitActor);
                }
                else
                {
#if ENABLE_DRAW_DEBUG
                    DebugActorsHitDuringCameraPenetration.AddUnique(TObjectPtr<const AActor>(HitActor));
#endif
                }
            }

            if (!bIgnoreHit)
            {
                float const Weight = Cast<APawn>(Hit.GetActor()) ? Feeler.PawnWeight : Feeler.WorldWeight;
                float NewBlockPct = Hit.Time;
                NewBlockPct += (1.f - NewBlockPct) * (1.f - Weight);

                // Recompute blocked pct taking into account pushout distance.
                NewBlockPct = (Hit.Location - SafeLoc).Size() / (RayTarget - SafeLoc).Size();
                DistBlockedPctThisFrame = FMath::Min(NewBlockPct, DistBlockedPctThisFrame);

#if ENABLE_DRAW_DEBUG
                DebugActorsHitDuringCameraPenetration.AddUnique(TObjectPtr<const AActor>(HitActor));
#endif
            }
        }

        if (RayIdx == 0)
        {
            // Don't interpolate toward this one, snap to it. THIS ASSUMES RAY 0 IS THE CENTER/MAIN RAY.
            HardBlockedPct = DistBlockedPctThisFrame;
        }
        else
        {
            SoftBlockedPct = DistBlockedPctThisFrame;
        }
    }

    if (OutDistBlockedPct < DistBlockedPctThisFrame)
    {
        // interpolate smoothly out
        if (CollisionSettings.PenetrationBlendOutTime > DeltaTime)
        {
            OutDistBlockedPct = OutDistBlockedPct + DeltaTime / CollisionSettings.PenetrationBlendOutTime * (DistBlockedPctThisFrame - OutDistBlockedPct);
        }
        else
        {
            OutDistBlockedPct = DistBlockedPctThisFrame;
        }
    }
    else
    {
        if (OutDistBlockedPct > HardBlockedPct)
        {
            OutDistBlockedPct = HardBlockedPct;
        }
        else if (OutDistBlockedPct > SoftBlockedPct)
        {
            // interpolate smoothly in
            if (CollisionSettings.PenetrationBlendInTime > DeltaTime)
            {
                OutDistBlockedPct = OutDistBlockedPct - DeltaTime / CollisionSettings.PenetrationBlendInTime * (OutDistBlockedPct - SoftBlockedPct);
            }
            else
            {
                OutDistBlockedPct = SoftBlockedPct;
            }
        }
    }

    OutDistBlockedPct = FMath::Clamp<float>(OutDistBlockedPct, 0.f, 1.f);
    if (OutDistBlockedPct < (1.f - ZERO_ANIMWEIGHT_THRESH))
    {
        OutCameraLoc = SafeLoc + (CameraLoc - SafeLoc) * OutDistBlockedPct;
    }
}

//void UUGC_CameraCollisionModifier::ProcessBoomLengthAndFOV_Implementation(float DeltaTime, float InFOV,
//    float InArmLength, FVector ViewLocation, FRotator ViewRotation, float& OutFOV, float& OutArmLength)
//{
//    Super::ProcessBoomLengthAndFOV_Implementation(DeltaTime, InFOV, InArmLength, ViewLocation, ViewRotation, OutFOV,
//        OutArmLength);
//
//    return;
//
//    // When game starts.
//    static FMinimalViewInfo const DefaultViewInfo = FMinimalViewInfo();
//    if (OnDisabledPOV.Equals(DefaultViewInfo))
//    {
//        OnDisabledPOV = CachedPOV;
//    }
//
//    if (!CollisionSettings.bPreventPenetration)
//    {
//        return;
//    }
//
//    if (CollisionSettings.bDisableDuringCinematics && UGCCameraManager && UGCCameraManager->IsPlayingAnyLevelSequence())
//    {
//#if ENABLE_DRAW_DEBUG
//        if (bDebug && GEngine != nullptr)
//        {
//            GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Turquoise
//                , FString::Printf(TEXT("UGC_CameraCollisionModifier: View Target is not player character (in Level Sequence). No collision checks.")));
//        }
//#endif
//        return;
//    }
//
//    FVector const SafeLocation = GetTraceSafeLocation(CachedPOV);
//
//    // Then aim line to desired camera position
//    bool const bSingleRayPenetrationCheck = !CollisionSettings.bDoPredictiveAvoidance;
//    if (AActor* TargetActor = GetViewTarget())
//    {
//        PreventCameraPenetration2(*TargetActor, SafeLocation, CachedPOV.Location, OutArmLength, DeltaTime, AimLineToDesiredPosBlockedPct, bSingleRayPenetrationCheck);
//    }
//}
//
//void UUGC_CameraCollisionModifier::PreventCameraPenetration2(AActor const& ViewTarget, FVector const& SafeLoc, FVector const& CameraLoc, float& OutArmLength, float const& DeltaTime, float& OutDistBlockedPct, bool bSingleRayOnly)
//{
//#if ENABLE_DRAW_DEBUG
//    DebugActorsHitDuringCameraPenetration.Reset();
//#endif
//
//    float HardBlockedPct = OutDistBlockedPct;
//    float SoftBlockedPct = OutDistBlockedPct;
//
//    FVector BaseRay = CameraLoc - SafeLoc;
//    FRotationMatrix BaseRayMatrix(BaseRay.Rotation());
//    FVector BaseRayLocalUp, BaseRayLocalFwd, BaseRayLocalRight;
//
//    BaseRayMatrix.GetScaledAxes(BaseRayLocalFwd, BaseRayLocalRight, BaseRayLocalUp);
//
//    float DistBlockedPctThisFrame = 1.f;
//
//    int32 const NumRaysToShoot = bSingleRayOnly ? FMath::Min(1, CollisionSettings.PenetrationAvoidanceFeelers.Num()) : CollisionSettings.PenetrationAvoidanceFeelers.Num();
//    FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(CameraPen), false, nullptr/*PlayerCamera*/);
//
//    SphereParams.AddIgnoredActor(&ViewTarget);
//
//    FCollisionShape SphereShape = FCollisionShape::MakeSphere(0.f);
//    UWorld* World = GetWorld();
//
//    for (int32 RayIdx = 0; RayIdx < NumRaysToShoot; ++RayIdx)
//    {
//        FPenetrationAvoidanceFeeler& Feeler = CollisionSettings.PenetrationAvoidanceFeelers[RayIdx];
//        FRotator AdjustmentRot = Feeler.AdjustmentRot;
//
//        if (RayIdx == 0 && Feeler.AdjustmentRot != FRotator::ZeroRotator)
//        {
//#if ENABLE_DRAW_DEBUG
//            if (GEngine != nullptr)
//            {
//                GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, TEXT("UGC_CameraCollisionModifier: First Penetration Avoidance Feeler should always have an adjustment roation equal to 0,0,0!."));
//            }
//#endif
//            AdjustmentRot = FRotator::ZeroRotator;
//        }
//
//        // calc ray target
//        FVector RayTarget;
//        {
//            FVector RotatedRay = BaseRay.RotateAngleAxis(AdjustmentRot.Yaw, BaseRayLocalUp);
//            RotatedRay = RotatedRay.RotateAngleAxis(AdjustmentRot.Pitch, BaseRayLocalRight);
//            RayTarget = SafeLoc + RotatedRay;
//        }
//
//        // Cast for world and pawn hits separately.  this is so we can safely ignore the camera's target pawn.
//        SphereShape.Sphere.Radius = Feeler.ProbeRadius;
//        ECollisionChannel TraceChannel = ECC_Camera;
//
//        // Do multi-line check to make sure the hits we throw out aren't masking real hits behind (these are important rays).
//        // Passing camera as actor so that camerablockingvolumes know when it's the camera doing traces.
//        FHitResult Hit;
//        const bool bHit = World->SweepSingleByChannel(Hit, SafeLoc, RayTarget, FQuat::Identity, TraceChannel, SphereShape, SphereParams);
//#if ENABLE_DRAW_DEBUG
//        if (World->TimeSince(LastDrawDebugTime) < 1.f)
//        {
//            DrawDebugSphere(World, SafeLoc, SphereShape.Sphere.Radius, 8, FColor::Red);
//            DrawDebugSphere(World, bHit ? Hit.Location : RayTarget, SphereShape.Sphere.Radius, 8, FColor::Red);
//            DrawDebugLine(World, SafeLoc, bHit ? Hit.Location : RayTarget, FColor::Red);
//        }
//#endif // ENABLE_DRAW_DEBUG
//
//        const AActor* HitActor = Hit.GetActor();
//
//        if (bHit && HitActor)
//        {
//            bool bIgnoreHit = false;
//
//            if (HitActor->ActorHasTag(CollisionSettings.IgnoreCameraCollisionTag))
//            {
//                bIgnoreHit = true;
//                SphereParams.AddIgnoredActor(HitActor);
//            }
//
//            // Ignore CameraBlockingVolume hits that occur in front of the ViewTarget.
//            if (!bIgnoreHit && HitActor->IsA<ACameraBlockingVolume>())
//            {
//                const FVector ViewTargetForwardXY = ViewTarget.GetActorForwardVector().GetSafeNormal2D();
//                const FVector ViewTargetLocation = ViewTarget.GetActorLocation();
//                const FVector HitOffset = Hit.Location - ViewTargetLocation;
//                const FVector HitDirectionXY = HitOffset.GetSafeNormal2D();
//                const float DotHitDirection = FVector::DotProduct(ViewTargetForwardXY, HitDirectionXY);
//                if (DotHitDirection > 0.0f)
//                {
//                    bIgnoreHit = true;
//                    // Ignore this CameraBlockingVolume on the remaining sweeps.
//                    SphereParams.AddIgnoredActor(HitActor);
//                }
//                else
//                {
//#if ENABLE_DRAW_DEBUG
//                    DebugActorsHitDuringCameraPenetration.AddUnique(TObjectPtr<const AActor>(HitActor));
//#endif
//                }
//            }
//
//            if (!bIgnoreHit)
//            {
//                float const Weight = Cast<APawn>(Hit.GetActor()) ? Feeler.PawnWeight : Feeler.WorldWeight;
//                float NewBlockPct = Hit.Time;
//                NewBlockPct += (1.f - NewBlockPct) * (1.f - Weight);
//
//                // Recompute blocked pct taking into account pushout distance.
//                NewBlockPct = (Hit.Location - SafeLoc).Size() / (RayTarget - SafeLoc).Size();
//                DistBlockedPctThisFrame = FMath::Min(NewBlockPct, DistBlockedPctThisFrame);
//
//#if ENABLE_DRAW_DEBUG
//                DebugActorsHitDuringCameraPenetration.AddUnique(TObjectPtr<const AActor>(HitActor));
//#endif
//            }
//        }
//
//        if (RayIdx == 0)
//        {
//            // Don't interpolate toward this one, snap to it. THIS ASSUMES RAY 0 IS THE CENTER/MAIN RAY.
//            HardBlockedPct = DistBlockedPctThisFrame;
//        }
//        else
//        {
//            SoftBlockedPct = DistBlockedPctThisFrame;
//        }
//    }
//
//    if (OutDistBlockedPct < DistBlockedPctThisFrame)
//    {
//        // interpolate smoothly out
//        if (CollisionSettings.PenetrationBlendOutTime > DeltaTime)
//        {
//            OutDistBlockedPct = OutDistBlockedPct + DeltaTime / CollisionSettings.PenetrationBlendOutTime * (DistBlockedPctThisFrame - OutDistBlockedPct);
//        }
//        else
//        {
//            OutDistBlockedPct = DistBlockedPctThisFrame;
//        }
//    }
//    else
//    {
//        if (OutDistBlockedPct > HardBlockedPct)
//        {
//            OutDistBlockedPct = HardBlockedPct;
//        }
//        else if (OutDistBlockedPct > SoftBlockedPct)
//        {
//            // interpolate smoothly in
//            if (CollisionSettings.PenetrationBlendInTime > DeltaTime)
//            {
//                OutDistBlockedPct = OutDistBlockedPct - DeltaTime / CollisionSettings.PenetrationBlendInTime * (OutDistBlockedPct - SoftBlockedPct);
//            }
//            else
//            {
//                OutDistBlockedPct = SoftBlockedPct;
//            }
//        }
//    }
//
//    OutDistBlockedPct = FMath::Clamp<float>(OutDistBlockedPct, 0.f, 1.f);
//    if (OutDistBlockedPct < (1.f - ZERO_ANIMWEIGHT_THRESH))
//    {
//        FVector const& OutCameraLoc = SafeLoc + (CameraLoc - SafeLoc) * OutDistBlockedPct;
//
//        DrawDebugSphere(World, SafeLoc, 10.f, 16, FColor::White, false, DeltaTime, 1);
//
//        FVector const& Proj1 = CameraLoc + ((SafeLoc - CameraLoc) | CachedPOV.Rotation.Vector()) * CachedPOV.Rotation.Vector();
//
//        DrawDebugSphere(World, CameraLoc, 10.f, 16, FColor::Red, false, DeltaTime, 1);
//        DrawDebugSphere(World, Proj1, 10.f, 16, FColor::Red, false, DeltaTime, 1);
//
//        DrawDebugLine(World, CameraLoc, Proj1, FColor::Red, false, DeltaTime, 1);
//        DrawDebugLine(World, CameraLoc, SafeLoc, FColor::Red, false, DeltaTime, 1);
//        DrawDebugLine(World, Proj1, SafeLoc, FColor::Red, false, DeltaTime, 1);
//
//        float const Result = (SafeLoc - OutCameraLoc) | CachedPOV.Rotation.Vector();
//        FVector const& Proj2 = OutCameraLoc + Result * CachedPOV.Rotation.Vector();
//
//        DrawDebugSphere(World, OutCameraLoc, 10.f, 16, FColor::Green, false, DeltaTime, 1);
//        DrawDebugSphere(World, Proj2, 10.f, 16, FColor::Green, false, DeltaTime, 1);
//
//        DrawDebugLine(World, OutCameraLoc, SafeLoc, FColor::Green, false, DeltaTime, 1);
//        DrawDebugLine(World, OutCameraLoc, Proj2, FColor::Green, false, DeltaTime, 1);
//        DrawDebugLine(World, SafeLoc, Proj2, FColor::Green, false, DeltaTime, 1);
//
//        if (GEngine != nullptr)
//        {
//            GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, FString::Printf(TEXT("Old Hyp.: %.2f"), FVector::Dist(CameraLoc, SafeLoc)));
//            GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Yellow, FString::Printf(TEXT("Old Hyp. * Pct: %.2f"), FVector::Dist(CameraLoc, SafeLoc) * OutDistBlockedPct));
//            GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Green, FString::Printf(TEXT("New Hyp.: %.2f"), FVector::Dist(OutCameraLoc, SafeLoc)));
//            GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Green, FString::Printf(TEXT("New Arm Length: %.2f"), Result));
//        }
//        OutArmLength *= OutDistBlockedPct;
//    }
//}