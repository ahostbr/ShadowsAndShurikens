#include "Modules/ModuleManager.h"
#include "SOTS_SteamLog.h"

#include "SOTS_SteamCoreBridgeSettings.h"
#include "SOTS_SteamCoreLifecycleHook.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

DEFINE_LOG_CATEGORY(LogSOTS_Steam);

namespace SOTS::Steam::CoreBridge
{
    FSOTS_SteamCoreLifecycleHook GHook;
    FSOTS_CoreLifecycleListenerHandle GHookHandle;

    void Register()
    {
        const USOTS_SteamCoreBridgeSettings* Settings = USOTS_SteamCoreBridgeSettings::Get();
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

class FSOTS_SteamModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogSOTS_Steam, Log, TEXT("SOTS_Steam module started."));
        SOTS::Steam::CoreBridge::Register();
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogSOTS_Steam, Log, TEXT("SOTS_Steam module shutting down."));
        SOTS::Steam::CoreBridge::Unregister();
    }
};

IMPLEMENT_MODULE(FSOTS_SteamModule, SOTS_Steam)
