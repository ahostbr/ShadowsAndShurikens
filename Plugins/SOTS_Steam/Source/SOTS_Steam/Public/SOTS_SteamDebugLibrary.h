#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_SteamDebugLibrary.generated.h"

class USOTS_SteamAchievementsSubsystem;
class USOTS_SteamLeaderboardsSubsystem;

/**
 * Debug / test helpers for SOTS_Steam.
 *
 * These functions are primarily intended for:
 * - Editor-only test widgets
 * - SOTS_Debug overlays
 * - Quick validation of the SOTS_Steam subsystems without wiring full game logic.
 */
UCLASS()
class SOTS_STEAM_API USOTS_SteamDebugLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // -------------------------
    // Subsystem accessors
    // -------------------------

    // Get the SOTS_Steam Achievements subsystem from any WorldContextObject.
    UFUNCTION(BlueprintPure, Category="SOTS|Steam|Debug", meta=(WorldContext="WorldContextObject"))
    static USOTS_SteamAchievementsSubsystem* GetAchievementsSubsystem(const UObject* WorldContextObject);

    // Get the SOTS_Steam Leaderboards subsystem from any WorldContextObject.
    UFUNCTION(BlueprintPure, Category="SOTS|Steam|Debug", meta=(WorldContext="WorldContextObject"))
    static USOTS_SteamLeaderboardsSubsystem* GetLeaderboardsSubsystem(const UObject* WorldContextObject);

    // -------------------------
    // Achievements debug helpers
    // -------------------------

    // Force-set an achievement as unlocked by InternalId (local + Steam mirror).
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Debug|Achievements", meta=(WorldContext="WorldContextObject"))
    static void DebugUnlockAchievement(const UObject* WorldContextObject, FName InternalId);

    // Request/load current achievement stats for the active profile.
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Debug|Achievements", meta=(WorldContext="WorldContextObject"))
    static void DebugRequestAchievementStats(const UObject* WorldContextObject);

    // Dump current achievement state to the log.
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Debug|Achievements", meta=(WorldContext="WorldContextObject"))
    static void DebugDumpAchievementsToLog(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Debug|Achievements", meta=(WorldContext="WorldContextObject"))
    static bool DebugGetAchievementSyncStatus(const UObject* WorldContextObject, FString& OutStatusMessage);

    // -------------------------
    // Leaderboards debug helpers
    // -------------------------

    // Submit a test score to a leaderboard (local + optional Steam mirror).
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Debug|Leaderboards", meta=(WorldContext="WorldContextObject"))
    static void DebugSubmitScore(
        const UObject* WorldContextObject,
        FName LeaderboardInternalId,
        int32 Score,
        const FString& PlayerName);

    // Request/load leaderboard data for the active profile.
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Debug|Leaderboards", meta=(WorldContext="WorldContextObject"))
    static void DebugRequestLeaderboardData(const UObject* WorldContextObject);

    // Dump current local leaderboard state to the log.
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Debug|Leaderboards", meta=(WorldContext="WorldContextObject"))
    static void DebugDumpLeaderboardsToLog(const UObject* WorldContextObject);

    // Kick off a Steam Top N query for a leaderboard.
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Debug|Leaderboards", meta=(WorldContext="WorldContextObject"))
    static void DebugQuerySteamTop(
        const UObject* WorldContextObject,
        FName LeaderboardInternalId,
        int32 NumEntries);

    // Kick off a Steam "around player" query for a leaderboard.
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Debug|Leaderboards", meta=(WorldContext="WorldContextObject"))
    static void DebugQuerySteamAroundPlayer(
        const UObject* WorldContextObject,
        FName LeaderboardInternalId,
        int32 Range);
};
