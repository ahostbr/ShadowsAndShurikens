#include "SOTS_SteamDebugLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include "SOTS_SteamAchievementsSubsystem.h"
#include "SOTS_SteamLeaderboardsSubsystem.h"
#include "SOTS_SteamLog.h"

namespace
{
    UGameInstance* GetGI(const UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return nullptr;
        }

        if (const UWorld* World = WorldContextObject->GetWorld())
        {
            return World->GetGameInstance();
        }

        return nullptr;
    }
}

USOTS_SteamAchievementsSubsystem* USOTS_SteamDebugLibrary::GetAchievementsSubsystem(const UObject* WorldContextObject)
{
    if (UGameInstance* GI = GetGI(WorldContextObject))
    {
        return GI->GetSubsystem<USOTS_SteamAchievementsSubsystem>();
    }

    return nullptr;
}

USOTS_SteamLeaderboardsSubsystem* USOTS_SteamDebugLibrary::GetLeaderboardsSubsystem(const UObject* WorldContextObject)
{
    if (UGameInstance* GI = GetGI(WorldContextObject))
    {
        return GI->GetSubsystem<USOTS_SteamLeaderboardsSubsystem>();
    }

    return nullptr;
}

void USOTS_SteamDebugLibrary::DebugUnlockAchievement(const UObject* WorldContextObject, FName InternalId)
{
    if (USOTS_SteamAchievementsSubsystem* Subsys = GetAchievementsSubsystem(WorldContextObject))
    {
        Subsys->SetAchievement(InternalId);
        return;
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("DebugUnlockAchievement - Achievements subsystem not available."));
}

void USOTS_SteamDebugLibrary::DebugRequestAchievementStats(const UObject* WorldContextObject)
{
    if (USOTS_SteamAchievementsSubsystem* Subsys = GetAchievementsSubsystem(WorldContextObject))
    {
        Subsys->RequestCurrentStats();
        return;
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("DebugRequestAchievementStats - Achievements subsystem not available."));
}

void USOTS_SteamDebugLibrary::DebugDumpAchievementsToLog(const UObject* WorldContextObject)
{
    if (USOTS_SteamAchievementsSubsystem* Subsys = GetAchievementsSubsystem(WorldContextObject))
    {
        Subsys->DumpAchievementsToLog();
        return;
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("DebugDumpAchievementsToLog - Achievements subsystem not available."));
}

bool USOTS_SteamDebugLibrary::DebugGetAchievementSyncStatus(const UObject* WorldContextObject, FString& OutStatusMessage)
{
    if (USOTS_SteamAchievementsSubsystem* Subsys = GetAchievementsSubsystem(WorldContextObject))
    {
        const bool bReady = Subsys->IsOnlineAchievementSyncAvailable();
        OutStatusMessage = bReady
            ? TEXT("Steam achievement sync is ready.")
            : TEXT("Steam achievement sync is not available (check settings or Steam connection).");

        UE_LOG(LogSOTS_Steam, Log, TEXT("DebugGetAchievementSyncStatus - %s"), *OutStatusMessage);
        return bReady;
    }

    OutStatusMessage = TEXT("Achievements subsystem not available.");
    UE_LOG(LogSOTS_Steam, Warning, TEXT("DebugGetAchievementSyncStatus - Achievements subsystem not available."));
    return false;
}

void USOTS_SteamDebugLibrary::DebugSubmitScore(
    const UObject* WorldContextObject,
    FName LeaderboardInternalId,
    int32 Score,
    const FString& PlayerName)
{
    if (USOTS_SteamLeaderboardsSubsystem* Subsys = GetLeaderboardsSubsystem(WorldContextObject))
    {
        Subsys->SubmitScore(LeaderboardInternalId, Score, PlayerName, true);
        return;
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("DebugSubmitScore - Leaderboards subsystem not available."));
}

void USOTS_SteamDebugLibrary::DebugRequestLeaderboardData(const UObject* WorldContextObject)
{
    if (USOTS_SteamLeaderboardsSubsystem* Subsys = GetLeaderboardsSubsystem(WorldContextObject))
    {
        Subsys->RequestLeaderboardData();
        return;
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("DebugRequestLeaderboardData - Leaderboards subsystem not available."));
}

void USOTS_SteamDebugLibrary::DebugDumpLeaderboardsToLog(const UObject* WorldContextObject)
{
    if (USOTS_SteamLeaderboardsSubsystem* Subsys = GetLeaderboardsSubsystem(WorldContextObject))
    {
        Subsys->DumpLeaderboardsToLog();
        return;
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("DebugDumpLeaderboardsToLog - Leaderboards subsystem not available."));
}

void USOTS_SteamDebugLibrary::DebugQuerySteamTop(
    const UObject* WorldContextObject,
    FName LeaderboardInternalId,
    int32 NumEntries)
{
    if (USOTS_SteamLeaderboardsSubsystem* Subsys = GetLeaderboardsSubsystem(WorldContextObject))
    {
        Subsys->QuerySteamTopEntries(LeaderboardInternalId, NumEntries);
        return;
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("DebugQuerySteamTop - Leaderboards subsystem not available."));
}

void USOTS_SteamDebugLibrary::DebugQuerySteamAroundPlayer(
    const UObject* WorldContextObject,
    FName LeaderboardInternalId,
    int32 Range)
{
    if (USOTS_SteamLeaderboardsSubsystem* Subsys = GetLeaderboardsSubsystem(WorldContextObject))
    {
        Subsys->QuerySteamAroundPlayer(LeaderboardInternalId, Range);
        return;
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("DebugQuerySteamAroundPlayer - Leaderboards subsystem not available."));
}
