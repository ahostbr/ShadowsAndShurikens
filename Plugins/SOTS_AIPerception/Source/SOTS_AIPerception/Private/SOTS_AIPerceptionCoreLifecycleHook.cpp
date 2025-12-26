#include "SOTS_AIPerceptionCoreLifecycleHook.h"

#include "SOTS_AIPerceptionCoreBridgeSettings.h"
#include "SOTS_AIPerceptionSubsystem.h"

#include "Settings/SOTS_CoreSettings.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_AIPerceptionCoreBridge, Log, All);

bool FAIPerception_CoreLifecycleHook::IsBridgeEnabled() const
{
	const USOTS_AIPerceptionCoreBridgeSettings* Settings = USOTS_AIPerceptionCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
}

bool FAIPerception_CoreLifecycleHook::ShouldLogVerbose() const
{
	const USOTS_AIPerceptionCoreBridgeSettings* Settings = USOTS_AIPerceptionCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FAIPerception_CoreLifecycleHook::CacheSubsystemsFromWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}

	if (!CachedAIPerception.IsValid() || CachedAIPerception->GetWorld() != World)
	{
		CachedAIPerception = World->GetSubsystem<USOTS_AIPerceptionSubsystem>();
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	if (!CachedCoreLifecycle.IsValid() || CachedCoreLifecycle->GetGameInstance() != GameInstance)
	{
		CachedCoreLifecycle = GameInstance->GetSubsystem<USOTS_CoreLifecycleSubsystem>();
	}
}

void FAIPerception_CoreLifecycleHook::BindTravelDelegatesIfEnabled()
{
	USOTS_CoreLifecycleSubsystem* CoreLifecycle = CachedCoreLifecycle.Get();
	if (!CoreLifecycle)
	{
		return;
	}

	const USOTS_CoreSettings* CoreSettings = USOTS_CoreSettings::Get();
	if (!CoreSettings || !CoreSettings->bEnableMapTravelBridge)
	{
		return;
	}

	if (!PreLoadMapHandle.IsValid())
	{
		PreLoadMapHandle = CoreLifecycle->OnPreLoadMap_Native.AddRaw(this, &FAIPerception_CoreLifecycleHook::HandleCorePreLoadMap);
	}

	if (!PostLoadMapHandle.IsValid())
	{
		PostLoadMapHandle = CoreLifecycle->OnPostLoadMap_Native.AddRaw(this, &FAIPerception_CoreLifecycleHook::HandleCorePostLoadMap);
	}
}

void FAIPerception_CoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
	if (!IsBridgeEnabled())
	{
		return;
	}

	CacheSubsystemsFromWorld(World);

	if (USOTS_AIPerceptionSubsystem* Subsystem = CachedAIPerception.Get())
	{
		Subsystem->HandleCoreWorldReady(World);
	}

	BindTravelDelegatesIfEnabled();

	if (ShouldLogVerbose())
	{
		UE_LOG(LogSOTS_AIPerceptionCoreBridge, Verbose, TEXT("SOTS_AIPerception CoreBridge: OnSOTS_WorldStartPlay World=%s"), *GetNameSafe(World));
	}
}

void FAIPerception_CoreLifecycleHook::OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn)
{
	if (!IsBridgeEnabled() || !PC || !Pawn)
	{
		return;
	}

	if (LastPC.Get() == PC && LastPawn.Get() == Pawn)
	{
		return;
	}

	LastPC = PC;
	LastPawn = Pawn;

	UWorld* World = PC ? PC->GetWorld() : nullptr;
	CacheSubsystemsFromWorld(World);

	if (USOTS_AIPerceptionSubsystem* Subsystem = CachedAIPerception.Get())
	{
		Subsystem->HandleCorePrimaryPlayerReady(PC, Pawn);
	}

	BindTravelDelegatesIfEnabled();

	if (ShouldLogVerbose())
	{
		UE_LOG(
			LogSOTS_AIPerceptionCoreBridge,
			Verbose,
			TEXT("SOTS_AIPerception CoreBridge: OnSOTS_PawnPossessed PC=%s Pawn=%s"),
			*GetNameSafe(PC),
			*GetNameSafe(Pawn));
	}
}

void FAIPerception_CoreLifecycleHook::HandleCorePreLoadMap(const FString& MapName)
{
	if (!IsBridgeEnabled())
	{
		return;
	}

	if (USOTS_AIPerceptionSubsystem* Subsystem = CachedAIPerception.Get())
	{
		Subsystem->HandleCorePreLoadMap(MapName);
	}

	if (ShouldLogVerbose())
	{
		UE_LOG(LogSOTS_AIPerceptionCoreBridge, Verbose, TEXT("SOTS_AIPerception CoreBridge: PreLoadMap MapName=%s"), *MapName);
	}
}

void FAIPerception_CoreLifecycleHook::HandleCorePostLoadMap(UWorld* World)
{
	if (!IsBridgeEnabled())
	{
		return;
	}

	CacheSubsystemsFromWorld(World);

	if (USOTS_AIPerceptionSubsystem* Subsystem = CachedAIPerception.Get())
	{
		Subsystem->HandleCorePostLoadMap(World);
	}

	if (ShouldLogVerbose())
	{
		UE_LOG(LogSOTS_AIPerceptionCoreBridge, Verbose, TEXT("SOTS_AIPerception CoreBridge: PostLoadMap World=%s"), *GetNameSafe(World));
	}
}

void FAIPerception_CoreLifecycleHook::Shutdown()
{
	if (USOTS_CoreLifecycleSubsystem* CoreLifecycle = CachedCoreLifecycle.Get())
	{
		if (PreLoadMapHandle.IsValid())
		{
			CoreLifecycle->OnPreLoadMap_Native.Remove(PreLoadMapHandle);
			PreLoadMapHandle = FDelegateHandle();
		}

		if (PostLoadMapHandle.IsValid())
		{
			CoreLifecycle->OnPostLoadMap_Native.Remove(PostLoadMapHandle);
			PostLoadMapHandle = FDelegateHandle();
		}
	}

	CachedAIPerception.Reset();
	CachedCoreLifecycle.Reset();
	LastPC.Reset();
	LastPawn.Reset();
}
