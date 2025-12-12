#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_TagManagerModule, Log, All);

/**
 * Minimal runtime module for SOTS_TagManager.
 * All functional logic lives in USOTS_GameplayTagManagerSubsystem and helpers.
 */
class FSOTS_TagManagerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogSOTS_TagManagerModule, Log, TEXT("SOTS_TagManager module starting up"));
        // IMPORTANT: Do NOT access worlds, game instances, or assets here.
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogSOTS_TagManagerModule, Log, TEXT("SOTS_TagManager module shutting down"));
    }
};

// The module name MUST match the "Name" in SOTS_TagManager.uplugin → "SOTS_TagManager"
IMPLEMENT_MODULE(FSOTS_TagManagerModule, SOTS_TagManager);

