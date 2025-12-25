#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"
#include "Templates/Function.h"
#include "SOTS_CoreLifecycleSubsystem.generated.h"

class UWorld;
class AGameModeBase;
class APlayerController;
class AController;
class APawn;
class AHUD;
class ISOTS_CoreLifecycleListener;

USTRUCT(BlueprintType)
struct FSOTS_CoreLifecycleSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Lifecycle")
    TObjectPtr<UWorld> World = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Lifecycle")
    TObjectPtr<AGameModeBase> GameMode = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Lifecycle")
    TObjectPtr<APlayerController> PrimaryPC = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Lifecycle")
    TObjectPtr<APawn> PrimaryPawn = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Lifecycle")
    TObjectPtr<AHUD> PrimaryHUD = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Lifecycle")
    bool bWorldStarted = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnWorldStartPlay_Native, UWorld*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTS_OnPostLogin_Native, AGameModeBase*, APlayerController*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTS_OnLogout_Native, AGameModeBase*, AController*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnPlayerControllerBeginPlay_Native, APlayerController*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTS_OnPawnPossessed_Native, APlayerController*, APawn*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnHUDReady_Native, AHUD*);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnWorldStartPlay_BP, UWorld*, World);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParam(FSOTS_OnPostLogin_BP, AGameModeBase*, GameMode, APlayerController*, NewPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParam(FSOTS_OnLogout_BP, AGameModeBase*, GameMode, AController*, Exiting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnPlayerControllerBeginPlay_BP, APlayerController*, PlayerController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParam(FSOTS_OnPawnPossessed_BP, APlayerController*, PlayerController, APawn*, Pawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnHUDReady_BP, AHUD*, HUD);

UCLASS()
class SOTS_CORE_API USOTS_CoreLifecycleSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void NotifyWorldStartPlay(UWorld* World);
    void NotifyPostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
    void NotifyLogout(AGameModeBase* GameMode, AController* Exiting);
    void NotifyPlayerControllerBeginPlay(APlayerController* PlayerController);
    void NotifyPawnPossessed(APlayerController* PlayerController, APawn* Pawn);
    void NotifyHUDReady(AHUD* HUD);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Core|Lifecycle")
    FSOTS_CoreLifecycleSnapshot GetCurrentSnapshot() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Core|Lifecycle")
    bool HasPrimaryPlayerReady() const;

    FSOTS_OnWorldStartPlay_Native OnWorldStartPlay_Native;
    FSOTS_OnPostLogin_Native OnPostLogin_Native;
    FSOTS_OnLogout_Native OnLogout_Native;
    FSOTS_OnPlayerControllerBeginPlay_Native OnPlayerControllerBeginPlay_Native;
    FSOTS_OnPawnPossessed_Native OnPawnPossessed_Native;
    FSOTS_OnHUDReady_Native OnHUDReady_Native;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnWorldStartPlay_BP OnWorldStartPlay_BP;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnPostLogin_BP OnPostLogin_BP;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnLogout_BP OnLogout_BP;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnPlayerControllerBeginPlay_BP OnPlayerControllerBeginPlay_BP;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnPawnPossessed_BP OnPawnPossessed_BP;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnHUDReady_BP OnHUDReady_BP;

private:
    void DispatchToListeners(TFunctionRef<void(ISOTS_CoreLifecycleListener&)> Fn, const TCHAR* EventName);
    void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS);
    void HandleWorldBeginPlay(UWorld* World);

    void ResetSnapshotForWorld(UWorld* World);
    bool ShouldLogVerbose() const;
    bool ShouldSuppressDuplicates() const;

    FSOTS_CoreLifecycleSnapshot Current;
    TWeakObjectPtr<APlayerController> LastPossessedPC;
    TWeakObjectPtr<APawn> LastPossessedPawn;
    TWeakObjectPtr<AHUD> LastHUDReady;
    TSet<TWeakObjectPtr<APlayerController>> BeganPlayControllers;
    bool bWorldStartBroadcasted = false;

    FDelegateHandle PostWorldInitHandle;
    FDelegateHandle WorldBeginPlayHandle;
};
