#include "SOTS_DebugCoreLifecycleHook.h"

#include "SOTS_DebugCoreBridgeSettings.h"
#include "SOTS_SuiteDebugSubsystem.h"

#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTSDebugCoreBridge, Log, All);

bool FSOTS_DebugCoreLifecycleHook::IsBridgeEnabled() const
{
#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
	return false;
#else
	const USOTS_DebugCoreBridgeSettings* Settings = USOTS_DebugCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
#endif
}

bool FSOTS_DebugCoreLifecycleHook::ShouldLogVerbose() const
{
	const USOTS_DebugCoreBridgeSettings* Settings = USOTS_DebugCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FSOTS_DebugCoreLifecycleHook::CacheSubsystemFromWorld(UWorld* World)
{
	CachedWorld = World;
	CachedSuiteDebug.Reset();

	if (!World)
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	CachedSuiteDebug = GameInstance->GetSubsystem<USOTS_SuiteDebugSubsystem>();
}

void FSOTS_DebugCoreLifecycleHook::BindToCoreTravelDelegates(UGameInstance* GameInstance)
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
			PreLoadMapHandle = Lifecycle->OnPreLoadMap_Native.AddRaw(this, &FSOTS_DebugCoreLifecycleHook::HandlePreLoadMap);
		}

		if (!PostLoadMapHandle.IsValid())
		{
			PostLoadMapHandle = Lifecycle->OnPostLoadMap_Native.AddRaw(this, &FSOTS_DebugCoreLifecycleHook::HandlePostLoadMap);
		}
	}
}

void FSOTS_DebugCoreLifecycleHook::UnbindCoreTravelDelegates()
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

void FSOTS_DebugCoreLifecycleHook::HandlePreLoadMap(const FString& MapName)
{
	if (USOTS_SuiteDebugSubsystem* DebugSubsystem = CachedSuiteDebug.Get())
	{
		// Safe reset only (no new widget creation): ensure overlay is removed before travel.
		DebugSubsystem->HideKEMAnchorOverlay();
	}

	CacheSubsystemFromWorld(nullptr);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogSOTSDebugCoreBridge, Verbose, TEXT("SOTS_Debug CoreBridge: PreLoadMap MapName=%s"), *MapName);
	}
}

void FSOTS_DebugCoreLifecycleHook::HandlePostLoadMap(UWorld* World)
{
	CacheSubsystemFromWorld(World);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogSOTSDebugCoreBridge, Verbose, TEXT("SOTS_Debug CoreBridge: PostLoadMap World=%s"), *GetNameSafe(World));
	}
}

void FSOTS_DebugCoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
	if (!IsBridgeEnabled() || !World)
	{
		return;
	}

	BindToCoreTravelDelegates(World->GetGameInstance());
	CacheSubsystemFromWorld(World);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogSOTSDebugCoreBridge, Verbose, TEXT("SOTS_Debug CoreBridge: OnSOTS_WorldStartPlay World=%s"), *GetNameSafe(World));
	}
}

void FSOTS_DebugCoreLifecycleHook::Shutdown()
{
	UnbindCoreTravelDelegates();
	CacheSubsystemFromWorld(nullptr);
}
