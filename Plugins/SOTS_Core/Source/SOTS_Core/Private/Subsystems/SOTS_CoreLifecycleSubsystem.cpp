#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SOTS_Core.h"
#include "Lifecycle/SOTS_CoreLifecycleListener.h"
#include "Settings/SOTS_CoreSettings.h"
#include "Features/IModularFeatures.h"

void USOTS_CoreLifecycleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get())
    {
        if (Settings->bEnableWorldDelegateBridge)
        {
            PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(
                this,
                &USOTS_CoreLifecycleSubsystem::HandlePostWorldInitialization);
            WorldBeginPlayHandle = FWorldDelegates::OnWorldBeginPlay.AddUObject(
                this,
                &USOTS_CoreLifecycleSubsystem::HandleWorldBeginPlay);
        }
    }
}

void USOTS_CoreLifecycleSubsystem::Deinitialize()
{
    if (PostWorldInitHandle.IsValid())
    {
        FWorldDelegates::OnPostWorldInitialization.Remove(PostWorldInitHandle);
        PostWorldInitHandle = FDelegateHandle();
    }

    if (WorldBeginPlayHandle.IsValid())
    {
        FWorldDelegates::OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
        WorldBeginPlayHandle = FDelegateHandle();
    }

    Super::Deinitialize();
}

FSOTS_CoreLifecycleSnapshot USOTS_CoreLifecycleSubsystem::GetCurrentSnapshot() const
{
    return Current;
}

bool USOTS_CoreLifecycleSubsystem::HasPrimaryPlayerReady() const
{
    return Current.PrimaryPC != nullptr && Current.PrimaryPawn != nullptr;
}

void USOTS_CoreLifecycleSubsystem::NotifyWorldStartPlay(UWorld* World)
{
    if (!World)
    {
        return;
    }

    if (Current.World != World)
    {
        ResetSnapshotForWorld(World);
    }

    if (ShouldSuppressDuplicates() && bWorldStartBroadcasted)
    {
        return;
    }

    bWorldStartBroadcasted = true;
    Current.World = World;
    Current.GameMode = World->GetAuthGameMode();
    Current.bWorldStarted = true;

    if (ShouldLogVerbose())
    {
        UE_LOG(LogSOTS_Core, Verbose, TEXT("NotifyWorldStartPlay World=%s"), *GetNameSafe(World));
    }

    OnWorldStartPlay_Native.Broadcast(World);
    OnWorldStartPlay_BP.Broadcast(World);

    DispatchToListeners(
        [World](ISOTS_CoreLifecycleListener& Listener)
        {
            Listener.OnSOTS_WorldStartPlay(World);
        },
        TEXT("WorldStartPlay"));
}

void USOTS_CoreLifecycleSubsystem::NotifyPostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
    if (!GameMode || !NewPlayer)
    {
        return;
    }

    if (Current.World != GameMode->GetWorld())
    {
        ResetSnapshotForWorld(GameMode->GetWorld());
    }

    Current.GameMode = GameMode;
    Current.PrimaryPC = NewPlayer;
    Current.PrimaryPawn = NewPlayer->GetPawn();

    if (ShouldLogVerbose())
    {
        UE_LOG(
            LogSOTS_Core,
            Verbose,
            TEXT("NotifyPostLogin GameMode=%s PlayerController=%s"),
            *GetNameSafe(GameMode),
            *GetNameSafe(NewPlayer));
    }

    OnPostLogin_Native.Broadcast(GameMode, NewPlayer);
    OnPostLogin_BP.Broadcast(GameMode, NewPlayer);

    DispatchToListeners(
        [GameMode, NewPlayer](ISOTS_CoreLifecycleListener& Listener)
        {
            Listener.OnSOTS_PostLogin(GameMode, NewPlayer);
        },
        TEXT("PostLogin"));
}

