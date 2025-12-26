#include "SOTS_MMSSCoreLifecycleHook.h"

#include "SOTS_MMSSCoreBridgeDelegates.h"
#include "SOTS_MMSSCoreBridgeSettings.h"

#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMMSSCoreBridge, Log, All);

bool FMMSS_CoreLifecycleHook::IsBridgeEnabled() const
{
    const USOTS_MMSSCoreBridgeSettings* Settings = USOTS_MMSSCoreBridgeSettings::Get();
    return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
}

bool FMMSS_CoreLifecycleHook::ShouldLogVerbose() const
{
    const USOTS_MMSSCoreBridgeSettings* Settings = USOTS_MMSSCoreBridgeSettings::Get();
    return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FMMSS_CoreLifecycleHook::BindToCoreTravelDelegates(UGameInstance* GameInstance)
{
    if (!GameInstance)
    {
        return;
    }

    if (CachedGameInstance.Get() != GameInstance)
    {
        UnbindCoreTravelDelegates();
        CachedGameInstance = GameInstance;
    }

    if (USOTS_CoreLifecycleSubsystem* Lifecycle = GameInstance->GetSubsystem<USOTS_CoreLifecycleSubsystem>())
    {
        CachedCoreLifecycle = Lifecycle;

        if (!Lifecycle->IsMapTravelBridgeBound())
        {
            return;
        }

        if (!PreLoadMapHandle.IsValid())
        {
            PreLoadMapHandle = Lifecycle->OnPreLoadMap_Native.AddRaw(this, &FMMSS_CoreLifecycleHook::HandlePreLoadMap);
        }

        if (!PostLoadMapHandle.IsValid())
        {
            PostLoadMapHandle = Lifecycle->OnPostLoadMap_Native.AddRaw(this, &FMMSS_CoreLifecycleHook::HandlePostLoadMap);
        }
    }
}

void FMMSS_CoreLifecycleHook::UnbindCoreTravelDelegates()
{
    if (USOTS_CoreLifecycleSubsystem* Lifecycle = CachedCoreLifecycle.Get())
    {
        if (PreLoadMapHandle.IsValid())
        {
            Lifecycle->OnPreLoadMap_Native.Remove(PreLoadMapHandle);
            PreLoadMapHandle.Reset();
        }

        if (PostLoadMapHandle.IsValid())
        {
            Lifecycle->OnPostLoadMap_Native.Remove(PostLoadMapHandle);
            PostLoadMapHandle.Reset();
        }
    }

    CachedCoreLifecycle.Reset();
    CachedGameInstance.Reset();
}

void FMMSS_CoreLifecycleHook::HandlePreLoadMap(const FString& MapName)
{
    LastPC.Reset();
    LastPawn.Reset();

    SOTS_MMSS::CoreBridge::OnCorePreLoadMap_Native().Broadcast(MapName);

    if (ShouldLogVerbose())
    {
        UE_LOG(LogMMSSCoreBridge, Verbose, TEXT("MMSS CoreBridge: PreLoadMap MapName=%s"), *MapName);
    }
}

void FMMSS_CoreLifecycleHook::HandlePostLoadMap(UWorld* World)
{
    SOTS_MMSS::CoreBridge::OnCorePostLoadMap_Native().Broadcast(World);

    if (ShouldLogVerbose())
    {
        UE_LOG(LogMMSSCoreBridge, Verbose, TEXT("MMSS CoreBridge: PostLoadMap World=%s"), *GetNameSafe(World));
    }
}

void FMMSS_CoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
    if (!IsBridgeEnabled() || !World)
    {
        return;
    }

    BindToCoreTravelDelegates(World->GetGameInstance());

    SOTS_MMSS::CoreBridge::OnCoreWorldReady_Native().Broadcast(World);

    if (ShouldLogVerbose())
    {
        UE_LOG(LogMMSSCoreBridge, Verbose, TEXT("MMSS CoreBridge: OnSOTS_WorldStartPlay World=%s"), *GetNameSafe(World));
    }
}

void FMMSS_CoreLifecycleHook::OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn)
{
    if (!IsBridgeEnabled() || !PC || !Pawn)
    {
        return;
    }

    if (!PC->IsLocalController())
    {
        return;
    }

    if (LastPC.Get() == PC && LastPawn.Get() == Pawn)
    {
        return;
    }

    LastPC = PC;
    LastPawn = Pawn;

    SOTS_MMSS::CoreBridge::OnCorePrimaryPlayerReady_Native().Broadcast(PC, Pawn);

    if (ShouldLogVerbose())
    {
        UE_LOG(LogMMSSCoreBridge, Verbose, TEXT("MMSS CoreBridge: OnSOTS_PawnPossessed PC=%s Pawn=%s"), *GetNameSafe(PC), *GetNameSafe(Pawn));
    }
}

void FMMSS_CoreLifecycleHook::Shutdown()
{
    UnbindCoreTravelDelegates();
    LastPC.Reset();
    LastPawn.Reset();
}
