#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "LightProbeCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="LightProbe - Core Bridge"))
class LIGHTPROBEPLUGIN_API ULightProbeCoreBridgeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const ULightProbeCoreBridgeSettings* Get();

    UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
    bool bEnableSOTSCoreLifecycleBridge = false;

    UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
    bool bEnableSOTSCoreBridgeVerboseLogs = false;
};
