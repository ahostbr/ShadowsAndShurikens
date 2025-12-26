#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "SOTS_Core.h"
#include "Lifecycle/SOTS_CoreLifecycleListener.h"
#include "Settings/SOTS_CoreSettings.h"
#include "Features/IModularFeatures.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/LocalPlayer.h"

void USOTS_CoreLifecycleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bMapTravelBridgeBound = false;
    bWorldDelegateBridgeBound = false;

    if (const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get())
    {
        if (Settings->bEnableWorldDelegateBridge || Settings->bEnableMapTravelBridge)
        {
            if (PostWorldInitHandle.IsValid())
            {
                ensureMsgf(false, TEXT("SOTS_CoreLifecycleSubsystem: PostWorldInitialization already bound."));
            }
            else
            {
                PostWorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(
                    this,
                    &USOTS_CoreLifecycleSubsystem::HandlePostWorldInitialization);
            }
        }

        if (Settings->bEnableWorldDelegateBridge)
        {
            bWorldDelegateBridgeBound = PostWorldInitHandle.IsValid();
        }

        if (Settings->bEnableMapTravelBridge)
        {
            if (PreLoadMapHandle.IsValid())
            {
                ensureMsgf(false, TEXT("SOTS_CoreLifecycleSubsystem: PreLoadMap already bound."));
            }
            else
            {
                PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(
                    this,
                    &USOTS_CoreLifecycleSubsystem::HandlePreLoadMap);
            }

            if (PostLoadMapHandle.IsValid())
            {
                ensureMsgf(false, TEXT("SOTS_CoreLifecycleSubsystem: PostLoadMapWithWorld already bound."));
            }
            else
            {
                PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
                    this,
                    &USOTS_CoreLifecycleSubsystem::HandlePostLoadMapWithWorld);
            }

            if (WorldCleanupHandle.IsValid())
            {
                ensureMsgf(false, TEXT("SOTS_CoreLifecycleSubsystem: WorldCleanup already bound."));
            }
            else
            {
                WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
                    this,
                    &USOTS_CoreLifecycleSubsystem::HandleWorldCleanup);
            }

            bMapTravelBridgeBound = PreLoadMapHandle.IsValid()
                && PostLoadMapHandle.IsValid()
                && WorldCleanupHandle.IsValid()
                && PostWorldInitHandle.IsValid();
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

    if (PreLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
        PreLoadMapHandle = FDelegateHandle();
    }

    if (PostLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
        PostLoadMapHandle = FDelegateHandle();
    }

    if (WorldCleanupHandle.IsValid())
    {
        FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
        WorldCleanupHandle = FDelegateHandle();
    }

    bMapTravelBridgeBound = false;
    bWorldDelegateBridgeBound = false;

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

bool USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound() const
{
    return bMapTravelBridgeBound;
}

bool USOTS_CoreLifecycleSubsystem::IsWorldDelegateBridgeBound() const
{
    return bWorldDelegateBridgeBound;
}

FSOTS_PrimaryPlayerIdentity USOTS_CoreLifecycleSubsystem::BuildPrimaryIdentity() const
{
    FSOTS_PrimaryPlayerIdentity Identity;

    APlayerController* PlayerController = Current.PrimaryPC;
    if (!PlayerController)
    {
        return Identity;
    }

    Identity.bIsValid = true;

    if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
    {
        Identity.LocalPlayerIndex = LocalPlayer->GetLocalPlayerIndex();
        Identity.ControllerId = LocalPlayer->GetControllerId();
    }

    if (APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>())
    {
        Identity.PlayerName = PlayerState->GetPlayerName();

        const FUniqueNetIdRepl& UniqueId = PlayerState->GetUniqueId();
        if (UniqueId.IsValid())
        {
            Identity.PlayerStateUniqueIdString = TEXT("Valid");
        }
    }

    return Identity;
}

void USOTS_CoreLifecycleSubsystem::RefreshPrimaryIdentity()
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    if (!Settings || !Settings->bEnablePrimaryIdentityCache)
    {
        return;
    }

    APlayerController* PlayerController = Current.PrimaryPC;
    APlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<APlayerState>() : nullptr;

    if (LastIdentityPC.Get() == PlayerController && LastIdentityPlayerState.Get() == PlayerState)
    {
        return;
    }

    LastIdentityPC = PlayerController;
    LastIdentityPlayerState = PlayerState;
    Current.PrimaryIdentity = BuildPrimaryIdentity();
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
    RefreshPrimaryIdentity();

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
    RefreshPrimaryIdentity();

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
    RefreshPrimaryIdentity();

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
    RefreshPrimaryIdentity();

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

