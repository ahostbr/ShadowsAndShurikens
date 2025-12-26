#pragma once

#include "CoreMinimal.h"

#include "Lifecycle/SOTS_CoreLifecycleListener.h"

class UGameInstance;
class UWorld;
class APlayerController;
class APawn;
class USOTS_CoreLifecycleSubsystem;

class FLightProbe_CoreLifecycleHook final : public ISOTS_CoreLifecycleListener
{
public:
    void OnSOTS_WorldStartPlay(UWorld* World) override;
    void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override;

    void Shutdown();

private:
    bool IsBridgeEnabled() const;
    bool ShouldLogVerbose() const;

    void BindToCoreTravelDelegates(UGameInstance* GameInstance);
    void UnbindCoreTravelDelegates();

    void HandlePreLoadMap(const FString& MapName);
    void HandlePostLoadMap(UWorld* World);

    TWeakObjectPtr<UGameInstance> CachedGameInstance;
    TWeakObjectPtr<USOTS_CoreLifecycleSubsystem> CachedCoreLifecycle;
    FDelegateHandle PreLoadMapHandle;
    FDelegateHandle PostLoadMapHandle;

    TWeakObjectPtr<APlayerController> LastPC;
    TWeakObjectPtr<APawn> LastPawn;
};
