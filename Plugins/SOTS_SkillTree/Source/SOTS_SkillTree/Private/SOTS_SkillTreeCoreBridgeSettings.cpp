#include "SOTS_SkillTreeCoreBridgeSettings.h"

USOTS_SkillTreeCoreBridgeSettings::USOTS_SkillTreeCoreBridgeSettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("SOTS SkillTree Core Bridge");

    bEnableSOTSCoreSaveParticipantBridge = false;
    bEnableSOTSCoreBridgeVerboseLogs = false;
}
