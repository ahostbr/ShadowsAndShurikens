#pragma once

#include "CoreMinimal.h"
#include "Lifecycle/SOTS_CoreLifecycleListener.h"

#include "Delegates/Delegate.h"
#include "UObject/WeakObjectPtr.h"

class USOTS_CoreLifecycleSubsystem;
class USOTS_MissionDirectorSubsystem;
class UWorld;
class APlayerController;
class APawn;

class FMissionDirector_CoreLifecycleHook final : public ISOTS_CoreLifecycleListener
{
public:
    virtual void OnSOTS_WorldStartPlay(UWorld* World) override;
    virtual void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override;

    void Shutdown();

private:
    bool IsBridgeEnabled() const;
    bool ShouldLogVerbose() const;

    void CacheSubsystemsFromWorld(UWorld* World);
    void BindTravelDelegatesIfEnabled();

    void HandleCorePreLoadMap(const FString& MapName);
    void HandleCorePostLoadMap(UWorld* World);

private:
    TWeakObjectPtr<USOTS_MissionDirectorSubsystem> CachedMissionDirector;
    TWeakObjectPtr<USOTS_CoreLifecycleSubsystem> CachedCoreLifecycle;

    FDelegateHandle PreLoadMapHandle;
    FDelegateHandle PostLoadMapHandle;
};
