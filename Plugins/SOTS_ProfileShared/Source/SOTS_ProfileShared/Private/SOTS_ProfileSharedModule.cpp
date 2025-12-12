// SOTS_ProfileSharedModule.cpp
// Minimal runtime module for shared profile types.

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_ProfileSharedModule, Log, All);

/**
 * All real logic for profiles lives in the structs in SOTS_ProfileTypes.h
 * and in other systems that *use* those structs. This module just needs
 * to exist so the engine can load the plugin cleanly.
 */
class FSOTS_ProfileSharedModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogSOTS_ProfileSharedModule, Log, TEXT("SOTS_ProfileShared module starting up"));
        // IMPORTANT: Do NOT touch worlds, GameInstance, or assets here.
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogSOTS_ProfileSharedModule, Log, TEXT("SOTS_ProfileShared module shutting down"));
    }
};

// The module name MUST match the .uplugin Modules entry → "SOTS_ProfileShared"
IMPLEMENT_MODULE(FSOTS_ProfileSharedModule, SOTS_ProfileShared);
