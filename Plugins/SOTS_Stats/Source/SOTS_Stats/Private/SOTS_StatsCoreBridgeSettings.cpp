#include "SOTS_StatsCoreBridgeSettings.h"

USOTS_StatsCoreBridgeSettings::USOTS_StatsCoreBridgeSettings()
{
    CategoryName = TEXT("SOTS");
    SectionName = TEXT("SOTS Stats Core Bridge");

    bEnableSOTSCoreSaveParticipantBridge = false;
    bEnableSOTSCoreBridgeVerboseLogs = false;
}
