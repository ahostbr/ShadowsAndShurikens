#include "OmniTraceCoreLifecycleHook.h"

#include "OmniTraceCoreBridgeSettings.h"
#include "OmniTraceDebugSubsystem.h"

#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

bool FOmniTrace_CoreLifecycleHook::IsBridgeEnabled() const
{
#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
	return false;
#else
	const UOmniTraceCoreBridgeSettings* Settings = UOmniTraceCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
#endif
}

bool FOmniTrace_CoreLifecycleHook::ShouldLogVerbose() const
{
	const UOmniTraceCoreBridgeSettings* Settings = UOmniTraceCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FOmniTrace_CoreLifecycleHook::CacheSubsystemFromWorld(UWorld* World)
{
	CachedWorld = World;
	CachedDebug.Reset();

	if (!World)
	{
		return;
	}

	CachedDebug = World->GetSubsystem<UOmniTraceDebugSubsystem>();
}

void FOmniTrace_CoreLifecycleHook::BindToCoreTravelDelegates(UGameInstance* GameInstance)
{
	if (!GameInstance)
	{
		return;
	}

	if (CachedGameInstance.Get() != GameInstance)
	{
		UnbindCoreTravelDelegates();
		CachedGameInstance = GameInstance;
	}

	if (USOTS_CoreLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<USOTS_CoreLifecycleSubsystem>())
	{
		CachedCoreLifecycle = Lifecycle;

		if (!Lifecycle->IsMapTravelBridgeBound())
		{
			return;
		}

		if (!PreLoadMapHandle.IsValid())
		{
			PreLoadMapHandle = Lifecycle->OnPreLoadMap_Native.AddRaw(this, &FOmniTrace_CoreLifecycleHook::HandlePreLoadMap);
		}

		if (!PostLoadMapHandle.IsValid())
		{
			PostLoadMapHandle = Lifecycle->OnPostLoadMap_Native.AddRaw(this, &FOmniTrace_CoreLifecycleHook::HandlePostLoadMap);
		}
	}
}

void FOmniTrace_CoreLifecycleHook::UnbindCoreTravelDelegates()
{
	if (USOTS_CoreLifecycleSubsystem* Lifecycle = CachedCoreLifecycle.Get())
	{
		if (PreLoadMapHandle.IsValid())
		{
			Lifecycle->OnPreLoadMap_Native.Remove(PreLoadMapHandle);
			PreLoadMapHandle.Reset();
		}

		if (PostLoadMapHandle.IsValid())
		{
			Lifecycle->OnPostLoadMap_Native.Remove(PostLoadMapHandle);
			PostLoadMapHandle.Reset();
		}
	}

	CachedCoreLifecycle.Reset();
	CachedGameInstance.Reset();
}

void FOmniTrace_CoreLifecycleHook::HandlePreLoadMap(const FString& MapName)
{
	if (UOmniTraceDebugSubsystem* Debug = CachedDebug.Get())
	{
		Debug->ClearLastKEMTrace();
	}

	CacheSubsystemFromWorld(nullptr);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogOmniTrace, Verbose, TEXT("OmniTrace CoreBridge: PreLoadMap MapName=%s"), *MapName);
	}
}

void FOmniTrace_CoreLifecycleHook::HandlePostLoadMap(UWorld* World)
{
	CacheSubsystemFromWorld(World);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogOmniTrace, Verbose, TEXT("OmniTrace CoreBridge: PostLoadMap World=%s"), *GetNameSafe(World));
	}
}

void FOmniTrace_CoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
	if (!IsBridgeEnabled() || !World)
	{
		return;
	}

	BindToCoreTravelDelegates(World->GetGameInstance());
	CacheSubsystemFromWorld(World);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogOmniTrace, Verbose, TEXT("OmniTrace CoreBridge: OnSOTS_WorldStartPlay World=%s"), *GetNameSafe(World));
	}
}

void FOmniTrace_CoreLifecycleHook::Shutdown()
{
	UnbindCoreTravelDelegates();
	CacheSubsystemFromWorld(nullptr);
}
