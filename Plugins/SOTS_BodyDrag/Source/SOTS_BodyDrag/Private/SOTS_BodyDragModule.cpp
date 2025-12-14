#include "SOTS_BodyDragModule.h"
#include "SOTS_BodyDragLog.h"

#define LOCTEXT_NAMESPACE "FSOTS_BodyDragModule"

void FSOTS_BodyDragModule::StartupModule()
{
    UE_LOG(LogSOTSBodyDrag, Log, TEXT("SOTS_BodyDrag module starting up."));
}

void FSOTS_BodyDragModule::ShutdownModule()
{
    UE_LOG(LogSOTSBodyDrag, Log, TEXT("SOTS_BodyDrag module shutting down."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSOTS_BodyDragModule, SOTS_BodyDrag)
