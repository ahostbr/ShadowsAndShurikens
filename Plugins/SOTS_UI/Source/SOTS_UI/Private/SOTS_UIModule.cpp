// SOTS_UIModule.cpp
// Minimal runtime module for the SOTS_UI plugin.

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

#include "GameFramework/HUD.h"

#include "Features/IModularFeatures.h"
#include "Lifecycle/SOTS_CoreLifecycleListener.h"
#include "SOTS_UIRouterSubsystem.h"
#include "SOTS_UISettings.h"

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

        IModularFeatures::Get().RegisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, &CoreLifecycleHook);
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogSOTS_UIModule, Log, TEXT("SOTS_UI module shutting down"));

        if (IModularFeatures::Get().IsModularFeatureAvailable(SOTS_CoreLifecycleListenerFeatureName))
        {
            IModularFeatures::Get().UnregisterModularFeature(SOTS_CoreLifecycleListenerFeatureName, &CoreLifecycleHook);
        }
    }

private:
    class FSOTS_UI_CoreLifecycleHook final : public ISOTS_CoreLifecycleListener
    {
    public:
        virtual void OnSOTS_HUDReady(AHUD* HUD) override
        {
            const USOTS_UISettings* Settings = USOTS_UISettings::Get();
            if (!Settings || !Settings->bEnableSOTSCoreHUDHostBridge)
            {
                return;
            }

            if (!HUD)
            {
                return;
            }

            USOTS_UIRouterSubsystem* Router = USOTS_UIRouterSubsystem::Get(HUD);
            if (!Router)
            {
                if (Settings->bEnableSOTSCoreBridgeVerboseLogs)
                {
                    UE_LOG(LogSOTS_UIModule, Verbose, TEXT("SOTS_UI BRIDGE3: HUDReady but router missing. HUD=%s"), *GetNameSafe(HUD));
                }
                return;
            }

            Router->RegisterHUDHost(HUD);
        }
    };

    FSOTS_UI_CoreLifecycleHook CoreLifecycleHook;
};

// The module name MUST match the .uplugin Modules entry → "SOTS_UI"
IMPLEMENT_MODULE(FSOTS_UIModule, SOTS_UI);
