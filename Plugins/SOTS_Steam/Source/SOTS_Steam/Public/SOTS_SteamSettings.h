#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_SteamAchievementRegistry.h"
#include "SOTS_SteamLeaderboardRegistry.h"
#include "UObject/SoftObjectPtr.h"
#include "SOTS_SteamSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS Steam Settings"))
class SOTS_STEAM_API USOTS_SteamSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USOTS_SteamSettings(const FObjectInitializer& ObjectInitializer);

    virtual FName GetCategoryName() const override;
    virtual FText GetSectionText() const override;

    static const USOTS_SteamSettings* Get();
    static bool IsVerboseLoggingEnabled();

public:
    UPROPERTY(EditAnywhere, Config, Category="Achievements", meta=(AllowedClasses="/Script/SOTS_Steam.SOTS_SteamAchievementRegistry", ToolTip="Default registry asset that describes achievement definitions and their Steam mappings."))
    TSoftObjectPtr<USOTS_SteamAchievementRegistry> DefaultAchievementRegistry;

    UPROPERTY(EditAnywhere, Config, Category="Achievements", meta=(ToolTip="Enable local achievement tracking within the SOTS_Steam subsystem."))
    bool bEnableAchievements = true;

    UPROPERTY(EditAnywhere, Config, Category="Leaderboards", meta=(AllowedClasses="/Script/SOTS_Steam.SOTS_SteamLeaderboardRegistry", ToolTip="Default registry asset describing leaderboard definitions."))
    TSoftObjectPtr<USOTS_SteamLeaderboardRegistry> DefaultLeaderboardRegistry;

    UPROPERTY(EditAnywhere, Config, Category="Leaderboards", meta=(ToolTip="Enable local leaderboard tracking within the SOTS_Steam subsystem."))
    bool bEnableLeaderboards = true;

    UPROPERTY(EditAnywhere, Config, Category="Steam", meta=(ToolTip="If true, SOTS_Steam will talk to OnlineSubsystemSteam (when available) to mirror progress."))
    bool bEnableSteamIntegration = false;

    UPROPERTY(EditAnywhere, Config, Category="Steam", meta=(ToolTip="Use Steam (via OnlineSubsystemSteam) when updating achievements."))
    bool bUseSteamForAchievements = true;

    UPROPERTY(EditAnywhere, Config, Category="Steam", meta=(ToolTip="Use Steam (via OnlineSubsystemSteam) when updating leaderboard scores."))
    bool bUseSteamForLeaderboards = true;

    UPROPERTY(EditAnywhere, Config, Category="Steam", meta=(ToolTip="Treat missing Steam integration (e.g., OnlineSubsystemSteam not loaded) as a warning instead of a hard error."))
    bool bTreatMissingSteamAsWarningNotError = true;

    UPROPERTY(EditAnywhere, Config, Category="Steam", meta=(ToolTip="AppId reported to Steam (used by OnlineSubsystemSteam)."))
    int32 SteamAppId = 0;

    UPROPERTY(EditAnywhere, Config, Category="Debug", meta=(ToolTip="Print extra log detail about achievement and leaderboard state."))
    bool bEnableVerboseLogging = false;

    UPROPERTY(EditAnywhere, Config, Category="Debug", meta=(ToolTip="Profile ID used when debugging (e.g., to keep saves isolated)."))
    FString DebugProfileId;
};
