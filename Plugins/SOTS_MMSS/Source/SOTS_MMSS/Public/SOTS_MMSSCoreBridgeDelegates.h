#pragma once

#include "CoreMinimal.h"

class UWorld;
class APlayerController;
class APawn;

DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_MMSS_OnCoreWorldReady_Native, UWorld*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTS_MMSS_OnCorePrimaryPlayerReady_Native, APlayerController*, APawn*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_MMSS_OnCorePreLoadMap_Native, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_MMSS_OnCorePostLoadMap_Native, UWorld*);

namespace SOTS_MMSS::CoreBridge
{
    SOTS_MMSS_API FSOTS_MMSS_OnCoreWorldReady_Native& OnCoreWorldReady_Native();
    SOTS_MMSS_API FSOTS_MMSS_OnCorePrimaryPlayerReady_Native& OnCorePrimaryPlayerReady_Native();
    SOTS_MMSS_API FSOTS_MMSS_OnCorePreLoadMap_Native& OnCorePreLoadMap_Native();
    SOTS_MMSS_API FSOTS_MMSS_OnCorePostLoadMap_Native& OnCorePostLoadMap_Native();
}
