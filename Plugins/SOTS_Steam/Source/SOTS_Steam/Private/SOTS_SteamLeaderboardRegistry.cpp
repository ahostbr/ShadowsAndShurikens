#include "SOTS_SteamLeaderboardRegistry.h"
#include "SOTS_SteamLog.h"

const FSOTS_SteamLeaderboardDefinition* USOTS_SteamLeaderboardRegistry::FindByInternalId(FName InternalId) const
{
    if (!InternalId.IsNone())
    {
        for (const FSOTS_SteamLeaderboardDefinition& Def : Leaderboards)
        {
            if (Def.InternalId == InternalId)
            {
                return &Def;
            }
        }
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("USOTS_SteamLeaderboardRegistry::FindByInternalId - No leaderboard found for InternalId '%s'."), *InternalId.ToString());
    return nullptr;
}
