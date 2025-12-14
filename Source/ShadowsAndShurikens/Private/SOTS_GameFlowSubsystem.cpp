#include "SOTS_GameFlowSubsystem.h"

#include "GameMapsSettings.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_UIRouterSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_GameFlow, Log, All);

void USOTS_GameFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedRouter = USOTS_UIRouterSubsystem::Get(this);
	if (CachedRouter)
	{
		CachedRouter->OnReturnToMainMenuRequested.AddDynamic(this, &USOTS_GameFlowSubsystem::HandleReturnToMainMenuRequested);
	}
	else
	{
		UE_LOG(LogSOTS_GameFlow, Warning, TEXT("GameFlow: UI router unavailable; return-to-main-menu requests cannot be handled."));
	}
}

void USOTS_GameFlowSubsystem::Deinitialize()
{
	if (CachedRouter)
	{
		CachedRouter->OnReturnToMainMenuRequested.RemoveDynamic(this, &USOTS_GameFlowSubsystem::HandleReturnToMainMenuRequested);
		CachedRouter = nullptr;
	}

	Super::Deinitialize();
}

void USOTS_GameFlowSubsystem::HandleReturnToMainMenuRequested()
{
	if (!PerformReturnToMainMenu())
	{
		UE_LOG(LogSOTS_GameFlow, Warning, TEXT("GameFlow: Return-to-main-menu request was not executed (missing map or world)."));
	}
}

bool USOTS_GameFlowSubsystem::PerformReturnToMainMenu()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FString TargetMapPath;
	if (MainMenuMap.IsValid())
	{
		TargetMapPath = MainMenuMap.GetLongPackageName();
	}
	else
	{
		TargetMapPath = UGameMapsSettings::GetGameDefaultMap();
	}

	if (TargetMapPath.IsEmpty())
	{
		return false;
	}

	UGameplayStatics::OpenLevel(this, FName(*TargetMapPath));
	return true;
}
