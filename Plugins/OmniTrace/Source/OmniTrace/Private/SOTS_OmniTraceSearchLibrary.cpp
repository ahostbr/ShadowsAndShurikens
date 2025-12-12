#include "SOTS_OmniTraceSearchLibrary.h"

#include "Engine/World.h"

FSOTS_SearchPatternResult USOTS_OmniTraceSearchLibrary::SOTS_RequestSearchPattern(
    UObject* WorldContextObject,
    FVector Origin,
    float Radius,
    int32 NumPoints,
    FGameplayTag PatternTag)
{
    FSOTS_SearchPatternResult Result;

    if (!WorldContextObject || Radius <= 0.f || NumPoints <= 0)
    {
        return Result;
    }

    // Phase 1: simple circle of waypoints around the origin.
    const float AngleStep = 2.0f * PI / static_cast<float>(NumPoints);

    for (int32 Index = 0; Index < NumPoints; ++Index)
    {
        const float Angle = AngleStep * static_cast<float>(Index);
        const float X = Origin.X + Radius * FMath::Cos(Angle);
        const float Y = Origin.Y + Radius * FMath::Sin(Angle);

        Result.Waypoints.Add(FVector(X, Y, Origin.Z));
    }

    return Result;
}

