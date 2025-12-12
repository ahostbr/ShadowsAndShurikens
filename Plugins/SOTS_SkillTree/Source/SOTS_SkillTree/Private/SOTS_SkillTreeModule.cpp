#include "SOTS_SkillTreeModule.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogSOTSSkillTree);

IMPLEMENT_MODULE(FSOTS_SkillTreeModule, SOTS_SkillTree)

void FSOTS_SkillTreeModule::StartupModule()
{
    UE_LOG(LogSOTSSkillTree, Log, TEXT("SOTS_SkillTree module starting up"));
}

void FSOTS_SkillTreeModule::ShutdownModule()
{
    UE_LOG(LogSOTSSkillTree, Log, TEXT("SOTS_SkillTree module shutting down"));
}