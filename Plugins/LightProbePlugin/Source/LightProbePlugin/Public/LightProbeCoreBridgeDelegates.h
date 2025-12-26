#pragma once

#include "CoreMinimal.h"

class UWorld;
class APlayerController;
class APawn;

DECLARE_MULTICAST_DELEGATE_OneParam(FLightProbe_OnCoreWorldReady_Native, UWorld*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FLightProbe_OnCorePrimaryPlayerReady_Native, APlayerController*, APawn*);
DECLARE_MULTICAST_DELEGATE_OneParam(FLightProbe_OnCorePreLoadMap_Native, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FLightProbe_OnCorePostLoadMap_Native, UWorld*);

namespace LightProbePlugin::CoreBridge
{
    LIGHTPROBEPLUGIN_API FLightProbe_OnCoreWorldReady_Native& OnCoreWorldReady_Native();
    LIGHTPROBEPLUGIN_API FLightProbe_OnCorePrimaryPlayerReady_Native& OnCorePrimaryPlayerReady_Native();
    LIGHTPROBEPLUGIN_API FLightProbe_OnCorePreLoadMap_Native& OnCorePreLoadMap_Native();
    LIGHTPROBEPLUGIN_API FLightProbe_OnCorePostLoadMap_Native& OnCorePostLoadMap_Native();
}
