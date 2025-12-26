#include "Diagnostics/SOTS_CoreDiagnostics.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Features/IModularFeatures.h"
#include "Lifecycle/SOTS_CoreLifecycleListener.h"
#include "Save/SOTS_CoreSaveParticipantRegistry.h"
#include "Save/SOTS_CoreSaveParticipant.h"
#include "Settings/SOTS_CoreSettings.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"
#include "SOTS_CoreVersion.h"
#include "SOTS_Core.h"

#if PLATFORM_COMPILER_HAS_RTTI
#include <typeinfo>
#endif

namespace
{
    USOTS_CoreLifecycleSubsystem* ResolveLifecycleSubsystem(UWorld* World)
    {
        if (!World)
        {
            return nullptr;
        }

        UGameInstance* GameInstance = World->GetGameInstance();
        if (!GameInstance)
        {
            return nullptr;
        }

        return GameInstance->GetSubsystem<USOTS_CoreLifecycleSubsystem>();
    }
}

void FSOTS_CoreDiagnostics::DumpSettings()
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    if (!Settings)
    {
        UE_LOG(LogSOTS_Core, Warning, TEXT("SOTS_Core diagnostics: settings not available."));
        return;
    }

    UE_LOG(LogSOTS_Core, Log, TEXT("SOTS_Core Settings:"));
    UE_LOG(
        LogSOTS_Core,
        Log,
        TEXT("  WorldDelegateBridge=%s MapTravelBridge=%s LifecycleDispatch=%s SaveParticipantQueries=%s PrimaryIdentityCache=%s"),
        Settings->bEnableWorldDelegateBridge ? TEXT("true") : TEXT("false"),
        Settings->bEnableMapTravelBridge ? TEXT("true") : TEXT("false"),
        Settings->bEnableLifecycleListenerDispatch ? TEXT("true") : TEXT("false"),
        Settings->bEnableSaveParticipantQueries ? TEXT("true") : TEXT("false"),
        Settings->bEnablePrimaryIdentityCache ? TEXT("true") : TEXT("false"));
    UE_LOG(
        LogSOTS_Core,
        Log,
        TEXT("  VerboseCoreLogs=%s MapTravelLogs=%s LifecycleDispatchLogs=%s SaveContractLogs=%s"),
        Settings->bEnableVerboseCoreLogs ? TEXT("true") : TEXT("false"),
        Settings->bEnableMapTravelLogs ? TEXT("true") : TEXT("false"),
        Settings->bEnableLifecycleDispatchLogs ? TEXT("true") : TEXT("false"),
        Settings->bEnableSaveContractLogs ? TEXT("true") : TEXT("false"));
}

void FSOTS_CoreDiagnostics::DumpLifecycleSnapshot(UWorld* World)
{
    if (!World)
    {
        UE_LOG(LogSOTS_Core, Warning, TEXT("SOTS_Core snapshot: world is null."));
        return;
    }

    USOTS_CoreLifecycleSubsystem* Subsystem = ResolveLifecycleSubsystem(World);
    if (!Subsystem)
    {
        UE_LOG(LogSOTS_Core, Warning, TEXT("SOTS_Core snapshot: lifecycle subsystem unavailable."));
        return;
    }

    const FSOTS_CoreLifecycleSnapshot Snapshot = Subsystem->GetCurrentSnapshot();
    UE_LOG(LogSOTS_Core, Log, TEXT("SOTS_Core Snapshot:"));
    UE_LOG(
        LogSOTS_Core,
        Log,
        TEXT("  World=%s GameMode=%s Started=%s"),
        *GetNameSafe(Snapshot.World.Get()),
        *GetNameSafe(Snapshot.GameMode.Get()),
        Snapshot.bWorldStarted ? TEXT("true") : TEXT("false"));
    UE_LOG(
        LogSOTS_Core,
        Log,
        TEXT("  PrimaryPC=%s PrimaryPawn=%s PrimaryHUD=%s"),
        *GetNameSafe(Snapshot.PrimaryPC.Get()),
        *GetNameSafe(Snapshot.PrimaryPawn.Get()),
        *GetNameSafe(Snapshot.PrimaryHUD.Get()));
    UE_LOG(
        LogSOTS_Core,
        Log,
        TEXT("  Identity: Valid=%s LocalPlayerIndex=%d ControllerId=%d PlayerName=%s UniqueId=%s"),
        Snapshot.PrimaryIdentity.bIsValid ? TEXT("true") : TEXT("false"),
        Snapshot.PrimaryIdentity.LocalPlayerIndex,
        Snapshot.PrimaryIdentity.ControllerId,
        *Snapshot.PrimaryIdentity.PlayerName,
        *Snapshot.PrimaryIdentity.PlayerStateUniqueIdString);
}

