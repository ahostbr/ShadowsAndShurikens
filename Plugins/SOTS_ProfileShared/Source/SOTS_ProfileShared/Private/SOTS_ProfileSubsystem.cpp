#include "SOTS_ProfileSubsystem.h"

#include "Algo/Sort.h"
#include "Components/ActorComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Math/Transform.h"
#include "SOTS_ProfileSaveGame.h"
#include "UObject/SoftObjectPath.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTSProfileShared, Log, All);

namespace
{
    bool TryResolveMissionDirectorTotalPlaySeconds(const UObject* WorldContextObject, float& OutSeconds)
    {
        OutSeconds = 0.0f;

        if (!WorldContextObject)
        {
            return false;
        }

        const UWorld* World = WorldContextObject->GetWorld();
        if (!World)
        {
            return false;
        }

        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            static const FSoftClassPath MissionDirectorPath(TEXT("/Script/SOTS_MissionDirector.SOTS_MissionDirectorSubsystem"));
            if (UClass* MissionDirectorClass = MissionDirectorPath.ResolveClass())
            {
                if (UObject* MissionDirector = GameInstance->GetSubsystemBase(MissionDirectorClass))
                {
                    static const FName FuncName(TEXT("GetTotalPlaySeconds"));
                    if (UFunction* Func = MissionDirector->FindFunction(FuncName))
                    {
                        struct FParams
                        {
                            float ReturnValue = 0.0f;
                        };

                        FParams Params;
                        MissionDirector->ProcessEvent(Func, &Params);
                        OutSeconds = Params.ReturnValue;
                        return true;
                    }
                }
            }
        }

        return false;
    }
}

FString USOTS_ProfileSubsystem::GetSlotNameForProfile(const FSOTS_ProfileId& ProfileId) const
{
    return FString::Printf(TEXT("SOTS_Profile_%d_%s"),
        ProfileId.SlotIndex,
        *ProfileId.ProfileName.ToString());
}

void USOTS_ProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    StartAutosaveTimer();
}

void USOTS_ProfileSubsystem::Deinitialize()
{
    StopAutosaveTimer();
    Super::Deinitialize();
}

bool USOTS_ProfileSubsystem::SaveProfile(const FSOTS_ProfileId& ProfileId, const FSOTS_ProfileSnapshot& Snapshot)
{
    if (IsSaveBlockedByKEM())
    {
        UE_LOG(LogSOTSProfileShared, Warning, TEXT("[ProfileSubsystem] SaveProfile blocked: KEM active."));
        return false;
    }

    const FString SlotName = GetSlotNameForProfile(ProfileId);
    constexpr int32 UserIndex = 0;

    FSOTS_ProfileSnapshot SnapshotToSave = Snapshot;
    SnapshotToSave.Meta.Id = ProfileId;
    if (SnapshotToSave.Meta.DisplayName.IsEmpty())
    {
        SnapshotToSave.Meta.DisplayName = ProfileId.ProfileName.ToString();
    }
    SnapshotToSave.Meta.LastPlayedUtc = FDateTime::UtcNow();
    SnapshotToSave.SnapshotVersion = SOTS_PROFILE_SHARED_CURRENT_SNAPSHOT_VERSION;

    if (USOTS_ProfileSaveGame* SaveGame = Cast<USOTS_ProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(USOTS_ProfileSaveGame::StaticClass())))
    {
        SaveGame->Snapshot = SnapshotToSave;
        if (UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex))
        {
            ActiveProfileId = ProfileId;
            return true;
        }
    }

    UE_LOG(LogSOTSProfileShared, Warning, TEXT("[ProfileSubsystem] SaveProfile failed for slot %s"), *SlotName);
    return false;
}

bool USOTS_ProfileSubsystem::LoadProfile(const FSOTS_ProfileId& ProfileId, FSOTS_ProfileSnapshot& OutSnapshot)
{
    const FString SlotName = GetSlotNameForProfile(ProfileId);
    constexpr int32 UserIndex = 0;

    if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        UE_LOG(LogSOTSProfileShared, Verbose, TEXT("[ProfileSubsystem] LoadProfile: no save for slot %s"), *SlotName);
        return false;
    }

    if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex))
    {
        if (USOTS_ProfileSaveGame* ProfileSave = Cast<USOTS_ProfileSaveGame>(Loaded))
        {
            OutSnapshot = ProfileSave->Snapshot;
            OutSnapshot.Meta.Id = ProfileId;
            ActiveProfileId = ProfileId;
            if (OutSnapshot.Meta.DisplayName.IsEmpty())
            {
                OutSnapshot.Meta.DisplayName = ProfileId.ProfileName.ToString();
            }
            return true;
        }
    }

    UE_LOG(LogSOTSProfileShared, Warning, TEXT("[ProfileSubsystem] LoadProfile failed for slot %s"), *SlotName);
    return false;
}

