#include "SOTS_SteamAchievementRegistry.h"
#include "SOTS_SteamLog.h"

const FSOTS_SteamAchievementDefinition* USOTS_SteamAchievementRegistry::FindByInternalId(FName InternalId) const
{
    if (!InternalId.IsNone())
    {
        for (const FSOTS_SteamAchievementDefinition& Def : Achievements)
        {
            if (Def.InternalId == InternalId)
            {
                return &Def;
            }
        }
    }

    UE_LOG(LogSOTS_Steam, Warning, TEXT("USOTS_SteamAchievementRegistry::FindByInternalId - No achievement found for InternalId '%s'."), *InternalId.ToString());
    return nullptr;
}