void FSOTS_CoreDiagnostics::DumpRegisteredLifecycleListeners()
{
    TArray<IModularFeature*> Features = IModularFeatures::Get().GetModularFeatureImplementations<IModularFeature>(
        SOTS_CoreLifecycleListenerFeatureName);

    UE_LOG(LogSOTS_Core, Log, TEXT("SOTS_Core listeners: %d registered."), Features.Num());
    for (int32 Index = 0; Index < Features.Num(); ++Index)
    {
        UE_LOG(LogSOTS_Core, Log, TEXT("  [%d] Listener=%p"), Index, Features[Index]);
    }
}

void FSOTS_CoreDiagnostics::DumpRegisteredSaveParticipants()
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    if (!Settings || !Settings->bEnableSaveParticipantQueries)
    {
        UE_LOG(LogSOTS_Core, Warning, TEXT("SOTS_Core save participants: queries disabled."));
        return;
    }

    TArray<ISOTS_CoreSaveParticipant*> Participants;
    FSOTS_CoreSaveParticipantRegistry::GetRegisteredSaveParticipants(Participants);

    UE_LOG(LogSOTS_Core, Log, TEXT("SOTS_Core save participants: %d registered."), Participants.Num());
    for (int32 Index = 0; Index < Participants.Num(); ++Index)
    {
        ISOTS_CoreSaveParticipant* Participant = Participants[Index];
        const FName Id = Participant ? Participant->GetParticipantId() : NAME_None;
        UE_LOG(LogSOTS_Core, Log, TEXT("  [%d] ParticipantId=%s Ptr=%p"), Index, *Id.ToString(), Participant);
    }
}

