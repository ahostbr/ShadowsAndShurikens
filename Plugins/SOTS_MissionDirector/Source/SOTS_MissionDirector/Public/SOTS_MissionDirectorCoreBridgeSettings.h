#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_MissionDirectorCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS MissionDirector - Core Bridge"))
class SOTS_MISSIONDIRECTOR_API USOTS_MissionDirectorCoreBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const USOTS_MissionDirectorCoreBridgeSettings* Get();

    UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
    bool bEnableSOTSCoreLifecycleBridge = false;

    UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
    bool bEnableSOTSCoreBridgeVerboseLogs = false;
};
