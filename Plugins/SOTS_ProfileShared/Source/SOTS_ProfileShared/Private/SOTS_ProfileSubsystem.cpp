#include "SOTS_ProfileSubsystem.h"

#include "Algo/Sort.h"
#include "Components/ActorComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_ProfileSaveGame.h"

FString USOTS_ProfileSubsystem::GetSlotNameForProfile(const FSOTS_ProfileId& ProfileId) const
{
    return FString::Printf(TEXT("SOTS_Profile_%d_%s"),
        ProfileId.SlotIndex,
        *ProfileId.ProfileName.ToString());
}

bool USOTS_ProfileSubsystem::SaveProfile(const FSOTS_ProfileId& ProfileId, const FSOTS_ProfileSnapshot& Snapshot)
{
    const FString SlotName = GetSlotNameForProfile(ProfileId);
    constexpr int32 UserIndex = 0;

    FSOTS_ProfileSnapshot SnapshotToSave = Snapshot;
    SnapshotToSave.Meta.Id = ProfileId;
    if (SnapshotToSave.Meta.DisplayName.IsEmpty())
    {
        SnapshotToSave.Meta.DisplayName = ProfileId.ProfileName.ToString();
    }
    SnapshotToSave.Meta.LastPlayedUtc = FDateTime::UtcNow();

    if (USOTS_ProfileSaveGame* SaveGame = Cast<USOTS_ProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(USOTS_ProfileSaveGame::StaticClass())))
    {
        SaveGame->Snapshot = SnapshotToSave;
        if (UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex))
        {
            return true;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[ProfileSubsystem] SaveProfile failed for slot %s"), *SlotName);
    return false;
}

bool USOTS_ProfileSubsystem::LoadProfile(const FSOTS_ProfileId& ProfileId, FSOTS_ProfileSnapshot& OutSnapshot)
{
    const FString SlotName = GetSlotNameForProfile(ProfileId);
    constexpr int32 UserIndex = 0;

    if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[ProfileSubsystem] LoadProfile: no save for slot %s"), *SlotName);
        return false;
    }

    if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex))
    {
        if (USOTS_ProfileSaveGame* ProfileSave = Cast<USOTS_ProfileSaveGame>(Loaded))
        {
            OutSnapshot = ProfileSave->Snapshot;
            OutSnapshot.Meta.Id = ProfileId;
            if (OutSnapshot.Meta.DisplayName.IsEmpty())
            {
                OutSnapshot.Meta.DisplayName = ProfileId.ProfileName.ToString();
            }
            return true;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[ProfileSubsystem] LoadProfile failed for slot %s"), *SlotName);
    return false;
}

void USOTS_ProfileSubsystem::BuildSnapshotFromWorld(FSOTS_ProfileSnapshot& OutSnapshot)
{
    OutSnapshot.Meta.LastPlayedUtc = FDateTime::UtcNow();
    if (OutSnapshot.Meta.DisplayName.IsEmpty() && !OutSnapshot.Meta.Id.ProfileName.IsNone())
    {
        OutSnapshot.Meta.DisplayName = OutSnapshot.Meta.Id.ProfileName.ToString();
    }

    GatherPlayerSnapshot(OutSnapshot);
    InvokeProviderBuild(OutSnapshot);
}

void USOTS_ProfileSubsystem::ApplySnapshotToWorld(const FSOTS_ProfileSnapshot& Snapshot)
{
    RestorePlayerFromSnapshot(Snapshot);
    InvokeProviderApply(Snapshot);
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
            UE_LOG(LogTemp, Log, TEXT("ProfileStats: no stats available"));
        }
        else
        {
            for (const auto& Pair : Snapshot.StatValues)
            {
                UE_LOG(LogTemp, Log, TEXT("ProfileStats: Tag=%s Value=%.2f"), *Pair.Key.ToString(), Pair.Value);
            }
        }
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("SOTS_DumpCurrentPlayerStatsSnapshot: player pawn unavailable."));
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
        Pawn->SetActorTransform(Snapshot.PlayerCharacter.Transform);
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
