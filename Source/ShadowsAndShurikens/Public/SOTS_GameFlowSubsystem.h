#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_GameFlowSubsystem.generated.h"

class USOTS_UIRouterSubsystem;

/**
 * Handles high-level game flow requests raised by UI (e.g., returning to the main menu).
 * Keeps the actual travel logic outside of SOTS_UI while still using the router as the hub.
 */
UCLASS(Config=Game)
class SHADOWSANDSHURIKENS_API USOTS_GameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	UFUNCTION()
	void HandleReturnToMainMenuRequested();

	bool PerformReturnToMainMenu();

private:
	// Optional map override for main menu. Falls back to GameMapsSettings::GetGameDefaultMap when unset.
	UPROPERTY(EditAnywhere, Config, Category = "GameFlow")
	FSoftObjectPath MainMenuMap;

	UPROPERTY()
	TObjectPtr<USOTS_UIRouterSubsystem> CachedRouter;
};