void USOTS_CoreLifecycleSubsystem::NotifyLogout(AGameModeBase* GameMode, AController* Exiting)
{
    if (!GameMode || !Exiting)
    {
        return;
    }

    if (Current.PrimaryPC == Exiting)
    {
        Current.PrimaryPC = nullptr;
        Current.PrimaryPawn = nullptr;
    }

    if (ShouldLogVerbose())
    {
        UE_LOG(
            LogSOTS_Core,
            Verbose,
            TEXT("NotifyLogout GameMode=%s Controller=%s"),
            *GetNameSafe(GameMode),
            *GetNameSafe(Exiting));
    }

    OnLogout_Native.Broadcast(GameMode, Exiting);
    OnLogout_BP.Broadcast(GameMode, Exiting);

    DispatchToListeners(
        [GameMode, Exiting](ISOTS_CoreLifecycleListener& Listener)
        {
            Listener.OnSOTS_Logout(GameMode, Exiting);
        },
        TEXT("Logout"));
}

void USOTS_CoreLifecycleSubsystem::NotifyPlayerControllerBeginPlay(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return;
    }

    if (Current.World != PlayerController->GetWorld())
    {
        ResetSnapshotForWorld(PlayerController->GetWorld());
    }

    if (ShouldSuppressDuplicates()
        && BeganPlayControllers.Contains(TWeakObjectPtr<APlayerController>(PlayerController)))
    {
        return;
    }

    BeganPlayControllers.Add(TWeakObjectPtr<APlayerController>(PlayerController));
    Current.PrimaryPC = PlayerController;
    Current.PrimaryPawn = PlayerController->GetPawn();

    if (ShouldLogVerbose())
    {
        UE_LOG(
            LogSOTS_Core,
            Verbose,
            TEXT("NotifyPlayerControllerBeginPlay PlayerController=%s"),
            *GetNameSafe(PlayerController));
    }

    OnPlayerControllerBeginPlay_Native.Broadcast(PlayerController);
    OnPlayerControllerBeginPlay_BP.Broadcast(PlayerController);

    DispatchToListeners(
        [PlayerController](ISOTS_CoreLifecycleListener& Listener)
        {
            Listener.OnSOTS_PlayerControllerBeginPlay(PlayerController);
        },
        TEXT("PlayerControllerBeginPlay"));
}

void USOTS_CoreLifecycleSubsystem::NotifyPawnPossessed(APlayerController* PlayerController, APawn* Pawn)
{
    if (!PlayerController || !Pawn)
    {
        return;
    }

    if (Current.World != PlayerController->GetWorld())
    {
        ResetSnapshotForWorld(PlayerController->GetWorld());
    }

    if (ShouldSuppressDuplicates()
        && LastPossessedPC.Get() == PlayerController
        && LastPossessedPawn.Get() == Pawn)
    {
        return;
    }

    LastPossessedPC = PlayerController;
    LastPossessedPawn = Pawn;
    Current.PrimaryPC = PlayerController;
    Current.PrimaryPawn = Pawn;

    if (ShouldLogVerbose())
    {
        UE_LOG(
            LogSOTS_Core,
            Verbose,
            TEXT("NotifyPawnPossessed PlayerController=%s Pawn=%s"),
            *GetNameSafe(PlayerController),
            *GetNameSafe(Pawn));
    }

    OnPawnPossessed_Native.Broadcast(PlayerController, Pawn);
    OnPawnPossessed_BP.Broadcast(PlayerController, Pawn);

    DispatchToListeners(
        [PlayerController, Pawn](ISOTS_CoreLifecycleListener& Listener)
        {
            Listener.OnSOTS_PawnPossessed(PlayerController, Pawn);
        },
        TEXT("PawnPossessed"));
}

