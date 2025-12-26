#include "SOTS_MissionDirectorModule.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"
#include "SOTS_MissionDirectorCoreBridgeSettings.h"
#include "SOTS_MissionDirectorCoreLifecycleHook.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTSMissionDirector);

IMPLEMENT_MODULE(FSOTS_MissionDirectorModule, SOTS_MissionDirector)

namespace
{
    TUniquePtr<FMissionDirector_CoreLifecycleHook> GMissionDirectorCoreLifecycleHook;
    TUniquePtr<FSOTS_CoreLifecycleListenerHandle> GMissionDirectorCoreLifecycleHandle;
}

void FSOTS_MissionDirectorModule::StartupModule()
{
    UE_LOG(LogSOTSMissionDirector, Log, TEXT("SOTS_MissionDirector module starting up"));

    const USOTS_MissionDirectorCoreBridgeSettings* Settings = USOTS_MissionDirectorCoreBridgeSettings::Get();
    if (Settings && Settings->bEnableSOTSCoreLifecycleBridge)
    {
        GMissionDirectorCoreLifecycleHook = MakeUnique<FMissionDirector_CoreLifecycleHook>();
        GMissionDirectorCoreLifecycleHandle = MakeUnique<FSOTS_CoreLifecycleListenerHandle>(GMissionDirectorCoreLifecycleHook.Get());

        UE_LOG(LogSOTSMissionDirector, Log, TEXT("SOTS_MissionDirector CoreBridge: Registered Core lifecycle listener."));
    }
}

void FSOTS_MissionDirectorModule::ShutdownModule()
{
    if (GMissionDirectorCoreLifecycleHook)
    {
        GMissionDirectorCoreLifecycleHook->Shutdown();
    }

    GMissionDirectorCoreLifecycleHandle.Reset();
    GMissionDirectorCoreLifecycleHook.Reset();

    UE_LOG(LogSOTSMissionDirector, Log, TEXT("SOTS_MissionDirector module shutting down"));
}