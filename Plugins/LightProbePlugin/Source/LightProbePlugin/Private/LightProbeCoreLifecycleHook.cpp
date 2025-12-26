#include "LightProbeCoreLifecycleHook.h"

#include "LightProbeCoreBridgeDelegates.h"
#include "LightProbeCoreBridgeSettings.h"

#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogLightProbeCoreBridge, Log, All);

bool FLightProbe_CoreLifecycleHook::IsBridgeEnabled() const
{
    const ULightProbeCoreBridgeSettings* Settings = ULightProbeCoreBridgeSettings::Get();
    return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
}

bool FLightProbe_CoreLifecycleHook::ShouldLogVerbose() const
{
    const ULightProbeCoreBridgeSettings* Settings = ULightProbeCoreBridgeSettings::Get();
    return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
}

void FLightProbe_CoreLifecycleHook::BindToCoreTravelDelegates(UGameInstance* GameInstance)
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
            PreLoadMapHandle = Lifecycle->OnPreLoadMap_Native.AddRaw(this, &FLightProbe_CoreLifecycleHook::HandlePreLoadMap);
        }

        if (!PostLoadMapHandle.IsValid())
        {
            PostLoadMapHandle = Lifecycle->OnPostLoadMap_Native.AddRaw(this, &FLightProbe_CoreLifecycleHook::HandlePostLoadMap);
        }
    }
}

void FLightProbe_CoreLifecycleHook::UnbindCoreTravelDelegates()
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

void FLightProbe_CoreLifecycleHook::HandlePreLoadMap(const FString& MapName)
{
    LastPC.Reset();
    LastPawn.Reset();

    LightProbePlugin::CoreBridge::OnCorePreLoadMap_Native().Broadcast(MapName);

    if (ShouldLogVerbose())
    {
        UE_LOG(LogLightProbeCoreBridge, Verbose, TEXT("LightProbe CoreBridge: PreLoadMap MapName=%s"), *MapName);
    }
}

void FLightProbe_CoreLifecycleHook::HandlePostLoadMap(UWorld* World)
{
    LightProbePlugin::CoreBridge::OnCorePostLoadMap_Native().Broadcast(World);

    if (ShouldLogVerbose())
    {
        UE_LOG(LogLightProbeCoreBridge, Verbose, TEXT("LightProbe CoreBridge: PostLoadMap World=%s"), *GetNameSafe(World));
    }
}

void FLightProbe_CoreLifecycleHook::OnSOTS_WorldStartPlay(UWorld* World)
{
    if (!IsBridgeEnabled() || !World)
    {
        return;
    }

    BindToCoreTravelDelegates(World->GetGameInstance());

    LightProbePlugin::CoreBridge::OnCoreWorldReady_Native().Broadcast(World);

    if (ShouldLogVerbose())
    {
        UE_LOG(LogLightProbeCoreBridge, Verbose, TEXT("LightProbe CoreBridge: OnSOTS_WorldStartPlay World=%s"), *GetNameSafe(World));
    }
}

void FLightProbe_CoreLifecycleHook::OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn)
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

    LightProbePlugin::CoreBridge::OnCorePrimaryPlayerReady_Native().Broadcast(PC, Pawn);

    if (ShouldLogVerbose())
    {
        UE_LOG(
            LogLightProbeCoreBridge,
            Verbose,
            TEXT("LightProbe CoreBridge: OnSOTS_PawnPossessed PC=%s Pawn=%s"),
            *GetNameSafe(PC),
            *GetNameSafe(Pawn));
    }
}

void FLightProbe_CoreLifecycleHook::Shutdown()
{
    UnbindCoreTravelDelegates();
    LastPC.Reset();
    LastPawn.Reset();
}
