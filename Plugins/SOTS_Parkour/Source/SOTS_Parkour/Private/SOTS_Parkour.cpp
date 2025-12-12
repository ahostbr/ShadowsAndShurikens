#include "Modules/ModuleManager.h"
#include "SOTS_ParkourLog.h"

class FSOTS_ParkourModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
		UE_LOG(LogSOTS_Parkour, Log, TEXT("SOTS_Parkour module starting"));
    }

    virtual void ShutdownModule() override
    {
		UE_LOG(LogSOTS_Parkour, Log, TEXT("SOTS_Parkour module shutting down"));
    }
};

IMPLEMENT_MODULE(FSOTS_ParkourModule, SOTS_Parkour)

DEFINE_LOG_CATEGORY(LogSOTS_Parkour);