void FSOTS_CoreDiagnostics::DumpBridgeHealth(UWorld* World)
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();

    UE_LOG(LogSOTS_Core, Log, TEXT("SOTS_Core Bridge Health:"));
    UE_LOG(
        LogSOTS_Core,
        Log,
        TEXT("  Version=%d.%d.%d Schema=%d ConfigSchema=%s"),
        SOTS_CORE_VER_MAJOR,
        SOTS_CORE_VER_MINOR,
        SOTS_CORE_VER_PATCH,
        SOTS_CORE_CONFIG_SCHEMA_VERSION,
        Settings ? *FString::FromInt(Settings->ConfigSchemaVersion) : TEXT("<missing>"));

    if (!Settings)
    {
        UE_LOG(LogSOTS_Core, Log, TEXT("  Settings: <missing>"));
    }
    else
    {
        UE_LOG(
            LogSOTS_Core,
            Log,
            TEXT("  Flags: LifecycleDispatch=%s WorldDelegateBridge=%s MapTravelBridge=%s SaveParticipantQueries=%s"),
            Settings->bEnableLifecycleListenerDispatch ? TEXT("true") : TEXT("false"),
            Settings->bEnableWorldDelegateBridge ? TEXT("true") : TEXT("false"),
            Settings->bEnableMapTravelBridge ? TEXT("true") : TEXT("false"),
            Settings->bEnableSaveParticipantQueries ? TEXT("true") : TEXT("false"));
    }

    {
        TArray<IModularFeature*> Features = IModularFeatures::Get().GetModularFeatureImplementations<IModularFeature>(
            SOTS_CoreLifecycleListenerFeatureName);

        UE_LOG(
            LogSOTS_Core,
            Log,
            TEXT("  LifecycleListeners: %d (FeatureName=%s)"),
            Features.Num(),
            *SOTS_CoreLifecycleListenerFeatureName.ToString());

        for (int32 Index = 0; Index < Features.Num(); ++Index)
        {
            IModularFeature* Feature = Features[Index];
            FString TypeName = TEXT("UNKNOWN");

#if PLATFORM_COMPILER_HAS_RTTI
            if (Feature)
            {
                TypeName = ANSI_TO_TCHAR(typeid(*Feature).name());
            }
#endif

            UE_LOG(
                LogSOTS_Core,
                Log,
                TEXT("    [%d] Ptr=%p Type=%s Origin=%s"),
                Index,
                Feature,
                *TypeName,
                TEXT("UNKNOWN"));
        }
    }

    {
        TArray<ISOTS_CoreSaveParticipant*> Participants;
        FSOTS_CoreSaveParticipantRegistry::GetRegisteredSaveParticipants(Participants);

        UE_LOG(
            LogSOTS_Core,
            Log,
            TEXT("  SaveParticipants: %d (FeatureName=%s QueriesEnabled=%s)"),
            Participants.Num(),
            *SOTS_CoreSaveParticipantFeatureName.ToString(),
            (Settings && Settings->bEnableSaveParticipantQueries) ? TEXT("true") : TEXT("false"));

        for (int32 Index = 0; Index < Participants.Num(); ++Index)
        {
            ISOTS_CoreSaveParticipant* Participant = Participants[Index];
            const FName Id = Participant ? Participant->GetParticipantId() : NAME_None;

            FString TypeName = TEXT("UNKNOWN");
#if PLATFORM_COMPILER_HAS_RTTI
            if (Participant)
            {
                TypeName = ANSI_TO_TCHAR(typeid(*Participant).name());
            }
#endif

            UE_LOG(
                LogSOTS_Core,
                Log,
                TEXT("    [%d] ParticipantId=%s Ptr=%p Type=%s"),
                Index,
                *Id.ToString(),
                Participant,
                *TypeName);
        }
    }

    if (!World)
    {
        UE_LOG(LogSOTS_Core, Log, TEXT("  Snapshot: <world=null>"));
        return;
    }

    USOTS_CoreLifecycleSubsystem* Subsystem = ResolveLifecycleSubsystem(World);
    if (!Subsystem)
    {
        UE_LOG(LogSOTS_Core, Log, TEXT("  Snapshot: <lifecycle-subsystem-missing>"));
        return;
    }

    const FSOTS_CoreLifecycleSnapshot Snapshot = Subsystem->GetCurrentSnapshot();
    UE_LOG(
        LogSOTS_Core,
        Log,
        TEXT("  Snapshot: World=%s Started=%s PC=%s Pawn=%s HUD=%s PrimaryReady=%s"),
        *GetNameSafe(Snapshot.World.Get()),
        Snapshot.bWorldStarted ? TEXT("true") : TEXT("false"),
        *GetNameSafe(Snapshot.PrimaryPC.Get()),
        *GetNameSafe(Snapshot.PrimaryPawn.Get()),
        *GetNameSafe(Snapshot.PrimaryHUD.Get()),
        Subsystem->HasPrimaryPlayerReady() ? TEXT("true") : TEXT("false"));
}

void FSOTS_CoreDiagnostics::PrintHealthReport(UWorld* World)
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    UE_LOG(LogSOTS_Core, Log, TEXT("SOTS_Core Health Report:"));
    if (!Settings)
    {
        UE_LOG(LogSOTS_Core, Log, TEXT("  Settings: <missing>"));
    }
    else
    {
        UE_LOG(
            LogSOTS_Core,
            Log,
            TEXT("  Settings: WorldDelegateBridge=%s MapTravelBridge=%s LifecycleDispatch=%s SaveParticipantQueries=%s"),
            Settings->bEnableWorldDelegateBridge ? TEXT("true") : TEXT("false"),
            Settings->bEnableMapTravelBridge ? TEXT("true") : TEXT("false"),
            Settings->bEnableLifecycleListenerDispatch ? TEXT("true") : TEXT("false"),
            Settings->bEnableSaveParticipantQueries ? TEXT("true") : TEXT("false"));
    }

    if (!World)
    {
        UE_LOG(LogSOTS_Core, Log, TEXT("  World: <null>"));
        return;
    }

    USOTS_CoreLifecycleSubsystem* Subsystem = ResolveLifecycleSubsystem(World);
    if (!Subsystem)
    {
        UE_LOG(LogSOTS_Core, Log, TEXT("  LifecycleSubsystem: <missing>"));
        return;
    }

    const FSOTS_CoreLifecycleSnapshot Snapshot = Subsystem->GetCurrentSnapshot();
    UE_LOG(
        LogSOTS_Core,
        Log,
        TEXT("  Snapshot: World=%s Started=%s PrimaryReady=%s IdentityValid=%s"),
        *GetNameSafe(Snapshot.World),
        Snapshot.bWorldStarted ? TEXT("true") : TEXT("false"),
        Subsystem->HasPrimaryPlayerReady() ? TEXT("true") : TEXT("false"),
        Snapshot.PrimaryIdentity.bIsValid ? TEXT("true") : TEXT("false"));

    UE_LOG(
        LogSOTS_Core,
        Log,
        TEXT("  Listeners=%d SaveParticipants=%d"),
        GetRegisteredLifecycleListenerCount(),
        GetRegisteredSaveParticipantCount());

    ValidateCoreConfiguration(World);
}

