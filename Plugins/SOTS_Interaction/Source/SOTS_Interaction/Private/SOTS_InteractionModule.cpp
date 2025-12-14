#include "SOTS_InteractionModule.h"
#include "SOTS_InteractionLog.h"

#define LOCTEXT_NAMESPACE "FSOTS_InteractionModule"

void FSOTS_InteractionModule::StartupModule()
{
    UE_LOG(LogSOTSInteraction, Log, TEXT("SOTS_Interaction module startup."));
}

void FSOTS_InteractionModule::ShutdownModule()
{
    UE_LOG(LogSOTSInteraction, Log, TEXT("SOTS_Interaction module shutdown."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSOTS_InteractionModule, SOTS_Interaction)
