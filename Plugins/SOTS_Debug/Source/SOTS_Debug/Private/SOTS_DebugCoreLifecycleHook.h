#pragma once

#include "CoreMinimal.h"

#include "Lifecycle/SOTS_CoreLifecycleListener.h"

class UGameInstance;
class USOTS_CoreLifecycleSubsystem;
class USOTS_SuiteDebugSubsystem;

class FSOTS_DebugCoreLifecycleHook final : public ISOTS_CoreLifecycleListener
{
public:
	virtual void OnSOTS_WorldStartPlay(UWorld* World) override;

	void Shutdown();

private:
	bool IsBridgeEnabled() const;
	bool ShouldLogVerbose() const;

	void CacheSubsystemFromWorld(UWorld* World);

	void BindToCoreTravelDelegates(UGameInstance* GameInstance);
	void UnbindCoreTravelDelegates();

	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* World);

	TWeakObjectPtr<UGameInstance> CachedGameInstance;
	TWeakObjectPtr<USOTS_CoreLifecycleSubsystem> CachedCoreLifecycle;
	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;

	TWeakObjectPtr<UWorld> CachedWorld;
	TWeakObjectPtr<USOTS_SuiteDebugSubsystem> CachedSuiteDebug;
};
