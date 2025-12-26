#pragma once

#include "CoreMinimal.h"

#include "Lifecycle/SOTS_CoreLifecycleListener.h"

class USOTS_AIPerceptionSubsystem;
class USOTS_CoreLifecycleSubsystem;
class UWorld;
class APlayerController;
class APawn;

class FAIPerception_CoreLifecycleHook final : public ISOTS_CoreLifecycleListener
{
public:
	void OnSOTS_WorldStartPlay(UWorld* World) override;
	void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override;

	void Shutdown();

private:
	bool IsBridgeEnabled() const;
	bool ShouldLogVerbose() const;

	void CacheSubsystemsFromWorld(UWorld* World);
	void BindTravelDelegatesIfEnabled();

	void HandleCorePreLoadMap(const FString& MapName);
	void HandleCorePostLoadMap(UWorld* World);

	TWeakObjectPtr<USOTS_AIPerceptionSubsystem> CachedAIPerception;
	TWeakObjectPtr<USOTS_CoreLifecycleSubsystem> CachedCoreLifecycle;

	TWeakObjectPtr<APlayerController> LastPC;
	TWeakObjectPtr<APawn> LastPawn;

	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;
};
