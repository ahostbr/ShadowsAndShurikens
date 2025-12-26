#pragma once

#include "CoreMinimal.h"

#include "Lifecycle/SOTS_CoreLifecycleListener.h"

class USOTS_FXManagerSubsystem;
class UWorld;
class APlayerController;
class APawn;

class FFX_CoreLifecycleHook final : public ISOTS_CoreLifecycleListener
{
public:
	void OnSOTS_WorldStartPlay(UWorld* World) override;
	void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override;

	void Shutdown();

private:
	bool IsBridgeEnabled() const;
	bool ShouldLogVerbose() const;

	void CacheSubsystemFromWorld(UWorld* World);

	TWeakObjectPtr<USOTS_FXManagerSubsystem> CachedFX;
	TWeakObjectPtr<APlayerController> LastPC;
	TWeakObjectPtr<APawn> LastPawn;
};
