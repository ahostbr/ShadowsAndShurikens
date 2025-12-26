#include "SOTS_GlobalStealthManagerCoreLifecycleHook.h"

#include "SOTS_GlobalStealthManagerCoreBridgeSettings.h"
#include "SOTS_GlobalStealthManagerSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_GSMCoreBridge, Log, All);

bool FGSM_CoreLifecycleHook::IsBridgeEnabled() const
{
	const USOTS_GlobalStealthManagerCoreBridgeSettings* Settings = USOTS_GlobalStealthManagerCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
}

bool FGSM_CoreLifecycleHook::ShouldLogVerbose() const
{
	const USOTS_GlobalStealthManagerCoreBridgeSettings* Settings = USOTS_GlobalStealthManagerCoreBridgeSettings::Get();
	return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FGSM_CoreLifecycleHook::CacheSubsystemFromWorld(UWorld* World)
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

	if (!CachedGSM.IsValid() || CachedGSM->GetGameInstance() != GameInstance)
	{
		CachedGSM = GameInstance->GetSubsystem<USOTS_GlobalStealthManagerSubsystem>();
	}
}

void FGSM_CoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
	if (!IsBridgeEnabled())
	{
		return;
	}

	CacheSubsystemFromWorld(World);

	if (USOTS_GlobalStealthManagerSubsystem* Subsystem = CachedGSM.Get())
	{
		Subsystem->HandleCoreWorldReady(World);
	}

	if (ShouldLogVerbose())
	{
		UE_LOG(LogSOTS_GSMCoreBridge, Verbose, TEXT("SOTS_GSM CoreBridge: OnSOTS_WorldStartPlay World=%s"), *GetNameSafe(World));
	}
}

void FGSM_CoreLifecycleHook::OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn)
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

	if (USOTS_GlobalStealthManagerSubsystem* Subsystem = CachedGSM.Get())
	{
		Subsystem->HandleCorePrimaryPlayerReady(PC, Pawn);
	}

	if (ShouldLogVerbose())
	{
		UE_LOG(
			LogSOTS_GSMCoreBridge,
			Verbose,
			TEXT("SOTS_GSM CoreBridge: OnSOTS_PawnPossessed PC=%s Pawn=%s"),
			*GetNameSafe(PC),
			*GetNameSafe(Pawn));
	}
}

void FGSM_CoreLifecycleHook::Shutdown()
{
	CachedGSM.Reset();
	LastPC.Reset();
	LastPawn.Reset();
}
