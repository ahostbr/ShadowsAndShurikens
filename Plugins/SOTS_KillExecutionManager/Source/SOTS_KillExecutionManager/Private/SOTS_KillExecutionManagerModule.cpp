
#include "SOTS_KillExecutionManagerModule.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTSKEM);
IMPLEMENT_MODULE(FSOTS_KillExecutionManagerModule, SOTS_KillExecutionManager)

void FSOTS_KillExecutionManagerModule::StartupModule()
{
    UE_LOG(LogSOTSKEM, Log, TEXT("SOTS KillExecutionManager v0.40.2 module starting up"));
}

void FSOTS_KillExecutionManagerModule::ShutdownModule()
{
    UE_LOG(LogSOTSKEM, Log, TEXT("SOTS KillExecutionManager v0.40.2 module shutting down"));
}
