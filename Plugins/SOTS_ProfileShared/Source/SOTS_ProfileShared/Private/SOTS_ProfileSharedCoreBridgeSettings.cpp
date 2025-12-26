#include "SOTS_ProfileSharedCoreBridgeSettings.h"

USOTS_ProfileSharedCoreBridgeSettings::USOTS_ProfileSharedCoreBridgeSettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("SOTS ProfileShared Core Bridge");
    bEnableSOTSCoreLifecycleBridge = false;
    bEnableSOTSCoreBridgeVerboseLogs = false;
}
