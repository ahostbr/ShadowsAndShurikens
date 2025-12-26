// SOTS_ProfileSharedModule.cpp
// Minimal runtime module for shared profile types.

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

#include "SOTS_ProfileSharedCoreBridgeSettings.h"
#include "SOTS_ProfileSubsystem.h"

#include "Lifecycle/SOTS_CoreLifecycleListener.h"
#include "Lifecycle/SOTS_CoreLifecycleListenerHandle.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_ProfileSharedModule, Log, All);

namespace SOTS::ProfileShared::CoreBridge
{
    const USOTS_ProfileSharedCoreBridgeSettings* GetSettings()
    {
        return GetDefault<USOTS_ProfileSharedCoreBridgeSettings>();
    }

    bool IsBridgeEnabled(const USOTS_ProfileSharedCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreLifecycleBridge;
    }

    bool ShouldLogVerbose(const USOTS_ProfileSharedCoreBridgeSettings* Settings)
    {
        return Settings && Settings->bEnableSOTSCoreBridgeVerboseLogs;
    }

    USOTS_ProfileSubsystem* ResolveSubsystem(const UWorld* World)
    {
        if (!World)
        {
            return nullptr;
        }

        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<USOTS_ProfileSubsystem>();
        }

        return nullptr;
    }

    USOTS_ProfileSubsystem* ResolveSubsystemFromPC(const APlayerController* PC)
    {
        if (!PC)
        {
            return nullptr;
        }

        if (UGameInstance* GameInstance = PC->GetGameInstance())
        {
            return GameInstance->GetSubsystem<USOTS_ProfileSubsystem>();
        }

        return nullptr;
    }

    class FSOTS_ProfileSharedCoreLifecycleHook final : public ISOTS_CoreLifecycleListener
    {
    public:
        void OnSOTS_WorldStartPlay(UWorld* World) override
        {
            const USOTS_ProfileSharedCoreBridgeSettings* Settings = GetSettings();
            if (!IsBridgeEnabled(Settings) || !World)
            {
                return;
            }

            if (LastWorld.Get() == World)
            {
                return;
            }

            LastWorld = World;

            if (ShouldLogVerbose(Settings))
            {
                UE_LOG(LogSOTS_ProfileSharedModule, Verbose, TEXT("ProfileShared CoreBridge: WorldStartPlay %s"), *GetNameSafe(World));
            }

            if (USOTS_ProfileSubsystem* Subsystem = ResolveSubsystem(World))
            {
                Subsystem->HandleCoreWorldStartPlay(World);
            }
        }

        void OnSOTS_PostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer) override
        {
            const USOTS_ProfileSharedCoreBridgeSettings* Settings = GetSettings();
            if (!IsBridgeEnabled(Settings) || !NewPlayer)
            {
                return;
            }

            if (LastPostLoginPC.Get() == NewPlayer)
            {
                return;
            }

            LastPostLoginPC = NewPlayer;

            if (ShouldLogVerbose(Settings))
            {
                UE_LOG(LogSOTS_ProfileSharedModule, Verbose, TEXT("ProfileShared CoreBridge: PostLogin %s"), *GetNameSafe(NewPlayer));
            }

            if (USOTS_ProfileSubsystem* Subsystem = ResolveSubsystemFromPC(NewPlayer))
            {
                Subsystem->HandleCorePostLogin(GameMode, NewPlayer);
            }
        }

        void OnSOTS_PawnPossessed(APlayerController* PC, APawn* Pawn) override
        {
            const USOTS_ProfileSharedCoreBridgeSettings* Settings = GetSettings();
            if (!IsBridgeEnabled(Settings) || !PC || !Pawn)
            {
                return;
            }

            if (LastPrimaryPC.Get() == PC && LastPrimaryPawn.Get() == Pawn)
            {
                return;
            }

            LastPrimaryPC = PC;
            LastPrimaryPawn = Pawn;

            if (ShouldLogVerbose(Settings))
            {
                UE_LOG(LogSOTS_ProfileSharedModule, Verbose, TEXT("ProfileShared CoreBridge: PawnPossessed PC=%s Pawn=%s"), *GetNameSafe(PC), *GetNameSafe(Pawn));
            }

            if (USOTS_ProfileSubsystem* Subsystem = ResolveSubsystemFromPC(PC))
            {
                Subsystem->HandleCorePrimaryPlayerReady(PC, Pawn);
            }
        }

    private:
        TWeakObjectPtr<UWorld> LastWorld;
        TWeakObjectPtr<APlayerController> LastPostLoginPC;
        TWeakObjectPtr<APlayerController> LastPrimaryPC;
        TWeakObjectPtr<APawn> LastPrimaryPawn;
    };

    FSOTS_ProfileSharedCoreLifecycleHook GHook;
    FSOTS_CoreLifecycleListenerHandle GHookHandle;

    void Register()
    {
        GHookHandle.Register(&GHook);
    }

    void Unregister()
    {
        GHookHandle.Unregister();
    }
}

/**
 * All real logic for profiles lives in the structs in SOTS_ProfileTypes.h
 * and in other systems that *use* those structs. This module just needs
 * to exist so the engine can load the plugin cleanly.
 */
class FSOTS_ProfileSharedModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogSOTS_ProfileSharedModule, Log, TEXT("SOTS_ProfileShared module starting up"));
        // IMPORTANT: Do NOT touch worlds, GameInstance, or assets here.
        SOTS::ProfileShared::CoreBridge::Register();
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogSOTS_ProfileSharedModule, Log, TEXT("SOTS_ProfileShared module shutting down"));
        SOTS::ProfileShared::CoreBridge::Unregister();
    }
};

// The module name MUST match the .uplugin Modules entry → "SOTS_ProfileShared"
IMPLEMENT_MODULE(FSOTS_ProfileSharedModule, SOTS_ProfileShared);