void FSOTS_CoreDiagnostics::ValidateCoreConfiguration(UWorld* World)
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    if (!Settings)
    {
        UE_LOG(LogSOTS_Core, Warning, TEXT("SOTS_Core diagnostics: settings not available."));
        return;
    }

    if (Settings->bEnableMapTravelBridge)
    {
        USOTS_CoreLifecycleSubsystem* Subsystem = ResolveLifecycleSubsystem(World);
        if (!Subsystem)
        {
            UE_LOG(
                LogSOTS_Core,
                Warning,
                TEXT("SOTS_Core: MapTravelBridge enabled but lifecycle subsystem unavailable; cannot verify delegate bindings."));
        }
        else if (!Subsystem->IsMapTravelBridgeBound())
        {
            UE_LOG(
                LogSOTS_Core,
                Warning,
                TEXT("SOTS_Core: MapTravelBridge enabled but delegate bindings are inactive."));
        }
    }

    if (Settings->bEnableWorldDelegateBridge)
    {
        USOTS_CoreLifecycleSubsystem* Subsystem = ResolveLifecycleSubsystem(World);
        if (!Subsystem)
        {
            UE_LOG(
                LogSOTS_Core,
                Warning,
                TEXT("SOTS_Core: WorldDelegateBridge enabled but lifecycle subsystem unavailable; cannot verify delegate bindings."));
        }
        else if (!Subsystem->IsWorldDelegateBridgeBound())
        {
            UE_LOG(
                LogSOTS_Core,
                Warning,
                TEXT("SOTS_Core: WorldDelegateBridge enabled but delegate bindings are inactive."));
        }
    }

    if (Settings->bEnableLifecycleListenerDispatch)
    {
        const int32 ListenerCount = GetRegisteredLifecycleListenerCount();
        if (ListenerCount == 0)
        {
            UE_LOG(
                LogSOTS_Core,
                Verbose,
                TEXT("SOTS_Core: lifecycle dispatch enabled but no listeners registered."));
        }
    }

    if (!Settings->bSuppressDuplicateNotifications)
    {
        static bool bWarnedDuplicateSuppression = false;
        if (!bWarnedDuplicateSuppression)
        {
            UE_LOG(
                LogSOTS_Core,
                Warning,
                TEXT("SOTS_Core: duplicate suppression disabled; lifecycle notifications may fire multiple times."));
            bWarnedDuplicateSuppression = true;
        }
    }

    if (!Settings->bSuppressDuplicateNotifications
        && (Settings->bEnableWorldDelegateBridge || Settings->bEnableMapTravelBridge))
    {
        UE_LOG(
            LogSOTS_Core,
            Warning,
            TEXT("SOTS_Core: delegate bridges enabled while duplicate suppression is disabled; double-fire risk exists."));
    }
}

int32 FSOTS_CoreDiagnostics::GetRegisteredLifecycleListenerCount()
{
    return IModularFeatures::Get().GetModularFeatureImplementations<IModularFeature>(
        SOTS_CoreLifecycleListenerFeatureName)
        .Num();
}

int32 FSOTS_CoreDiagnostics::GetRegisteredSaveParticipantCount()
{
    const USOTS_CoreSettings* Settings = USOTS_CoreSettings::Get();
    if (!Settings || !Settings->bEnableSaveParticipantQueries)
    {
        return 0;
    }

    TArray<ISOTS_CoreSaveParticipant*> Participants;
    FSOTS_CoreSaveParticipantRegistry::GetRegisteredSaveParticipants(Participants);
    return Participants.Num();
}
