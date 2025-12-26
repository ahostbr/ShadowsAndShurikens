#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "OmniTraceCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="OmniTrace - Core Bridge"))
class OMNITRACE_API UOmniTraceCoreBridgeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UOmniTraceCoreBridgeSettings* Get();

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreLifecycleBridge = false;

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreBridgeVerboseLogs = false;
};
