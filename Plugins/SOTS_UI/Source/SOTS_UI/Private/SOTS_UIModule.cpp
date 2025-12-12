// SOTS_UIModule.cpp
// Minimal runtime module for the SOTS_UI plugin.

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_UIModule, Log, All);

/**
 * All real logic for the UI bridge lives in the various subsystems/widgets.
 * This module just needs to exist so the engine can load the plugin cleanly.
 */
class FSOTS_UIModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogSOTS_UIModule, Log, TEXT("SOTS_UI module starting up"));
        // IMPORTANT: Do NOT touch worlds, GameInstance, or assets here.
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogSOTS_UIModule, Log, TEXT("SOTS_UI module shutting down"));
    }
};

// The module name MUST match the .uplugin Modules entry → "SOTS_UI"
IMPLEMENT_MODULE(FSOTS_UIModule, SOTS_UI);