void USOTS_ProfileSubsystem::BuildSnapshotFromWorld(FSOTS_ProfileSnapshot& OutSnapshot)
{
    OutSnapshot.Meta.LastPlayedUtc = FDateTime::UtcNow();
    if (OutSnapshot.Meta.DisplayName.IsEmpty() && !OutSnapshot.Meta.Id.ProfileName.IsNone())
    {
        OutSnapshot.Meta.DisplayName = OutSnapshot.Meta.Id.ProfileName.ToString();
    }

    OutSnapshot.SnapshotVersion = SOTS_PROFILE_SHARED_CURRENT_SNAPSHOT_VERSION;
    GatherPlayerSnapshot(OutSnapshot);

    float TotalPlaySeconds = 0.0f;
    if (TryResolveMissionDirectorTotalPlaySeconds(this, TotalPlaySeconds))
    {
        OutSnapshot.Meta.TotalPlaySeconds = FMath::Max(0, FMath::RoundToInt(TotalPlaySeconds));
    }

    InvokeProviderBuild(OutSnapshot);
}

void USOTS_ProfileSubsystem::ApplySnapshotToWorld(const FSOTS_ProfileSnapshot& Snapshot)
{
    RestorePlayerFromSnapshot(Snapshot);
    InvokeProviderApply(Snapshot);
    OnProfileRestored.Broadcast(Snapshot);
}

void USOTS_ProfileSubsystem::RegisterProvider(UObject* Provider, int32 Priority)
{
    if (!Provider)
    {
        return;
    }

    PruneInvalidProviders();

    const TWeakObjectPtr<UObject> ProviderPtr(Provider);
    for (FSOTS_ProfileProviderEntry& Entry : Providers)
    {
        if (Entry.Provider == ProviderPtr)
        {
            Entry.Priority = Priority;
            SortProviders();
            return;
        }
    }

    FSOTS_ProfileProviderEntry NewEntry;
    NewEntry.Provider = ProviderPtr;
    NewEntry.Priority = Priority;
    NewEntry.Sequence = NextProviderSequence++;
    Providers.Add(NewEntry);
    SortProviders();
}

void USOTS_ProfileSubsystem::UnregisterProvider(UObject* Provider)
{
    if (!Provider)
    {
        return;
    }

    const TWeakObjectPtr<UObject> ProviderPtr(Provider);
    Providers.RemoveAll([ProviderPtr](const FSOTS_ProfileProviderEntry& Entry)
    {
        return Entry.Provider == ProviderPtr;
    });
}

void USOTS_ProfileSubsystem::SOTS_DumpCurrentPlayerStatsSnapshot()
{
    FSOTS_CharacterStateData Snapshot;
    APawn* Pawn = nullptr;
    if (TryGetPlayerPawn(Pawn) && Pawn)
    {
        BuildOptionalStats(Pawn, Snapshot);
        if (Snapshot.StatValues.Num() == 0)
        {
            UE_LOG(LogSOTSProfileShared, Log, TEXT("ProfileStats: no stats available"));
        }
        else
        {
            for (const auto& Pair : Snapshot.StatValues)
            {
                UE_LOG(LogSOTSProfileShared, Log, TEXT("ProfileStats: Tag=%s Value=%.2f"), *Pair.Key.ToString(), Pair.Value);
            }
        }
        return;
    }

    UE_LOG(LogSOTSProfileShared, Warning, TEXT("SOTS_DumpCurrentPlayerStatsSnapshot: player pawn unavailable."));
}

void USOTS_ProfileSubsystem::GatherPlayerSnapshot(FSOTS_ProfileSnapshot& OutSnapshot) const
{
    OutSnapshot.PlayerCharacter.Transform = FTransform::Identity;
    OutSnapshot.PlayerCharacter.StatValues.Reset();

    APawn* Pawn = nullptr;
    if (TryGetPlayerPawn(Pawn) && Pawn)
    {
        OutSnapshot.PlayerCharacter.Transform = Pawn->GetActorTransform();
        BuildOptionalStats(Pawn, OutSnapshot.PlayerCharacter);
    }
}

void USOTS_ProfileSubsystem::RestorePlayerFromSnapshot(const FSOTS_ProfileSnapshot& Snapshot) const
{
    APawn* Pawn = nullptr;
    if (TryGetPlayerPawn(Pawn) && Pawn)
    {
        if (!Snapshot.PlayerCharacter.Transform.ContainsNaN())
        {
            Pawn->SetActorTransform(Snapshot.PlayerCharacter.Transform);
        }
        else
        {
            UE_LOG(LogSOTSProfileShared, Warning, TEXT("[ProfileSubsystem] Snapshot transform invalid; skipping actor transform restore."));
        }
        ApplyOptionalStats(Pawn, Snapshot.PlayerCharacter);
    }
}

