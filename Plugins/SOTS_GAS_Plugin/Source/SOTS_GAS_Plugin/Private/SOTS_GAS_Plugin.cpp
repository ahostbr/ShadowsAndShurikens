#include "SOTS_GAS_Plugin.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTSGAS);
IMPLEMENT_MODULE(FSOTS_GAS_PluginModule, SOTS_GAS_Plugin);

void FSOTS_GAS_PluginModule::StartupModule()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogSOTSGAS, Log, TEXT("SOTS_GAS_Plugin module starting up."));
#endif
}

void FSOTS_GAS_PluginModule::ShutdownModule()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogSOTSGAS, Log, TEXT("SOTS_GAS_Plugin module shutting down."));
#endif
}
