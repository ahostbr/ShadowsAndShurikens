#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SOTS_MMSSCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS_MMSS - Core Bridge"))
class SOTS_MMSS_API USOTS_MMSSCoreBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const USOTS_MMSSCoreBridgeSettings* Get();

    UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
    bool bEnableSOTSCoreLifecycleBridge = false;

    UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
    bool bEnableSOTSCoreBridgeVerboseLogs = false;
};
