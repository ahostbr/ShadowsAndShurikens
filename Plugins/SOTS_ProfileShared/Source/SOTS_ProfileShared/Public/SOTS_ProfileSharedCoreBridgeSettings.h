#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SOTS_ProfileSharedCoreBridgeSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SOTS ProfileShared Core Bridge"))
class SOTS_PROFILESHARED_API USOTS_ProfileSharedCoreBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USOTS_ProfileSharedCoreBridgeSettings();

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreLifecycleBridge;

    UPROPERTY(EditAnywhere, Config, Category = "Core Bridge")
    bool bEnableSOTSCoreBridgeVerboseLogs;
};
