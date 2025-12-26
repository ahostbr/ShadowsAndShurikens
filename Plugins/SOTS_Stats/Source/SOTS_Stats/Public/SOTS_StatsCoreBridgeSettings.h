#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SOTS_StatsCoreBridgeSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SOTS Stats Core Bridge"))
class SOTS_STATS_API USOTS_StatsCoreBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USOTS_StatsCoreBridgeSettings();

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreSaveParticipantBridge;

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreBridgeVerboseLogs;
};
