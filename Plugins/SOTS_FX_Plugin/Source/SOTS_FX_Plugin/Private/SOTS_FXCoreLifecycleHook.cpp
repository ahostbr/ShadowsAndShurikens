#include "SOTS_FXCoreLifecycleHook.h"

#include "SOTS_FXCoreBridgeSettings.h"
#include "SOTS_FXManagerSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_FXCoreBridge, Log, All);

bool FFX_CoreLifecycleHook::IsBridgeEnabled() const
{
	const USOTS_FXCoreBridgeSettings* Settings = USOTS_FXCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
}

bool FFX_CoreLifecycleHook::ShouldLogVerbose() const
{
	const USOTS_FXCoreBridgeSettings* Settings = USOTS_FXCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FFX_CoreLifecycleHook::CacheSubsystemFromWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	if (!CachedFX.IsValid() || CachedFX->GetGameInstance() != GameInstance)
	{
		CachedFX = GameInstance->GetSubsystem<USOTS_FXManagerSubsystem>();
	}
}

void FFX_CoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
	if (!IsBridgeEnabled())
	{
		return;
	}

	CacheSubsystemFromWorld(World);

	if (USOTS_FXManagerSubsystem* Subsystem = CachedFX.Get())
	{
		Subsystem->HandleCoreWorldReady(World);
	}

	if (ShouldLogVerbose())
	{
		UE_LOG(LogSOTS_FXCoreBridge, Verbose, TEXT("SOTS_FX CoreBridge: OnSOTS_WorldStartPlay World=%s"), *GetNameSafe(World));
	}
}

void FFX_CoreLifecycleHook::OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn)
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
	CacheSubsystemFromWorld(World);

	if (USOTS_FXManagerSubsystem* Subsystem = CachedFX.Get())
	{
		Subsystem->HandleCorePrimaryPlayerReady(PC, Pawn);
	}

	if (ShouldLogVerbose())
	{
		UE_LOG(
			LogSOTS_FXCoreBridge,
			Verbose,
			TEXT("SOTS_FX CoreBridge: OnSOTS_PawnPossessed PC=%s Pawn=%s"),
			*GetNameSafe(PC),
			*GetNameSafe(Pawn));
	}
}

void FFX_CoreLifecycleHook::Shutdown()
{
	CachedFX.Reset();
	LastPC.Reset();
	LastPawn.Reset();
}
