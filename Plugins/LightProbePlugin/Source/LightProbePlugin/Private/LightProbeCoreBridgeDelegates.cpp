#include "LightProbeCoreBridgeDelegates.h"

namespace LightProbePlugin::CoreBridge
{
    static FLightProbe_OnCoreWorldReady_Native GOnCoreWorldReady;
    static FLightProbe_OnCorePrimaryPlayerReady_Native GOnCorePrimaryPlayerReady;
    static FLightProbe_OnCorePreLoadMap_Native GOnCorePreLoadMap;
    static FLightProbe_OnCorePostLoadMap_Native GOnCorePostLoadMap;

    FLightProbe_OnCoreWorldReady_Native& OnCoreWorldReady_Native()
    {
        return GOnCoreWorldReady;
    }

    FLightProbe_OnCorePrimaryPlayerReady_Native& OnCorePrimaryPlayerReady_Native()
    {
        return GOnCorePrimaryPlayerReady;
    }

    FLightProbe_OnCorePreLoadMap_Native& OnCorePreLoadMap_Native()
    {
        return GOnCorePreLoadMap;
    }

    FLightProbe_OnCorePostLoadMap_Native& OnCorePostLoadMap_Native()
    {
        return GOnCorePostLoadMap;
    }
}
