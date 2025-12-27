#include "SOTS_KEMCoreBridgeSettings.h"

USOTS_KEMCoreBridgeSettings::USOTS_KEMCoreBridgeSettings()
{
    CategoryName = TEXT("SOTS");
    SectionName = TEXT("SOTS KillExecutionManager Core Bridge");

    bEnableSOTSCoreSaveParticipantBridge = false;
    bEnableSOTSCoreBridgeVerboseLogs = false;
}
