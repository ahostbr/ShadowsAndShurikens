#include "SOTS_SteamSettings.h"

USOTS_SteamSettings::USOTS_SteamSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bEnableSteamIntegration = false;
    bUseSteamForAchievements = true;
    bUseSteamForLeaderboards = true;
    bEnableVerboseLogging = false;
    DebugProfileId = TEXT("Debug");
}

FName USOTS_SteamSettings::GetCategoryName() const
{
    return FName(TEXT("SOTS"));
}

FText USOTS_SteamSettings::GetSectionText() const
{
    return NSLOCTEXT("SOTS_Steam", "SettingsSection", "Steam Integration");
}

const USOTS_SteamSettings* USOTS_SteamSettings::Get()
{
    return GetDefault<USOTS_SteamSettings>();
}

bool USOTS_SteamSettings::IsVerboseLoggingEnabled()
{
    if (const USOTS_SteamSettings* Settings = Get())
    {
        return Settings->bEnableVerboseLogging;
    }

    return false;
}
