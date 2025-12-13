#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SOTS_UDSBridgeSettings.generated.h"

class USOTS_UDSBridgeConfig;

/**
 * Project-level defaults for the UDS bridge.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "SOTS UDS Bridge"))
class SOTS_UDSBRIDGE_API USOTS_UDSBridgeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USOTS_UDSBridgeSettings();

	// Default config asset applied when the subsystem spins up.
	UPROPERTY(Config, EditAnywhere, Category = "UDS Bridge")
	TSoftObjectPtr<USOTS_UDSBridgeConfig> DefaultBridgeConfig;
};
