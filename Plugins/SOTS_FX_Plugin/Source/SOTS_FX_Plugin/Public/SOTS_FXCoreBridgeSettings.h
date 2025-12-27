#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_FXCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS FX - Core Bridge"))
class SOTS_FX_PLUGIN_API USOTS_FXCoreBridgeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const USOTS_FXCoreBridgeSettings* Get();
	virtual FName GetCategoryName() const override { return TEXT("SOTS"); }

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreLifecycleBridge = false;

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreBridgeVerboseLogs = false;
};
