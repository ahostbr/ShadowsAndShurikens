#include "SOTS_GAS_Plugin.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"
#include "SOTS_GASCoreBridgeSettings.h"
#include "SOTS_GASCoreLifecycleHook.h"
#include "SOTS_GASCoreSaveParticipant.h"

#include "Save/SOTS_CoreSaveParticipantRegistry.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTSGAS);
IMPLEMENT_MODULE(FSOTS_GAS_PluginModule, SOTS_GAS_Plugin);

namespace
{
    TUniquePtr<FGAS_CoreLifecycleHook> GGASCoreLifecycleHook;
    TUniquePtr<FSOTS_CoreLifecycleListenerHandle> GGASCoreLifecycleHandle;
    TUniquePtr<FGAS_SaveParticipant> GGASSaveParticipant;
}

namespace SOTS::GAS::CoreBridge
{
    const USOTS_GASCoreBridgeSettings* GetSettings()
    {
        return USOTS_GASCoreBridgeSettings::Get();
    }

    void RegisterSaveParticipantIfEnabled()
    {
        const USOTS_GASCoreBridgeSettings* Settings = GetSettings();
        if (!Settings || !Settings->bEnableSOTSCoreSaveParticipantBridge)
        {
            return;
        }

        if (!GGASSaveParticipant)
        {
            GGASSaveParticipant = MakeUnique<FGAS_SaveParticipant>();
        }

        FSOTS_CoreSaveParticipantRegistry::RegisterSaveParticipant(GGASSaveParticipant.Get());

        if (Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTSGAS, Verbose, TEXT("GAS CoreBridge: Registered SOTS.CoreSaveParticipant id=%s"), *GGASSaveParticipant->GetParticipantId().ToString());
        }
    }

    void UnregisterSaveParticipantIfRegistered()
    {
        if (!GGASSaveParticipant)
        {
            return;
        }

        const USOTS_GASCoreBridgeSettings* Settings = GetSettings();
        FSOTS_CoreSaveParticipantRegistry::UnregisterSaveParticipant(GGASSaveParticipant.Get());

        if (Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTSGAS, Verbose, TEXT("GAS CoreBridge: Unregistered SOTS.CoreSaveParticipant id=%s"), *GGASSaveParticipant->GetParticipantId().ToString());
        }
    }
}

void FSOTS_GAS_PluginModule::StartupModule()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogSOTSGAS, Log, TEXT("SOTS_GAS_Plugin module starting up."));
#endif

    const USOTS_GASCoreBridgeSettings* Settings = USOTS_GASCoreBridgeSettings::Get();
    if (Settings && Settings->bEnableSOTSCoreLifecycleBridge)
    {
        GGASCoreLifecycleHook = MakeUnique<FGAS_CoreLifecycleHook>();
        GGASCoreLifecycleHandle = MakeUnique<FSOTS_CoreLifecycleListenerHandle>(GGASCoreLifecycleHook.Get());

        if (Settings->bEnableSOTSCoreBridgeVerboseLogs)
        {
            UE_LOG(LogSOTSGAS, Verbose, TEXT("GAS CoreBridge: Registered Core lifecycle listener."));
        }
    }

    SOTS::GAS::CoreBridge::RegisterSaveParticipantIfEnabled();
}

void FSOTS_GAS_PluginModule::ShutdownModule()
{
    if (GGASCoreLifecycleHook)
    {
        GGASCoreLifecycleHook->Shutdown();
    }

    GGASCoreLifecycleHandle.Reset();
    GGASCoreLifecycleHook.Reset();

    SOTS::GAS::CoreBridge::UnregisterSaveParticipantIfRegistered();
    GGASSaveParticipant.Reset();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogSOTSGAS, Log, TEXT("SOTS_GAS_Plugin module shutting down."));
#endif
}
