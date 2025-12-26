#include "CoreMinimal.h"

#include "SOTS_FXCoreBridgeSettings.h"
#include "SOTS_FXCoreLifecycleHook.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

#include "Modules/ModuleManager.h"

namespace SOTS::FX::CoreBridge
{
    FFX_CoreLifecycleHook GHook;
    FSOTS_CoreLifecycleListenerHandle GHookHandle;

    void Register()
    {
        const USOTS_FXCoreBridgeSettings* Settings = USOTS_FXCoreBridgeSettings::Get();
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

class FSOTS_FX_PluginModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        SOTS::FX::CoreBridge::Register();
    }

    virtual void ShutdownModule() override
    {
        SOTS::FX::CoreBridge::Unregister();
    }
};

IMPLEMENT_MODULE(FSOTS_FX_PluginModule, SOTS_FX_Plugin);
