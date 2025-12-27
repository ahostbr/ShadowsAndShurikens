#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_AIPerceptionCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS AIPerception - Core Bridge"))
class SOTS_AIPERCEPTION_API USOTS_AIPerceptionCoreBridgeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const USOTS_AIPerceptionCoreBridgeSettings* Get();
	virtual FName GetCategoryName() const override { return TEXT("SOTS"); }

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreLifecycleBridge = false;

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreBridgeVerboseLogs = false;
};
