// SOTS_INVModule.cpp
// Minimal runtime module for the SOTS_INV plugin.

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

#include "SOTS_INVCoreBridgeSettings.h"
#include "SOTS_INVCoreSaveParticipant.h"

#include "Save/SOTS_CoreSaveParticipantRegistry.h"

#include "Templates/UniquePtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_INVModule, Log, All);

/**
 * All real logic for inventory bridging lives in USOTS_InventoryBridgeSubsystem
 * and related components. This module just needs to exist so the engine can
 * load the plugin cleanly.
 */
class FSOTS_INVModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogSOTS_INVModule, Log, TEXT("SOTS_INV module starting up"));
        // IMPORTANT: Do NOT touch worlds, GameInstance, or assets here.

        RegisterSaveParticipantIfEnabled();
    }

    virtual void ShutdownModule() override
    {
        UnregisterSaveParticipantIfRegistered();
        UE_LOG(LogSOTS_INVModule, Log, TEXT("SOTS_INV module shutting down"));
    }

private:
    void RegisterSaveParticipantIfEnabled()
    {
        const USOTS_INVCoreBridgeSettings* Settings = GetDefault<USOTS_INVCoreBridgeSettings>();
        if (!Settings || !Settings->bEnableSOTSCoreSaveParticipantBridge)
        {
            return;
        }

        if (!SaveParticipant)
        {
            SaveParticipant = MakeUnique<FINV_SaveParticipant>();
        }

        FSOTS_CoreSaveParticipantRegistry::RegisterSaveParticipant(SaveParticipant.Get());

        if (Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTS_INVModule, Verbose, TEXT("INV CoreBridge: Registered SOTS.CoreSaveParticipant id=%s"), *SaveParticipant->GetParticipantId().ToString());
        }
    }

    void UnregisterSaveParticipantIfRegistered()
    {
        if (!SaveParticipant)
        {
            return;
        }

        const USOTS_INVCoreBridgeSettings* Settings = GetDefault<USOTS_INVCoreBridgeSettings>();
        FSOTS_CoreSaveParticipantRegistry::UnregisterSaveParticipant(SaveParticipant.Get());

        if (Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTS_INVModule, Verbose, TEXT("INV CoreBridge: Unregistered SOTS.CoreSaveParticipant id=%s"), *SaveParticipant->GetParticipantId().ToString());
        }
    }

    TUniquePtr<FINV_SaveParticipant> SaveParticipant;
};

// The module name MUST match the .uplugin Modules entry → "SOTS_INV"
IMPLEMENT_MODULE(FSOTS_INVModule, SOTS_INV);
