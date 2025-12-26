#pragma once

#include "CoreMinimal.h"

#include "Lifecycle/SOTS_CoreLifecycleListener.h"

class USOTS_GlobalStealthManagerSubsystem;
class UWorld;
class APlayerController;
class APawn;

class FGSM_CoreLifecycleHook final : public ISOTS_CoreLifecycleListener
{
public:
	void OnSOTS_WorldStartPlay(UWorld* World) override;
	void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override;

	void Shutdown();

private:
	bool IsBridgeEnabled() const;
	bool ShouldLogVerbose() const;

	void CacheSubsystemFromWorld(UWorld* World);

	TWeakObjectPtr<USOTS_GlobalStealthManagerSubsystem> CachedGSM;
	TWeakObjectPtr<APlayerController> LastPC;
	TWeakObjectPtr<APawn> LastPawn;
};