void USOTS_CoreLifecycleSubsystem::NotifyHUDReady(AHUD* HUD)
{
    if (!HUD)
    {
        return;
    }

    if (Current.World != HUD->GetWorld())
    {
        ResetSnapshotForWorld(HUD->GetWorld());
    }

    if (ShouldSuppressDuplicates() && LastHUDReady.Get() == HUD)
    {
        return;
    }

    LastHUDReady = HUD;
    Current.PrimaryHUD = HUD;
    Current.PrimaryPC = HUD->PlayerOwner;
    if (HUD->PlayerOwner)
    {
        Current.PrimaryPawn = HUD->PlayerOwner->GetPawn();
    }

    if (ShouldLogVerbose())
    {
        UE_LOG(
            LogSOTS_Core,
            Verbose,
            TEXT("NotifyHUDReady HUD=%s"),
            *GetNameSafe(HUD));
    }

    OnHUDReady_Native.Broadcast(HUD);
    OnHUDReady_BP.Broadcast(HUD);

    DispatchToListeners(
        [HUD](ISOTS_CoreLifecycleListener& Listener)
        {
            Listener.OnSOTS_HUDReady(HUD);
        },
        TEXT("HUDReady"));
}

void USOTS_CoreLifecycleSubsystem::DispatchToListeners(
    TFunctionRef<void(ISOTS_CoreLifecycleListener&)> Fn,
    const TCHAR* EventName)
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    if (!Settings || !Settings->bEnableLifecycleListenerDispatch)
    {
        return;
    }

    if (!IModularFeatures::IsAvailable())
    {
        if (Settings->bEnableLifecycleDispatchLogs)
        {
            UE_LOG(
                LogSOTS_Core,
                Verbose,
                TEXT("Lifecycle dispatch skipped (ModularFeatures unavailable). Event=%s"),
                EventName);
        }
        return;
    }

    TArray<IModularFeature*> Features;
    IModularFeatures::Get().GetModularFeatureImplementations(SOTS_CoreLifecycleListenerFeatureName, Features);

    for (IModularFeature* Feature : Features)
    {
        ISOTS_CoreLifecycleListener* Listener = static_cast<ISOTS_CoreLifecycleListener*>(Feature);
        if (!ensureMsgf(
                Listener,
                TEXT("Lifecycle dispatch encountered null listener. Event=%s"),
                EventName))
        {
            continue;
        }

        if (Settings->bEnableLifecycleDispatchLogs)
        {
            UE_LOG(
                LogSOTS_Core,
                Verbose,
                TEXT("Lifecycle dispatch Event=%s Listener=%p"),
                EventName,
                Listener);
        }

        Fn(*Listener);
    }
}

void USOTS_CoreLifecycleSubsystem::HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS)
{
    if (!World)
    {
        return;
    }

    (void)IVS;

    if (Current.World != World)
    {
        ResetSnapshotForWorld(World);
    }

    if (ShouldLogVerbose())
    {
        UE_LOG(LogSOTS_Core, Verbose, TEXT("HandlePostWorldInitialization World=%s"), *GetNameSafe(World));
    }
}

void USOTS_CoreLifecycleSubsystem::HandleWorldBeginPlay(UWorld* World)
{
    NotifyWorldStartPlay(World);
}

void USOTS_CoreLifecycleSubsystem::ResetSnapshotForWorld(UWorld* World)
{
    Current = FSOTS_CoreLifecycleSnapshot();
    Current.World = World;
    Current.GameMode = World ? World->GetAuthGameMode() : nullptr;

    LastPossessedPC = nullptr;
    LastPossessedPawn = nullptr;
    LastHUDReady = nullptr;
    BeganPlayControllers.Reset();
    bWorldStartBroadcasted = false;
}

bool USOTS_CoreLifecycleSubsystem::ShouldLogVerbose() const
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    return Settings && Settings->bEnableVerboseCoreLogs;
}

bool USOTS_CoreLifecycleSubsystem::ShouldSuppressDuplicates() const
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    return !Settings || Settings->bSuppressDuplicateNotifications;
}
