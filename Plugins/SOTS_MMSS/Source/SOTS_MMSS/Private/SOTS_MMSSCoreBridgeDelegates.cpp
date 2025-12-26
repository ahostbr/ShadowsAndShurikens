#include "SOTS_MMSSCoreBridgeDelegates.h"

namespace SOTS_MMSS::CoreBridge
{
    FSOTS_MMSS_OnCoreWorldReady_Native& OnCoreWorldReady_Native()
    {
        static FSOTS_MMSS_OnCoreWorldReady_Native Delegate;
        return Delegate;
    }

    FSOTS_MMSS_OnCorePrimaryPlayerReady_Native& OnCorePrimaryPlayerReady_Native()
    {
        static FSOTS_MMSS_OnCorePrimaryPlayerReady_Native Delegate;
        return Delegate;
    }

    FSOTS_MMSS_OnCorePreLoadMap_Native& OnCorePreLoadMap_Native()
    {
        static FSOTS_MMSS_OnCorePreLoadMap_Native Delegate;
        return Delegate;
    }

    FSOTS_MMSS_OnCorePostLoadMap_Native& OnCorePostLoadMap_Native()
    {
        static FSOTS_MMSS_OnCorePostLoadMap_Native Delegate;
        return Delegate;
    }
}
