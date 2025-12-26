#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SOTS_KEMCoreBridgeSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SOTS KillExecutionManager Core Bridge"))
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEMCoreBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USOTS_KEMCoreBridgeSettings();

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreSaveParticipantBridge;

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreBridgeVerboseLogs;
};