void USOTS_CoreLifecycleSubsystem::NotifyPreLoadMap(const FString& MapName)
{
    if (ShouldSuppressDuplicates() && MapName == LastPreLoadMapName)
    {
        return;
    }

    LastPreLoadMapName = MapName;

    if (ShouldLogMapTravel())
    {
        UE_LOG(LogSOTS_Core, Verbose, TEXT("NotifyPreLoadMap MapName=%s"), *MapName);
    }

    OnPreLoadMap_Native.Broadcast(MapName);
    OnPreLoadMap_BP.Broadcast(MapName);
}

void USOTS_CoreLifecycleSubsystem::NotifyPostLoadMap(UWorld* World)
{
    if (!World)
    {
        return;
    }

    if (ShouldSuppressDuplicates() && LastPostLoadMapWorld.Get() == World)
    {
        return;
    }

    ResetSnapshotForWorld(World);
    LastPostLoadMapWorld = World;

    if (ShouldLogMapTravel())
    {
        UE_LOG(LogSOTS_Core, Verbose, TEXT("NotifyPostLoadMap World=%s"), *GetNameSafe(World));
    }

    OnPostLoadMap_Native.Broadcast(World);
    OnPostLoadMap_BP.Broadcast(World);
}

void USOTS_CoreLifecycleSubsystem::NotifyWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    if (!World)
    {
        return;
    }

    if (ShouldSuppressDuplicates() && LastWorldCleanupWorld.Get() == World)
    {
        return;
    }

    ResetSnapshotForWorld(nullptr);
    LastWorldCleanupWorld = World;

    if (ShouldLogMapTravel())
    {
        UE_LOG(
            LogSOTS_Core,
            Verbose,
            TEXT("NotifyWorldCleanup World=%s SessionEnded=%s CleanupResources=%s"),
            *GetNameSafe(World),
            bSessionEnded ? TEXT("true") : TEXT("false"),
            bCleanupResources ? TEXT("true") : TEXT("false"));
    }

    OnWorldCleanup_Native.Broadcast(World, bSessionEnded, bCleanupResources);
    OnWorldCleanup_BP.Broadcast(World, bSessionEnded, bCleanupResources);
}

void USOTS_CoreLifecycleSubsystem::NotifyPostWorldInitialization(UWorld* World)
{
    if (!World)
    {
        return;
    }

    if (ShouldSuppressDuplicates() && LastPostWorldInitWorld.Get() == World)
    {
        return;
    }

    ResetSnapshotForWorld(World);
    LastPostWorldInitWorld = World;

    if (ShouldLogMapTravel())
    {
        UE_LOG(LogSOTS_Core, Verbose, TEXT("NotifyPostWorldInitialization World=%s"), *GetNameSafe(World));
    }

    OnPostWorldInitialization_Native.Broadcast(World);
    OnPostWorldInitialization_BP.Broadcast(World);
}

void USOTS_CoreLifecycleSubsystem::DispatchToListeners(
    TFunctionRef<void(ISOTS_CoreLifecycleListener&)> Fn,
    const TCHAR* EventName)
{
    if (!ensureMsgf(EventName, TEXT("SOTS_Core lifecycle dispatch missing event name.")))
    {
        EventName = TEXT("Unknown");
    }

    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    if (!Settings || !Settings->bEnableLifecycleListenerDispatch)
    {
        return;
    }

    TArray<IModularFeature*> Features = IModularFeatures::Get().GetModularFeatureImplementations<IModularFeature>(
        SOTS_CoreLifecycleListenerFeatureName);

    for (IModularFeature* Feature : Features)
    {
        if (!ensureMsgf(Feature, TEXT("Lifecycle dispatch encountered null feature. Event=%s"), EventName))
        {
            continue;
        }

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

    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    if (Settings && Settings->bEnableMapTravelBridge)
    {
        NotifyPostWorldInitialization(World);
        return;
    }

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

void USOTS_CoreLifecycleSubsystem::HandlePreLoadMap(const FString& MapName)
{
    NotifyPreLoadMap(MapName);
}

void USOTS_CoreLifecycleSubsystem::HandlePostLoadMapWithWorld(UWorld* World)
{
    NotifyPostLoadMap(World);
}

void USOTS_CoreLifecycleSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    NotifyWorldCleanup(World, bSessionEnded, bCleanupResources);
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
    LastPreLoadMapName.Reset();
    LastPostLoadMapWorld = nullptr;
    LastWorldCleanupWorld = nullptr;
    LastPostWorldInitWorld = nullptr;
    LastIdentityPC = nullptr;
    LastIdentityPlayerState = nullptr;
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

bool USOTS_CoreLifecycleSubsystem::ShouldLogMapTravel() const
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    return Settings && Settings->bEnableMapTravelLogs;
}
