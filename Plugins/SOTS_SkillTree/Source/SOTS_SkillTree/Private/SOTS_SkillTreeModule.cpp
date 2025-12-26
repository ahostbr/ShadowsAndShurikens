#include "SOTS_SkillTreeModule.h"

#include "SOTS_SkillTreeCoreBridgeSettings.h"
#include "SOTS_SkillTreeCoreSaveParticipant.h"

#include "Save/SOTS_CoreSaveParticipantRegistry.h"

#include "Modules/ModuleManager.h"
#include "Templates/UniquePtr.h"

DEFINE_LOG_CATEGORY(LogSOTSSkillTree);

IMPLEMENT_MODULE(FSOTS_SkillTreeModule, SOTS_SkillTree)

namespace SOTS::SkillTree::CoreBridge
{
    const USOTS_SkillTreeCoreBridgeSettings* GetSettings()
    {
        return GetDefault<USOTS_SkillTreeCoreBridgeSettings>();
    }

    bool IsBridgeEnabled(const USOTS_SkillTreeCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreSaveParticipantBridge;
    }

    TUniquePtr<FSkillTree_SaveParticipant> GSaveParticipant;

    void RegisterSaveParticipantIfEnabled()
    {
        const USOTS_SkillTreeCoreBridgeSettings* Settings = GetSettings();
        if (!IsBridgeEnabled(Settings))
        {
            return;
        }

        if (!GSaveParticipant)
        {
            GSaveParticipant = MakeUnique<FSkillTree_SaveParticipant>();
        }

        FSOTS_CoreSaveParticipantRegistry::RegisterSaveParticipant(GSaveParticipant.Get());

        if (Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTSSkillTree, Verbose, TEXT("SkillTree CoreBridge: Registered SOTS.CoreSaveParticipant id=%s"), *GSaveParticipant->GetParticipantId().ToString());
        }
    }

    void UnregisterSaveParticipantIfRegistered()
    {
        if (!GSaveParticipant)
        {
            return;
        }

        const USOTS_SkillTreeCoreBridgeSettings* Settings = GetSettings();
        FSOTS_CoreSaveParticipantRegistry::UnregisterSaveParticipant(GSaveParticipant.Get());

        if (Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTSSkillTree, Verbose, TEXT("SkillTree CoreBridge: Unregistered SOTS.CoreSaveParticipant id=%s"), *GSaveParticipant->GetParticipantId().ToString());
        }
    }
}

void FSOTS_SkillTreeModule::StartupModule()
{
    UE_LOG(LogSOTSSkillTree, Log, TEXT("SOTS_SkillTree module starting up"));

    SOTS::SkillTree::CoreBridge::RegisterSaveParticipantIfEnabled();
}

void FSOTS_SkillTreeModule::ShutdownModule()
{
    SOTS::SkillTree::CoreBridge::UnregisterSaveParticipantIfRegistered();

    UE_LOG(LogSOTSSkillTree, Log, TEXT("SOTS_SkillTree module shutting down"));
}