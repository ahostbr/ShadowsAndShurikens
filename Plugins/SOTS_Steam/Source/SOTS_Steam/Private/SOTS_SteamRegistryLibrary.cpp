#include "SOTS_SteamRegistryLibrary.h"
#include "SOTS_SteamLog.h"

bool USOTS_SteamRegistryLibrary::FindAchievementDefinitionById(
    const USOTS_SteamAchievementRegistry* Registry,
    FName InternalId,
    FSOTS_SteamAchievementDefinition& OutDefinition)
{
    if (!Registry)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("FindAchievementDefinitionById - Registry is null."));
        OutDefinition = FSOTS_SteamAchievementDefinition();
        return false;
    }

    if (InternalId.IsNone())
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("FindAchievementDefinitionById - InternalId is None."));
        OutDefinition = FSOTS_SteamAchievementDefinition();
        return false;
    }

    const FSOTS_SteamAchievementDefinition* Def = Registry->FindByInternalId(InternalId);
    if (!Def)
    {
        OutDefinition = FSOTS_SteamAchievementDefinition();
        return false;
    }

    OutDefinition = *Def;
    return true;
}

bool USOTS_SteamRegistryLibrary::FindLeaderboardDefinitionById(
    const USOTS_SteamLeaderboardRegistry* Registry,
    FName InternalId,
    FSOTS_SteamLeaderboardDefinition& OutDefinition)
{
    if (!Registry)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("FindLeaderboardDefinitionById - Registry is null."));
        OutDefinition = FSOTS_SteamLeaderboardDefinition();
        return false;
    }

    if (InternalId.IsNone())
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("FindLeaderboardDefinitionById - InternalId is None."));
        OutDefinition = FSOTS_SteamLeaderboardDefinition();
        return false;
    }

    const FSOTS_SteamLeaderboardDefinition* Def = Registry->FindByInternalId(InternalId);
    if (!Def)
    {
        OutDefinition = FSOTS_SteamLeaderboardDefinition();
        return false;
    }

    OutDefinition = *Def;
    return true;
}
