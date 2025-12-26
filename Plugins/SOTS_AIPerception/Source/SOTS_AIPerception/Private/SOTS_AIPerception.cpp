// Copyright Epic Games, Inc. All Rights Reserved.

#include "SOTS_AIPerception.h"

#include "SOTS_AIPerceptionCoreBridgeSettings.h"
#include "SOTS_AIPerceptionCoreLifecycleHook.h"

#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

#define LOCTEXT_NAMESPACE "FSOTS_AIPerceptionModule"

namespace SOTS::AIPerception::CoreBridge
{
	FAIPerception_CoreLifecycleHook GHook;
	FSOTS_CoreLifecycleListenerHandle GHookHandle;

	void Register()
	{
		const USOTS_AIPerceptionCoreBridgeSettings* Settings = USOTS_AIPerceptionCoreBridgeSettings::Get();
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

void FSOTS_AIPerceptionModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	SOTS::AIPerception::CoreBridge::Register();
}

void FSOTS_AIPerceptionModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	SOTS::AIPerception::CoreBridge::Unregister();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSOTS_AIPerceptionModule, SOTS_AIPerception)