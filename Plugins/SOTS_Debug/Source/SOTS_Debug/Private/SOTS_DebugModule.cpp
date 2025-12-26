#include "Modules/ModuleManager.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

#include "SOTS_DebugCoreBridgeSettings.h"
#include "SOTS_DebugCoreLifecycleHook.h"

namespace
{
    TUniquePtr<FSOTS_DebugCoreLifecycleHook> GDebugCoreLifecycleHook;
    TUniquePtr<FSOTS_CoreLifecycleListenerHandle> GDebugCoreLifecycleHandle;
}

class FSOTS_DebugModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        const USOTS_DebugCoreBridgeSettings* Settings = USOTS_DebugCoreBridgeSettings::Get();
        if (Settings && Settings->bEnableSOTSCoreLifecycleBridge)
        {
            GDebugCoreLifecycleHook = MakeUnique<FSOTS_DebugCoreLifecycleHook>();
            GDebugCoreLifecycleHandle = MakeUnique<FSOTS_CoreLifecycleListenerHandle>(GDebugCoreLifecycleHook.Get());
        }
#endif
    }

    virtual void ShutdownModule() override
    {
        if (GDebugCoreLifecycleHook)
        {
            GDebugCoreLifecycleHook->Shutdown();
        }

        GDebugCoreLifecycleHandle.Reset();
        GDebugCoreLifecycleHook.Reset();
    }
};

IMPLEMENT_MODULE(FSOTS_DebugModule, SOTS_Debug);
