#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_MissionDirectorDebugLibrary.generated.h"

/**
 * Debug helpers for quickly verifying Mission Director behavior.
 */
UCLASS()
class SOTS_MISSIONDIRECTOR_API USOTS_MissionDirectorDebugLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Runs a minimal mission flow in one call:
     * - Starts a debug mission if not already active.
     * - Registers a primary objective completion worth +250 points.
     * - Registers an extra generic score event (optional).
     * - Ends the mission and prints the resulting score, rank and duration.
     */
    UFUNCTION(BlueprintCallable, Category="SOTS|Mission|Debug", meta=(WorldContext="WorldContextObject"))
    static void RunSOTS_MissionDirector_DebugRun(
        const UObject* WorldContextObject,
        float ExtraScoreDelta);

    // Tag-driven helpers for marking and querying mission state using the
    // global player tag container. These functions do not modify Mission
    // Director state directly; they simply manipulate tags.
    UFUNCTION(BlueprintCallable, Category="SOTS|MissionDirector", meta=(WorldContext="WorldContextObject"))
    static void SOTS_MarkMissionStarted(const UObject* WorldContextObject, FGameplayTag MissionStartTag);

    UFUNCTION(BlueprintCallable, Category="SOTS|MissionDirector", meta=(WorldContext="WorldContextObject"))
    static void SOTS_MarkMissionCompleted(const UObject* WorldContextObject, FGameplayTag MissionCompleteTag);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|MissionDirector", meta=(WorldContext="WorldContextObject"))
    static bool SOTS_IsMissionCompleted(const UObject* WorldContextObject, FGameplayTag MissionCompleteTag);
};
