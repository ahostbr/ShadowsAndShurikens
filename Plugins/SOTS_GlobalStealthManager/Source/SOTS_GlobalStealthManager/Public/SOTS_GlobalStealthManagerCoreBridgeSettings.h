#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_GlobalStealthManagerCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS GlobalStealthManager - Core Bridge"))
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_GlobalStealthManagerCoreBridgeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const USOTS_GlobalStealthManagerCoreBridgeSettings* Get();
	virtual FName GetCategoryName() const override { return TEXT("SOTS"); }

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreLifecycleBridge = false;

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreBridgeVerboseLogs = false;
};
