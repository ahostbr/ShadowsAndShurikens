#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SOTS_DebugCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS Debug - Core Bridge"))
class SOTS_DEBUG_API USOTS_DebugCoreBridgeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const USOTS_DebugCoreBridgeSettings* Get();

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreLifecycleBridge = false;

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreBridgeVerboseLogs = false;
};
