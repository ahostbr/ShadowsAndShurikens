#include "SOTS_GlobalStealthManagerModule.h"

#include "SOTS_GlobalStealthManagerCoreBridgeSettings.h"
#include "SOTS_GlobalStealthManagerCoreLifecycleHook.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTSGlobalStealth);

IMPLEMENT_MODULE(FSOTS_GlobalStealthManagerModule, SOTS_GlobalStealthManager)

namespace SOTS::GlobalStealthManager::CoreBridge
{
    FGSM_CoreLifecycleHook GHook;
    FSOTS_CoreLifecycleListenerHandle GHookHandle;

    void Register()
    {
        const USOTS_GlobalStealthManagerCoreBridgeSettings* Settings = USOTS_GlobalStealthManagerCoreBridgeSettings::Get();
        if (!Settings || !Settings->bEnableSOTSCoreLifecycleBridge)
        {
            return;
        }

        GHookHandle.Register(&GHook);
    }

    void Unregister()
    {
        GHook.Shutdown();
        GHookHandle.Unregister();
    }
}

void FSOTS_GlobalStealthManagerModule::StartupModule()
{
    UE_LOG(LogSOTSGlobalStealth, Log, TEXT("SOTS_GlobalStealthManager module starting up"));
	SOTS::GlobalStealthManager::CoreBridge::Register();
}

void FSOTS_GlobalStealthManagerModule::ShutdownModule()
{
    UE_LOG(LogSOTSGlobalStealth, Log, TEXT("SOTS_GlobalStealthManager module shutting down"));
	SOTS::GlobalStealthManager::CoreBridge::Unregister();
}