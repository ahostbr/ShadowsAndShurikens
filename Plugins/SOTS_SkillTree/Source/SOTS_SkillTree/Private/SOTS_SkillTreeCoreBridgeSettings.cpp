#include "SOTS_SkillTreeCoreBridgeSettings.h"

USOTS_SkillTreeCoreBridgeSettings::USOTS_SkillTreeCoreBridgeSettings()
{
    CategoryName = TEXT("SOTS");
    SectionName = TEXT("SOTS SkillTree Core Bridge");

    bEnableSOTSCoreSaveParticipantBridge = false;
    bEnableSOTSCoreBridgeVerboseLogs = false;
}
