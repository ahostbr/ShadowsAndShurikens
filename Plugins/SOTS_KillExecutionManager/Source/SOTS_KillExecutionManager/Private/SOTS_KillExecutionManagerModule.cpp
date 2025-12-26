
#include "SOTS_KillExecutionManagerModule.h"

#include "SOTS_KEMCoreBridgeSettings.h"
#include "SOTS_KEMCoreSaveParticipant.h"

#include "Save/SOTS_CoreSaveParticipantRegistry.h"

#include "Modules/ModuleManager.h"

#include "Templates/UniquePtr.h"

DEFINE_LOG_CATEGORY(LogSOTSKEM);
IMPLEMENT_MODULE(FSOTS_KillExecutionManagerModule, SOTS_KillExecutionManager)

namespace SOTS::KEM::CoreBridge
{
    const USOTS_KEMCoreBridgeSettings* GetSettings()
    {
        return GetDefault<USOTS_KEMCoreBridgeSettings>();
    }

    bool IsBridgeEnabled(const USOTS_KEMCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreSaveParticipantBridge;
    }

    TUniquePtr<FKEM_SaveParticipant> GSaveParticipant;

    void RegisterSaveParticipantIfEnabled()
    {
        const USOTS_KEMCoreBridgeSettings* Settings = GetSettings();
        if (!IsBridgeEnabled(Settings))
        {
            return;
        }

        if (!GSaveParticipant)
        {
            GSaveParticipant = MakeUnique<FKEM_SaveParticipant>();
        }

        FSOTS_CoreSaveParticipantRegistry::RegisterSaveParticipant(GSaveParticipant.Get());

        if (Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTSKEM, Verbose, TEXT("KEM CoreBridge: Registered SOTS.CoreSaveParticipant id=%s"), *GSaveParticipant->GetParticipantId().ToString());
        }
    }

    void UnregisterSaveParticipantIfRegistered()
    {
        if (!GSaveParticipant)
        {
            return;
        }

        const USOTS_KEMCoreBridgeSettings* Settings = GetSettings();
        FSOTS_CoreSaveParticipantRegistry::UnregisterSaveParticipant(GSaveParticipant.Get());

        if (Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTSKEM, Verbose, TEXT("KEM CoreBridge: Unregistered SOTS.CoreSaveParticipant id=%s"), *GSaveParticipant->GetParticipantId().ToString());
        }
    }
}

void FSOTS_KillExecutionManagerModule::StartupModule()
{
    UE_LOG(LogSOTSKEM, Log, TEXT("SOTS KillExecutionManager v0.40.2 module starting up"));

    SOTS::KEM::CoreBridge::RegisterSaveParticipantIfEnabled();
}

void FSOTS_KillExecutionManagerModule::ShutdownModule()
{
    SOTS::KEM::CoreBridge::UnregisterSaveParticipantIfRegistered();

    UE_LOG(LogSOTSKEM, Log, TEXT("SOTS KillExecutionManager v0.40.2 module shutting down"));
}