void USOTS_ProfileSubsystem::BuildOptionalStats(AActor* Actor, FSOTS_CharacterStateData& OutState) const
{
    if (!Actor)
    {
        OutState.StatValues.Reset();
        return;
    }

    if (UActorComponent* StatsComponent = FindStatsComponent(Actor))
    {
        static const FName BuildFuncName(TEXT("BuildCharacterStateData"));
        if (UFunction* BuildFunc = StatsComponent->FindFunction(BuildFuncName))
        {
            struct FBuildParams
            {
                FSOTS_CharacterStateData OutState;
            };

            FBuildParams Params;
            Params.OutState = OutState;
            StatsComponent->ProcessEvent(BuildFunc, &Params);
            OutState = Params.OutState;
        }
    }
    else
    {
        OutState.StatValues.Reset();
    }
}

void USOTS_ProfileSubsystem::ApplyOptionalStats(AActor* Actor, const FSOTS_CharacterStateData& InState) const
{
    if (!Actor)
    {
        return;
    }

    if (UActorComponent* StatsComponent = FindStatsComponent(Actor))
    {
        static const FName ApplyFuncName(TEXT("ApplyCharacterStateData"));
        if (UFunction* ApplyFunc = StatsComponent->FindFunction(ApplyFuncName))
        {
            struct FApplyParams
            {
                FSOTS_CharacterStateData InState;
            };

            FApplyParams Params;
            Params.InState = InState;
            StatsComponent->ProcessEvent(ApplyFunc, &Params);
        }
    }
}

void USOTS_ProfileSubsystem::InvokeProviderBuild(FSOTS_ProfileSnapshot& InOutSnapshot)
{
    PruneInvalidProviders();
    if (Providers.Num() == 0 && !bLoggedMissingProvidersOnce)
    {
        UE_LOG(LogSOTSProfileShared, Verbose, TEXT("[ProfileSubsystem] No snapshot providers registered."));
        bLoggedMissingProvidersOnce = true;
    }
    for (const FSOTS_ProfileProviderEntry& Entry : Providers)
    {
        UObject* ProviderObj = Entry.Provider.Get();
        if (!ProviderObj)
        {
            continue;
        }

        if (ISOTS_ProfileSnapshotProvider* Provider = Cast<ISOTS_ProfileSnapshotProvider>(ProviderObj))
        {
            Provider->BuildProfileSnapshot(InOutSnapshot);
        }
    }
}

void USOTS_ProfileSubsystem::InvokeProviderApply(const FSOTS_ProfileSnapshot& Snapshot)
{
    PruneInvalidProviders();
    if (Providers.Num() == 0 && !bLoggedMissingProvidersOnce)
    {
        UE_LOG(LogSOTSProfileShared, Verbose, TEXT("[ProfileSubsystem] No snapshot providers registered."));
        bLoggedMissingProvidersOnce = true;
    }
    for (const FSOTS_ProfileProviderEntry& Entry : Providers)
    {
        UObject* ProviderObj = Entry.Provider.Get();
        if (!ProviderObj)
        {
            continue;
        }

        if (ISOTS_ProfileSnapshotProvider* Provider = Cast<ISOTS_ProfileSnapshotProvider>(ProviderObj))
        {
            Provider->ApplyProfileSnapshot(Snapshot);
        }
    }
}

void USOTS_ProfileSubsystem::PruneInvalidProviders()
{
    Providers.RemoveAll([](const FSOTS_ProfileProviderEntry& Entry)
    {
        return !Entry.Provider.IsValid();
    });
}

void USOTS_ProfileSubsystem::SortProviders()
{
    Algo::Sort(Providers, [](const FSOTS_ProfileProviderEntry& A, const FSOTS_ProfileProviderEntry& B)
    {
        if (A.Priority != B.Priority)
        {
            return A.Priority > B.Priority;
        }
        return A.Sequence < B.Sequence;
    });
    UpdateProviderResolutionCache();
}

void USOTS_ProfileSubsystem::UpdateProviderResolutionCache()
{
    const int32 ProviderCount = Providers.Num();
    FString PrimaryName;
    int32 PrimaryPriority = 0;

    if (ProviderCount > 0)
    {
        if (UObject* ProviderObj = Providers[0].Provider.Get())
        {
            PrimaryName = ProviderObj->GetClass() ? ProviderObj->GetClass()->GetName() : FString();
            PrimaryPriority = Providers[0].Priority;
        }
    }

    if (ProviderCount > 1 &&
        (PrimaryName != CachedPrimaryProviderName ||
         PrimaryPriority != CachedPrimaryProviderPriority ||
         ProviderCount != CachedProviderCount))
    {
        UE_LOG(LogSOTSProfileShared, Verbose,
            TEXT("[ProfileSubsystem] Provider arbitration resolved to '%s' (Priority=%d, Providers=%d)."),
            *PrimaryName,
            PrimaryPriority,
            ProviderCount);
    }

    CachedPrimaryProviderName = PrimaryName;
    CachedPrimaryProviderPriority = PrimaryPriority;
    CachedProviderCount = ProviderCount;
}

