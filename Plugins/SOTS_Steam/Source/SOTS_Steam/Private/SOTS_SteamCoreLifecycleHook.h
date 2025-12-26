#pragma once

#include "CoreMinimal.h"

#include "Lifecycle/SOTS_CoreLifecycleListener.h"

class UGameInstance;
class UWorld;
class APlayerController;

class FSOTS_SteamCoreLifecycleHook final : public ISOTS_CoreLifecycleListener
{
public:
    void OnSOTS_WorldStartPlay(UWorld* World) override;
    void OnSOTS_PlayerControllerBeginPlay(APlayerController* PC) override;

    void Shutdown();

private:
    bool IsBridgeEnabled() const;
    bool IsCoreTriggeredInitAllowed() const;
    bool ShouldLogVerbose() const;
    void TryCoreTriggeredInit(UGameInstance* GameInstance, const TCHAR* EventName);

    TWeakObjectPtr<UGameInstance> LastInitGameInstance;
    bool bInitAttemptedForLastGameInstance = false;
};
