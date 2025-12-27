#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_GASCoreBridgeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SOTS GAS - Core Bridge"))
class SOTS_GAS_PLUGIN_API USOTS_GASCoreBridgeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const USOTS_GASCoreBridgeSettings* Get();
	virtual FName GetCategoryName() const override { return TEXT("SOTS"); }

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreLifecycleBridge = false;

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreBridgeVerboseLogs = false;

	UPROPERTY(Config, EditAnywhere, Category="SOTS|Core Bridge")
	bool bEnableSOTSCoreSaveParticipantBridge = false;
};
