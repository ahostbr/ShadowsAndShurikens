#include "SOTS_InputCoreBridgeSettings.h"

USOTS_InputCoreBridgeSettings::USOTS_InputCoreBridgeSettings()
{
    CategoryName = TEXT("SOTS");
    SectionName = TEXT("SOTS Input Core Bridge");
    bEnableSOTSCoreLifecycleBridge = false;
    bEnableSOTSCoreBridgeVerboseLogs = false;
}
