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

	// Master enable for the UDS bridge. When false, the subsystem becomes inert.
	UPROPERTY(Config, EditAnywhere, Category = "UDS Bridge|Runtime")
	bool bEnableUDSBridge = true;

	// Polling cadence used for reflection-based DLWE state reads.
	UPROPERTY(Config, EditAnywhere, Category = "UDS Bridge|Runtime", meta=(ClampMin="0.05", ClampMax="5.0"))
	float PollIntervalSeconds = 0.35f;

	// Reflection class name for the DLWE interaction component (no hard dependency on UDS types).
	UPROPERTY(Config, EditAnywhere, Category = "UDS Bridge|Runtime")
	FName DLWEComponentClassName = FName(TEXT("DLWE_Interaction"));

	// Log a single warning if UDS/DLWE are absent when enabled.
	UPROPERTY(Config, EditAnywhere, Category = "UDS Bridge|Runtime")
	bool bLogOnceWhenUDSAbsent = false;

	// Default config asset applied when the subsystem spins up.
	UPROPERTY(Config, EditAnywhere, Category = "UDS Bridge")
	TSoftObjectPtr<USOTS_UDSBridgeConfig> DefaultBridgeConfig;
};
