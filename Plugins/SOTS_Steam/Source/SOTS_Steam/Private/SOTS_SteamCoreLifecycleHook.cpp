#include "SOTS_SteamCoreLifecycleHook.h"

#include "SOTS_SteamAchievementsSubsystem.h"
#include "SOTS_SteamCoreBridgeSettings.h"
#include "SOTS_SteamLeaderboardsSubsystem.h"
#include "SOTS_SteamMissionResultBridgeSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_SteamCoreBridge, Log, All);

bool FSOTS_SteamCoreLifecycleHook::IsBridgeEnabled() const
{
    const USOTS_SteamCoreBridgeSettings* Settings = USOTS_SteamCoreBridgeSettings::Get();
    return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
}

bool FSOTS_SteamCoreLifecycleHook::IsCoreTriggeredInitAllowed() const
{
    const USOTS_SteamCoreBridgeSettings* Settings = USOTS_SteamCoreBridgeSettings::Get();
    return Settings && Settings->bAllowCoreTriggeredSteamInit;
}

bool FSOTS_SteamCoreLifecycleHook::ShouldLogVerbose() const
{
    const USOTS_SteamCoreBridgeSettings* Settings = USOTS_SteamCoreBridgeSettings::Get();
    return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FSOTS_SteamCoreLifecycleHook::TryCoreTriggeredInit(UGameInstance* GameInstance, const TCHAR* EventName)
{
    if (!IsBridgeEnabled() || !IsCoreTriggeredInitAllowed() || !GameInstance)
    {
        return;
    }

    if (LastInitGameInstance.Get() == GameInstance && bInitAttemptedForLastGameInstance)
    {
        return;
    }

    LastInitGameInstance = GameInstance;
    bInitAttemptedForLastGameInstance = true;

    int32 RefreshedCount = 0;

    if (USOTS_SteamAchievementsSubsystem* Achievements = GameInstance->GetSubsystem<USOTS_SteamAchievementsSubsystem>())
    {
        Achievements->ForceRefreshSteamAvailability();
        ++RefreshedCount;
    }

    if (USOTS_SteamLeaderboardsSubsystem* Leaderboards = GameInstance->GetSubsystem<USOTS_SteamLeaderboardsSubsystem>())
    {
        Leaderboards->ForceRefreshSteamAvailability();
        ++RefreshedCount;
    }

    if (USOTS_SteamMissionResultBridgeSubsystem* MissionBridge = GameInstance->GetSubsystem<USOTS_SteamMissionResultBridgeSubsystem>())
    {
        MissionBridge->ForceRefreshSteamAvailability();
        ++RefreshedCount;
    }

    if (ShouldLogVerbose())
    {
        UE_LOG(
            LogSOTS_SteamCoreBridge,
            Verbose,
            TEXT("SOTS_Steam CoreBridge: %s refreshed availability for %d Steam subsystems (GameInstance=%s)"),
            EventName,
            RefreshedCount,
            *GetNameSafe(GameInstance));
    }
}

void FSOTS_SteamCoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
    if (!IsBridgeEnabled() || !World)
    {
        return;
    }

    TryCoreTriggeredInit(World->GetGameInstance(), TEXT("OnSOTS_WorldStartPlay"));
}

void FSOTS_SteamCoreLifecycleHook::OnSOTS_PlayerControllerBeginPlay(APlayerController* PC)
{
    if (!IsBridgeEnabled() || !PC)
    {
        return;
    }

    TryCoreTriggeredInit(PC->GetGameInstance(), TEXT("OnSOTS_PlayerControllerBeginPlay"));
}

void FSOTS_SteamCoreLifecycleHook::Shutdown()
{
    LastInitGameInstance.Reset();
    bInitAttemptedForLastGameInstance = false;
}
