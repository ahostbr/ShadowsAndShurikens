#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SOTS_SteamCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS Steam - Core Bridge"))
class SOTS_STEAM_API USOTS_SteamCoreBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const USOTS_SteamCoreBridgeSettings* Get();
    virtual FName GetCategoryName() const override { return TEXT("SOTS"); }

    // Master gate for BRIDGE15 (SOTS_Core lifecycle bridge).
    UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
    bool bEnableSOTSCoreLifecycleBridge = false;

    // Second explicit gate: even if the bridge is enabled, do not trigger any Steam init behavior
    // unless this is also true.
    UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
    bool bAllowCoreTriggeredSteamInit = false;

    UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
    bool bEnableSOTSCoreBridgeVerboseLogs = false;
};
