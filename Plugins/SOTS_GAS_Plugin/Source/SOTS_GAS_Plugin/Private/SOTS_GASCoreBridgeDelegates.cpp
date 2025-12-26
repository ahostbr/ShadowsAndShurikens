#include "SOTS_GASCoreBridgeDelegates.h"

namespace SOTS_GAS::CoreBridge
{
	FSOTS_GAS_OnCoreWorldReady_Native& OnCoreWorldReady_Native()
	{
		static FSOTS_GAS_OnCoreWorldReady_Native Delegate;
		return Delegate;
	}

	FSOTS_GAS_OnCorePrimaryPlayerReady_Native& OnCorePrimaryPlayerReady_Native()
	{
		static FSOTS_GAS_OnCorePrimaryPlayerReady_Native Delegate;
		return Delegate;
	}

	FSOTS_GAS_OnCorePreLoadMap_Native& OnCorePreLoadMap_Native()
	{
		static FSOTS_GAS_OnCorePreLoadMap_Native Delegate;
		return Delegate;
	}

	FSOTS_GAS_OnCorePostLoadMap_Native& OnCorePostLoadMap_Native()
	{
		static FSOTS_GAS_OnCorePostLoadMap_Native Delegate;
		return Delegate;
	}
}
