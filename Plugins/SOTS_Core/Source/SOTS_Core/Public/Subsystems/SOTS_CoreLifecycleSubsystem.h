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
class APlayerState;

USTRUCT(BlueprintType)
struct FSOTS_PrimaryPlayerIdentity
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Identity")
    int32 LocalPlayerIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Identity")
    int32 ControllerId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Identity")
    FString PlayerName;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Identity")
    FString PlayerStateUniqueIdString;

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Identity")
    bool bIsValid = false;
};

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

    UPROPERTY(BlueprintReadOnly, Category = "SOTS|Core|Identity")
    FSOTS_PrimaryPlayerIdentity PrimaryIdentity;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnWorldStartPlay_Native, UWorld*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTS_OnPostLogin_Native, AGameModeBase*, APlayerController*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTS_OnLogout_Native, AGameModeBase*, AController*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnPlayerControllerBeginPlay_Native, APlayerController*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSOTS_OnPawnPossessed_Native, APlayerController*, APawn*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnHUDReady_Native, AHUD*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnPreLoadMap_Native, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnPostLoadMap_Native, UWorld*);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FSOTS_OnWorldCleanup_Native, UWorld*, bool, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FSOTS_OnPostWorldInitialization_Native, UWorld*);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnWorldStartPlay_BP, UWorld*, World);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_OnPostLogin_BP, AGameModeBase*, GameMode, APlayerController*, NewPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_OnLogout_BP, AGameModeBase*, GameMode, AController*, Exiting);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnPlayerControllerBeginPlay_BP, APlayerController*, PlayerController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_OnPawnPossessed_BP, APlayerController*, PlayerController, APawn*, Pawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnHUDReady_BP, AHUD*, HUD);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnPreLoadMap_BP, const FString&, MapName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnPostLoadMap_BP, UWorld*, World);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSOTS_OnWorldCleanup_BP, UWorld*, World, bool, bSessionEnded, bool, bCleanupResources);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnPostWorldInitialization_BP, UWorld*, World);

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
    void NotifyPreLoadMap(const FString& MapName);
    void NotifyPostLoadMap(UWorld* World);
    void NotifyWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    void NotifyPostWorldInitialization(UWorld* World);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Core|Lifecycle")
    FSOTS_CoreLifecycleSnapshot GetCurrentSnapshot() const;

    UFUNCTION(BlueprintCallable, Category = "SOTS|Core|Lifecycle")
    bool HasPrimaryPlayerReady() const;

    FSOTS_PrimaryPlayerIdentity BuildPrimaryIdentity() const;
    void RefreshPrimaryIdentity();
    bool IsMapTravelBridgeBound() const;
    bool IsWorldDelegateBridgeBound() const;

    FSOTS_OnWorldStartPlay_Native OnWorldStartPlay_Native;
    FSOTS_OnPostLogin_Native OnPostLogin_Native;
    FSOTS_OnLogout_Native OnLogout_Native;
    FSOTS_OnPlayerControllerBeginPlay_Native OnPlayerControllerBeginPlay_Native;
    FSOTS_OnPawnPossessed_Native OnPawnPossessed_Native;
    FSOTS_OnHUDReady_Native OnHUDReady_Native;
    FSOTS_OnPreLoadMap_Native OnPreLoadMap_Native;
    FSOTS_OnPostLoadMap_Native OnPostLoadMap_Native;
    FSOTS_OnWorldCleanup_Native OnWorldCleanup_Native;
    FSOTS_OnPostWorldInitialization_Native OnPostWorldInitialization_Native;

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

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnPreLoadMap_BP OnPreLoadMap_BP;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnPostLoadMap_BP OnPostLoadMap_BP;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnWorldCleanup_BP OnWorldCleanup_BP;

    UPROPERTY(BlueprintAssignable, Category = "SOTS|Core|Lifecycle")
    FSOTS_OnPostWorldInitialization_BP OnPostWorldInitialization_BP;

private:
    void DispatchToListeners(TFunctionRef<void(ISOTS_CoreLifecycleListener&)> Fn, const TCHAR* EventName);
    void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS);
    void HandleWorldBeginPlay(UWorld* World);
    void HandlePreLoadMap(const FString& MapName);
    void HandlePostLoadMapWithWorld(UWorld* World);
    void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

    void ResetSnapshotForWorld(UWorld* World);
    bool ShouldLogVerbose() const;
    bool ShouldSuppressDuplicates() const;
    bool ShouldLogMapTravel() const;

    FSOTS_CoreLifecycleSnapshot Current;
    TWeakObjectPtr<APlayerController> LastPossessedPC;
    TWeakObjectPtr<APawn> LastPossessedPawn;
    TWeakObjectPtr<AHUD> LastHUDReady;
    TSet<TWeakObjectPtr<APlayerController>> BeganPlayControllers;
    bool bWorldStartBroadcasted = false;
    FString LastPreLoadMapName;
    TWeakObjectPtr<UWorld> LastPostLoadMapWorld;
    TWeakObjectPtr<UWorld> LastWorldCleanupWorld;
    TWeakObjectPtr<UWorld> LastPostWorldInitWorld;
    TWeakObjectPtr<APlayerController> LastIdentityPC;
    TWeakObjectPtr<APlayerState> LastIdentityPlayerState;
    bool bMapTravelBridgeBound = false;
    bool bWorldDelegateBridgeBound = false;

    FDelegateHandle PostWorldInitHandle;
    FDelegateHandle PreLoadMapHandle;
    FDelegateHandle PostLoadMapHandle;
    FDelegateHandle WorldCleanupHandle;
};
