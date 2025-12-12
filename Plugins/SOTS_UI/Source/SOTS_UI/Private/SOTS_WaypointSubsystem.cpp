#include "SOTS_WaypointSubsystem.h"

FSOTS_WaypointEntry USOTS_WaypointSubsystem::AddActorWaypoint(AActor* Target, FGameplayTag CategoryTag, bool bClampToEdges)
{
    FSOTS_WaypointEntry Entry;
    Entry.Id = FGuid::NewGuid();
    Entry.TargetActor = Target;
    Entry.WorldLocation = Target ? Target->GetActorLocation() : FVector::ZeroVector;
    Entry.CategoryTag = CategoryTag;
    Entry.bClampToScreenEdges = bClampToEdges;

    Waypoints.Add(Entry);
    OnWaypointsChanged.Broadcast();
    return Entry;
}

FSOTS_WaypointEntry USOTS_WaypointSubsystem::AddLocationWaypoint(const FVector& Location, FGameplayTag CategoryTag, bool bClampToEdges)
{
    FSOTS_WaypointEntry Entry;
    Entry.Id = FGuid::NewGuid();
    Entry.WorldLocation = Location;
    Entry.CategoryTag = CategoryTag;
    Entry.bClampToScreenEdges = bClampToEdges;

    Waypoints.Add(Entry);
    OnWaypointsChanged.Broadcast();
    return Entry;
}

void USOTS_WaypointSubsystem::RemoveWaypoint(FGuid Id)
{
    const int32 Removed = Waypoints.RemoveAll([Id](const FSOTS_WaypointEntry& Entry)
    {
        return Entry.Id == Id;
    });

    if (Removed > 0)
    {
        OnWaypointsChanged.Broadcast();
    }
}
