
#include "LightProbePluginModule.h"

#include "LightProbeCoreBridgeSettings.h"
#include "LightProbeCoreLifecycleHook.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FLightProbePluginModule, LightProbePlugin)

namespace LightProbePlugin::CoreBridge
{
	FLightProbe_CoreLifecycleHook GHook;
	FSOTS_CoreLifecycleListenerHandle GHookHandle;

	void Register()
	{
		const ULightProbeCoreBridgeSettings* Settings = ULightProbeCoreBridgeSettings::Get();
		if (!Settings || !Settings->bEnableSOTSCoreLifecycleBridge)
		{
			return;
		}

		GHookHandle.Register(&GHook);
	}

	void Unregister()
	{
		GHook.Shutdown();
		GHookHandle.Unregister();
	}
}

void FLightProbePluginModule::StartupModule()
{
	LightProbePlugin::CoreBridge::Register();
}

void FLightProbePluginModule::ShutdownModule()
{
	LightProbePlugin::CoreBridge::Unregister();
}
