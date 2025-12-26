#include "SOTS_MissionDirectorCoreLifecycleHook.h"

#include "SOTS_MissionDirectorCoreBridgeSettings.h"
#include "SOTS_MissionDirectorModule.h"
#include "SOTS_MissionDirectorSubsystem.h"

#include "Settings/SOTS_CoreSettings.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

bool FMissionDirector_CoreLifecycleHook::IsBridgeEnabled() const
{
    const USOTS_MissionDirectorCoreBridgeSettings* Settings = USOTS_MissionDirectorCoreBridgeSettings::Get();
    return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
}

bool FMissionDirector_CoreLifecycleHook::ShouldLogVerbose() const
{
    const USOTS_MissionDirectorCoreBridgeSettings* Settings = USOTS_MissionDirectorCoreBridgeSettings::Get();
    return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FMissionDirector_CoreLifecycleHook::CacheSubsystemsFromWorld(UWorld* World)
{
    if (!World)
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    if (!CachedMissionDirector.IsValid() || CachedMissionDirector->GetGameInstance() != GameInstance)
    {
        CachedMissionDirector = GameInstance->GetSubsystem<USOTS_MissionDirectorSubsystem>();
    }

    if (!CachedCoreLifecycle.IsValid() || CachedCoreLifecycle->GetGameInstance() != GameInstance)
    {
        CachedCoreLifecycle = GameInstance->GetSubsystem<USOTS_CoreLifecycleSubsystem>();
    }
}

void FMissionDirector_CoreLifecycleHook::BindTravelDelegatesIfEnabled()
{
    USOTS_CoreLifecycleSubsystem* CoreLifecycle = CachedCoreLifecycle.Get();
    if (!CoreLifecycle)
    {
        return;
    }

    const USOTS_CoreSettings* CoreSettings = USOTS_CoreSettings::Get();
    if (!CoreSettings || !CoreSettings->bEnableMapTravelBridge)
    {
        return;
    }

    if (!PreLoadMapHandle.IsValid())
    {
        PreLoadMapHandle = CoreLifecycle->OnPreLoadMap_Native.AddRaw(this, &FMissionDirector_CoreLifecycleHook::HandleCorePreLoadMap);
    }

    if (!PostLoadMapHandle.IsValid())
    {
        PostLoadMapHandle = CoreLifecycle->OnPostLoadMap_Native.AddRaw(this, &FMissionDirector_CoreLifecycleHook::HandleCorePostLoadMap);
    }
}

void FMissionDirector_CoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
    if (!IsBridgeEnabled())
    {
        return;
    }

    CacheSubsystemsFromWorld(World);

    if (USOTS_MissionDirectorSubsystem* MissionDirector = CachedMissionDirector.Get())
    {
        MissionDirector->HandleCoreWorldStartPlay(World);
    }

    BindTravelDelegatesIfEnabled();

    if (ShouldLogVerbose())
    {
        UE_LOG(LogSOTSMissionDirector, Verbose, TEXT("MissionDirector CoreBridge: OnSOTS_WorldStartPlay World=%s"), *GetNameSafe(World));
    }
}

void FMissionDirector_CoreLifecycleHook::OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn)
{
    if (!IsBridgeEnabled())
    {
        return;
    }

    UWorld* World = PC ? PC->GetWorld() : nullptr;
    CacheSubsystemsFromWorld(World);

    if (USOTS_MissionDirectorSubsystem* MissionDirector = CachedMissionDirector.Get())
    {
        MissionDirector->HandleCorePrimaryPlayerReady(PC, Pawn);
    }

    BindTravelDelegatesIfEnabled();

    if (ShouldLogVerbose())
    {
        UE_LOG(
            LogSOTSMissionDirector,
            Verbose,
            TEXT("MissionDirector CoreBridge: OnSOTS_PawnPossessed PC=%s Pawn=%s"),
            *GetNameSafe(PC),
            *GetNameSafe(Pawn));
    }
}

void FMissionDirector_CoreLifecycleHook::HandleCorePreLoadMap(const FString& MapName)
{
    if (!IsBridgeEnabled())
    {
        return;
    }

    if (USOTS_MissionDirectorSubsystem* MissionDirector = CachedMissionDirector.Get())
    {
        MissionDirector->HandleCorePreLoadMap(MapName);
    }

    if (ShouldLogVerbose())
    {
        UE_LOG(LogSOTSMissionDirector, Verbose, TEXT("MissionDirector CoreBridge: PreLoadMap MapName=%s"), *MapName);
    }
}

void FMissionDirector_CoreLifecycleHook::HandleCorePostLoadMap(UWorld* World)
{
    if (!IsBridgeEnabled())
    {
        return;
    }

    CacheSubsystemsFromWorld(World);

    if (USOTS_MissionDirectorSubsystem* MissionDirector = CachedMissionDirector.Get())
    {
        MissionDirector->HandleCorePostLoadMap(World);
    }

    if (ShouldLogVerbose())
    {
        UE_LOG(LogSOTSMissionDirector, Verbose, TEXT("MissionDirector CoreBridge: PostLoadMap World=%s"), *GetNameSafe(World));
    }
}

void FMissionDirector_CoreLifecycleHook::Shutdown()
{
    if (USOTS_CoreLifecycleSubsystem* CoreLifecycle = CachedCoreLifecycle.Get())
    {
        if (PreLoadMapHandle.IsValid())
        {
            CoreLifecycle->OnPreLoadMap_Native.Remove(PreLoadMapHandle);
            PreLoadMapHandle = FDelegateHandle();
        }

        if (PostLoadMapHandle.IsValid())
        {
            CoreLifecycle->OnPostLoadMap_Native.Remove(PostLoadMapHandle);
            PostLoadMapHandle = FDelegateHandle();
        }
    }

    CachedMissionDirector.Reset();
    CachedCoreLifecycle.Reset();
}
