// SOTS_INVModule.cpp
// Minimal runtime module for the SOTS_INV plugin.

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_INVModule, Log, All);

/**
 * All real logic for inventory bridging lives in USOTS_InventoryBridgeSubsystem
 * and related components. This module just needs to exist so the engine can
 * load the plugin cleanly.
 */
class FSOTS_INVModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogSOTS_INVModule, Log, TEXT("SOTS_INV module starting up"));
        // IMPORTANT: Do NOT touch worlds, GameInstance, or assets here.
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogSOTS_INVModule, Log, TEXT("SOTS_INV module shutting down"));
    }
};

// The module name MUST match the .uplugin Modules entry → "SOTS_INV"
IMPLEMENT_MODULE(FSOTS_INVModule, SOTS_INV);
