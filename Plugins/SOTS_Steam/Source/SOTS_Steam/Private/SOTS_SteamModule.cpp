#include "Modules/ModuleManager.h"
#include "SOTS_SteamLog.h"

DEFINE_LOG_CATEGORY(LogSOTS_Steam);

class FSOTS_SteamModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogSOTS_Steam, Log, TEXT("SOTS_Steam module started."));
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogSOTS_Steam, Log, TEXT("SOTS_Steam module shutting down."));
    }
};

IMPLEMENT_MODULE(FSOTS_SteamModule, SOTS_Steam)
