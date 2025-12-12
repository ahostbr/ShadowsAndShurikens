#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_WaypointSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSOTS_WaypointEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Waypoints")
    FGuid Id;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Waypoints")
    TWeakObjectPtr<AActor> TargetActor;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Waypoints")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Waypoints")
    FGameplayTag CategoryTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|Waypoints")
    bool bClampToScreenEdges = true;
};

UCLASS()
class SOTS_UI_API USOTS_WaypointSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="SOTS|Waypoints")
    FSOTS_WaypointEntry AddActorWaypoint(AActor* Target, FGameplayTag CategoryTag, bool bClampToEdges);

    UFUNCTION(BlueprintCallable, Category="SOTS|Waypoints")
    FSOTS_WaypointEntry AddLocationWaypoint(const FVector& Location, FGameplayTag CategoryTag, bool bClampToEdges);

    UFUNCTION(BlueprintCallable, Category="SOTS|Waypoints")
    void RemoveWaypoint(FGuid Id);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Waypoints")
    const TArray<FSOTS_WaypointEntry>& GetWaypoints() const { return Waypoints; }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSOTS_OnWaypointsChanged);

    UPROPERTY(BlueprintAssignable, Category="SOTS|Waypoints")
    FSOTS_OnWaypointsChanged OnWaypointsChanged;

private:
    UPROPERTY()
    TArray<FSOTS_WaypointEntry> Waypoints;
};
