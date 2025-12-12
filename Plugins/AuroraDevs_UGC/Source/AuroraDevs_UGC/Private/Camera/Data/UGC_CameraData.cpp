// Copyright(c) Aurora Devs 2024. All Rights Reserved.


#include "Camera/Data/UGC_CameraData.h"

FPenetrationAvoidanceFeeler::FPenetrationAvoidanceFeeler()
    : AdjustmentRot(ForceInit)
    , WorldWeight(0.f)
    , PawnWeight(0.f)
    , ProbeRadius(0)
{
}

FPenetrationAvoidanceFeeler::FPenetrationAvoidanceFeeler(const FRotator& InAdjustmentRot, const float& InWorldWeight, const float& InPawnWeight, const float& InExtent)
    : AdjustmentRot(InAdjustmentRot)
    , WorldWeight(InWorldWeight)
    , PawnWeight(InPawnWeight)
    , ProbeRadius(InExtent)
{
}

FCameraCollisionSettings::FCameraCollisionSettings()
{
    //                              AdjusmentRotation,                   WorldWeight,    PawnWeight,     Extent
    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+0.00f, +00.00f, 0.00f), 0.50f, 1.00f, 15.00f));

    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+0.00f, +5.00f, 0.00f), 0.20f, 0.75f, 15.00f));
    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+0.00f, +10.0f, 0.00f), 0.20f, 0.75f, 15.00f));
    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+0.00f, -5.00f, 0.00f), 0.20f, 0.75f, 15.00f));
    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+0.00f, -10.0f, 0.00f), 0.20f, 0.75f, 15.00f));

    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+0.00f, +5.00f, 0.00f), 0.15f, 0.50f, 15.00f));
    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+0.00f, +10.0f, 0.00f), 0.15f, 0.50f, 15.00f));
    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+0.00f, -5.00f, 0.00f), 0.15f, 0.50f, 15.00f));
    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+0.00f, -10.0f, 0.00f), 0.15f, 0.50f, 15.00f));

    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(+15.0f, +0.00f, 0.00f), 0.50f, 1.00f, 10.00f));
    PenetrationAvoidanceFeelers.Add(FPenetrationAvoidanceFeeler(FRotator(-10.0f, +0.00f, 0.00f), 0.50f, 0.50f, 10.00f));
}