UActorComponent* USOTS_ProfileSubsystem::FindStatsComponent(AActor* Actor) const
{
    if (!Actor)
    {
        return nullptr;
    }

    static const FString StatsComponentName(TEXT("SOTS_StatsComponent"));

    TInlineComponentArray<UActorComponent*> Components;
    Actor->GetComponents(Components);
    for (UActorComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

        const FString ClassName = Component->GetClass() ? Component->GetClass()->GetName() : FString();
        if (ClassName.Equals(StatsComponentName, ESearchCase::IgnoreCase) || ClassName.Contains(StatsComponentName, ESearchCase::IgnoreCase))
        {
            return Component;
        }
    }

    return nullptr;
}

bool USOTS_ProfileSubsystem::TryGetPlayerPawn(APawn*& OutPawn) const
{
    OutPawn = nullptr;
    if (const UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            OutPawn = PC->GetPawn();
        }
    }
    return OutPawn != nullptr;
}

bool USOTS_ProfileSubsystem::IsSaveBlockedByKEM() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return false;
    }

    static const FSoftClassPath KEMPath(TEXT("/Script/SOTS_KillExecutionManager.SOTS_KEMManagerSubsystem"));
    UClass* KEMClass = KEMPath.ResolveClass();
    if (!KEMClass)
    {
        KEMClass = KEMPath.TryLoadClass<UClass>();
    }

    if (!KEMClass)
    {
        return false;
    }

    if (UObject* KEM = GameInstance->GetSubsystemBase(KEMClass))
    {
        static const FName IsSaveBlockedName(TEXT("IsSaveBlocked"));
        if (UFunction* Func = KEM->FindFunction(IsSaveBlockedName))
        {
            struct FParams
            {
                bool ReturnValue = false;
            };

            FParams Params;
            KEM->ProcessEvent(Func, &Params);
            return Params.ReturnValue;
        }
    }

    return false;
}

bool USOTS_ProfileSubsystem::IsSaveBlockedForUI(FText& OutReason) const
{
    if (IsSaveBlockedByKEM())
    {
        OutReason = FText::FromString(TEXT("Cannot save during execution."));
        return true;
    }

    OutReason = FText::GetEmpty();
    return false;
}

bool USOTS_ProfileSubsystem::RequestSaveCurrentProfile(FText& OutFailureReason)
{
    if (ActiveProfileId.ProfileName.IsNone())
    {
        OutFailureReason = FText::FromString(TEXT("No active profile to save."));
        return false;
    }

    if (IsSaveBlockedForUI(OutFailureReason))
    {
        return false;
    }

    FSOTS_ProfileSnapshot Snapshot;
    BuildSnapshotFromWorld(Snapshot);
    const bool bSaved = SaveProfile(ActiveProfileId, Snapshot);
    if (!bSaved && OutFailureReason.IsEmpty())
    {
        OutFailureReason = FText::FromString(TEXT("Save failed."));
    }
    return bSaved;
}

bool USOTS_ProfileSubsystem::RequestCheckpointSave(const FSOTS_ProfileId& ProfileId, FText& OutFailureReason)
{
    if (IsSaveBlockedForUI(OutFailureReason))
    {
        return false;
    }

    FSOTS_ProfileSnapshot Snapshot;
    BuildSnapshotFromWorld(Snapshot);
    const bool bSaved = SaveProfile(ProfileId, Snapshot);
    if (!bSaved && OutFailureReason.IsEmpty())
    {
        OutFailureReason = FText::FromString(TEXT("Save failed."));
    }
    return bSaved;
}

void USOTS_ProfileSubsystem::SetActiveProfile(const FSOTS_ProfileId& ProfileId)
{
    ActiveProfileId = ProfileId;
}

void USOTS_ProfileSubsystem::StartAutosaveTimer()
{
    if (!bEnableAutosave || AutosaveIntervalSeconds <= 0.0f)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(AutosaveTimerHandle, this, &USOTS_ProfileSubsystem::HandleAutosave, AutosaveIntervalSeconds, true);
    }
}

void USOTS_ProfileSubsystem::StopAutosaveTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutosaveTimerHandle);
    }
}

void USOTS_ProfileSubsystem::HandleAutosave()
{
    if (ActiveProfileId.ProfileName.IsNone())
    {
        return;
    }

    FText Reason;
    RequestSaveCurrentProfile(Reason);
}
