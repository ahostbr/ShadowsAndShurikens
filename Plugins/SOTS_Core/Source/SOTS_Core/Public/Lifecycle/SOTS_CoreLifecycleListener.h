#pragma once

#include "CoreMinimal.h"

class UWorld;
class AGameModeBase;
class APlayerController;
class AController;
class APawn;
class AHUD;

static const FName SOTS_CoreLifecycleListenerFeatureName(TEXT("SOTS.CoreLifecycleListener"));

class ISOTS_CoreLifecycleListener
{
public:
    virtual ~ISOTS_CoreLifecycleListener() = default;

    virtual void OnSOTS_WorldStartPlay(UWorld* World) {}
    virtual void OnSOTS_PostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer) {}
    virtual void OnSOTS_Logout(AGameModeBase* GameMode, AController* Exiting) {}
    virtual void OnSOTS_PlayerControllerBeginPlay(APlayerController* PC) {}
    virtual void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) {}
    virtual void OnSOTS_HUDReady(AHUD* HUD) {}
};
