#include "SOTS_GASCoreLifecycleHook.h"

#include "SOTS_GASCoreBridgeDelegates.h"
#include "SOTS_GASCoreBridgeSettings.h"

#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogGASCoreBridge, Log, All);

bool FGAS_CoreLifecycleHook::IsBridgeEnabled() const
{
	const USOTS_GASCoreBridgeSettings* Settings = USOTS_GASCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
}

bool FGAS_CoreLifecycleHook::ShouldLogVerbose() const
{
	const USOTS_GASCoreBridgeSettings* Settings = USOTS_GASCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FGAS_CoreLifecycleHook::BindToCoreTravelDelegates(UGameInstance* GameInstance)
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
			PreLoadMapHandle = Lifecycle->OnPreLoadMap_Native.AddRaw(this, &FGAS_CoreLifecycleHook::HandlePreLoadMap);
		}

		if (!PostLoadMapHandle.IsValid())
		{
			PostLoadMapHandle = Lifecycle->OnPostLoadMap_Native.AddRaw(this, &FGAS_CoreLifecycleHook::HandlePostLoadMap);
		}
	}
}

void FGAS_CoreLifecycleHook::UnbindCoreTravelDelegates()
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

void FGAS_CoreLifecycleHook::HandlePreLoadMap(const FString& MapName)
{
	LastPC.Reset();
	LastPawn.Reset();

	SOTS_GAS::CoreBridge::OnCorePreLoadMap_Native().Broadcast(MapName);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogGASCoreBridge, Verbose, TEXT("GAS CoreBridge: PreLoadMap MapName=%s"), *MapName);
	}
}

void FGAS_CoreLifecycleHook::HandlePostLoadMap(UWorld* World)
{
	SOTS_GAS::CoreBridge::OnCorePostLoadMap_Native().Broadcast(World);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogGASCoreBridge, Verbose, TEXT("GAS CoreBridge: PostLoadMap World=%s"), *GetNameSafe(World));
	}
}

void FGAS_CoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
	if (!IsBridgeEnabled() || !World)
	{
		return;
	}

	BindToCoreTravelDelegates(World->GetGameInstance());

	SOTS_GAS::CoreBridge::OnCoreWorldReady_Native().Broadcast(World);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogGASCoreBridge, Verbose, TEXT("GAS CoreBridge: OnSOTS_WorldStartPlay World=%s"), *GetNameSafe(World));
	}
}

void FGAS_CoreLifecycleHook::OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn)
{
	if (!IsBridgeEnabled() || !PC || !Pawn)
	{
		return;
	}

	if (!PC->IsLocalController())
	{
		return;
	}

	if (LastPC.Get() == PC && LastPawn.Get() == Pawn)
	{
		return;
	}

	LastPC = PC;
	LastPawn = Pawn;

	SOTS_GAS::CoreBridge::OnCorePrimaryPlayerReady_Native().Broadcast(PC, Pawn);

	if (ShouldLogVerbose())
	{
		UE_LOG(LogGASCoreBridge, Verbose, TEXT("GAS CoreBridge: OnSOTS_PawnPossessed PC=%s Pawn=%s"), *GetNameSafe(PC), *GetNameSafe(Pawn));
	}
}

void FGAS_CoreLifecycleHook::Shutdown()
{
	UnbindCoreTravelDelegates();
	LastPC.Reset();
	LastPawn.Reset();
}
