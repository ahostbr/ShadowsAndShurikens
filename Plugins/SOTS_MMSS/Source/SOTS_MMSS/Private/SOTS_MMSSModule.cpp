#include "SOTS_MMSSCoreBridgeSettings.h"
#include "SOTS_MMSSCoreLifecycleHook.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

#include "Modules/ModuleManager.h"

class FSOTS_MMSSModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FSOTS_MMSSModule, SOTS_MMSS)

namespace SOTS_MMSS::CoreBridge
{
    FMMSS_CoreLifecycleHook GHook;
    FSOTS_CoreLifecycleListenerHandle GHookHandle;

    void Register()
    {
        const USOTS_MMSSCoreBridgeSettings* Settings = USOTS_MMSSCoreBridgeSettings::Get();
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

void FSOTS_MMSSModule::StartupModule()
{
    SOTS_MMSS::CoreBridge::Register();
}

void FSOTS_MMSSModule::ShutdownModule()
{
    SOTS_MMSS::CoreBridge::Unregister();
}
