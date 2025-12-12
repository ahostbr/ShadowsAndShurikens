#include "SOTS_GlobalStealthManagerModule.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTSGlobalStealth);

IMPLEMENT_MODULE(FSOTS_GlobalStealthManagerModule, SOTS_GlobalStealthManager)

void FSOTS_GlobalStealthManagerModule::StartupModule()
{
    UE_LOG(LogSOTSGlobalStealth, Log, TEXT("SOTS_GlobalStealthManager module starting up"));
}

void FSOTS_GlobalStealthManagerModule::ShutdownModule()
{
    UE_LOG(LogSOTSGlobalStealth, Log, TEXT("SOTS_GlobalStealthManager module shutting down"));
}