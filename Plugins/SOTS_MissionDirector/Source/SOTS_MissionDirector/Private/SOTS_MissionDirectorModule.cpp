#include "SOTS_MissionDirectorModule.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTSMissionDirector);

IMPLEMENT_MODULE(FSOTS_MissionDirectorModule, SOTS_MissionDirector)

void FSOTS_MissionDirectorModule::StartupModule()
{
    UE_LOG(LogSOTSMissionDirector, Log, TEXT("SOTS_MissionDirector module starting up"));
}

void FSOTS_MissionDirectorModule::ShutdownModule()
{
    UE_LOG(LogSOTSMissionDirector, Log, TEXT("SOTS_MissionDirector module shutting down"));
}