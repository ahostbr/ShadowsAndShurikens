#include "SOTS_GAS_Plugin.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTSGAS);
IMPLEMENT_MODULE(FSOTS_GAS_PluginModule, SOTS_GAS_Plugin);

void FSOTS_GAS_PluginModule::StartupModule()
{
    UE_LOG(LogSOTSGAS, Log, TEXT("SOTS_GAS_Plugin module starting up."));
}

void FSOTS_GAS_PluginModule::ShutdownModule()
{
    UE_LOG(LogSOTSGAS, Log, TEXT("SOTS_GAS_Plugin module shutting down."));
}
