#pragma once

#include "CoreMinimal.h"

class UWorld;
class APlayerController;
class APawn;

DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_GAS_OnCoreWorldReady_Native, UWorld*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTS_GAS_OnCorePrimaryPlayerReady_Native, APlayerController*, APawn*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_GAS_OnCorePreLoadMap_Native, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_GAS_OnCorePostLoadMap_Native, UWorld*);

namespace SOTS_GAS::CoreBridge
{
	SOTS_GAS_PLUGIN_API FSOTS_GAS_OnCoreWorldReady_Native& OnCoreWorldReady_Native();
	SOTS_GAS_PLUGIN_API FSOTS_GAS_OnCorePrimaryPlayerReady_Native& OnCorePrimaryPlayerReady_Native();
	SOTS_GAS_PLUGIN_API FSOTS_GAS_OnCorePreLoadMap_Native& OnCorePreLoadMap_Native();
	SOTS_GAS_PLUGIN_API FSOTS_GAS_OnCorePostLoadMap_Native& OnCorePostLoadMap_Native();
}
