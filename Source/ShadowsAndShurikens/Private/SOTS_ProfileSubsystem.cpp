#include "SOTS_ProfileSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "SOTS_CharacterBase.h"
#include "SOTS_FXManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_MissionDirectorSubsystem.h"
#include "SOTS_MMSSSubsystem.h"
#include "SOTS_ProfileSaveGame.h"
#include "SOTS_StatsComponent.h"

FString USOTS_ProfileSubsystem::GetSlotNameForProfile(const FSOTS_ProfileId& ProfileId) const
{
    return FString::Printf(TEXT("SOTS_Profile_%d_%s"),
        ProfileId.SlotIndex,
        *ProfileId.ProfileName.ToString());
}

bool USOTS_ProfileSubsystem::SaveProfile(const FSOTS_ProfileId& ProfileId, const FSOTS_ProfileSnapshot& InSnapshot)
{
    const FString SlotName = GetSlotNameForProfile(ProfileId);
    constexpr int32 UserIndex = 0;

    if (USOTS_ProfileSaveGame* SaveGame = Cast<USOTS_ProfileSaveGame>(
            UGameplayStatics::CreateSaveGameObject(USOTS_ProfileSaveGame::StaticClass())))
    {
        SaveGame->Snapshot = InSnapshot;
        if (UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex))
        {
            return true;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("USOTS_ProfileSubsystem::SaveProfile failed for slot %s"), *SlotName);
    return false;
}

bool USOTS_ProfileSubsystem::LoadProfile(const FSOTS_ProfileId& ProfileId, FSOTS_ProfileSnapshot& OutSnapshot)
{
    const FString SlotName = GetSlotNameForProfile(ProfileId);
    constexpr int32 UserIndex = 0;

    if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        UE_LOG(LogTemp, Verbose, TEXT("USOTS_ProfileSubsystem::LoadProfile no save %s"), *SlotName);
        return false;
    }

    if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex))
    {
        if (USOTS_ProfileSaveGame* ProfileSave = Cast<USOTS_ProfileSaveGame>(Loaded))
        {
            OutSnapshot = ProfileSave->Snapshot;
            return true;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("USOTS_ProfileSubsystem::LoadProfile failed to load %s"), *SlotName);
    return false;
}

void USOTS_ProfileSubsystem::BuildSnapshotFromWorld(FSOTS_ProfileSnapshot& OutSnapshot)
{
    if (UWorld* World = GetWorld())
    {
        OutSnapshot.Meta.LastPlayedUtc = FDateTime::UtcNow();

        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (ASOTS_CharacterBase* Player = Cast<ASOTS_CharacterBase>(PC->GetPawn()))
            {
                Player->BuildCharacterStateSnapshot(OutSnapshot.PlayerCharacter);
                BuildCharacterStatsStateFromWorld(Player, OutSnapshot.PlayerCharacter);
            }
            else if (APawn* Pawn = PC->GetPawn())
            {
                OutSnapshot.PlayerCharacter.Transform = Pawn->GetActorTransform();
            }
        }
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (USOTS_MissionDirectorSubsystem* MissionSubsystem = GameInstance->GetSubsystem<USOTS_MissionDirectorSubsystem>())
        {
            MissionSubsystem->BuildProfileData(OutSnapshot.Missions);
        }

        if (USOTS_MMSSSubsystem* MusicSubsystem = GameInstance->GetSubsystem<USOTS_MMSSSubsystem>())
        {
            MusicSubsystem->BuildProfileData(OutSnapshot.Music);
        }

        if (USOTS_FXManagerSubsystem* FXSubsystem = GameInstance->GetSubsystem<USOTS_FXManagerSubsystem>())
        {
            FXSubsystem->BuildProfileData(OutSnapshot.FX);
        }
    }
}

void USOTS_ProfileSubsystem::ApplySnapshotToWorld(const FSOTS_ProfileSnapshot& InSnapshot)
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (ASOTS_CharacterBase* Player = Cast<ASOTS_CharacterBase>(PC->GetPawn()))
            {
                Player->ApplyCharacterStateSnapshot(InSnapshot.PlayerCharacter);
                ApplyCharacterStatsStateToWorld(Player, InSnapshot.PlayerCharacter);
            }
            else if (APawn* Pawn = PC->GetPawn())
            {
                Pawn->SetActorTransform(InSnapshot.PlayerCharacter.Transform);
            }
        }
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (USOTS_MissionDirectorSubsystem* MissionSubsystem = GameInstance->GetSubsystem<USOTS_MissionDirectorSubsystem>())
        {
            MissionSubsystem->ApplyProfileData(InSnapshot.Missions);
        }

        if (USOTS_MMSSSubsystem* MusicSubsystem = GameInstance->GetSubsystem<USOTS_MMSSSubsystem>())
        {
            MusicSubsystem->ApplyProfileData(InSnapshot.Music);
        }

        if (USOTS_FXManagerSubsystem* FXSubsystem = GameInstance->GetSubsystem<USOTS_FXManagerSubsystem>())
        {
            FXSubsystem->ApplyProfileData(InSnapshot.FX);
        }
    }
}

void USOTS_ProfileSubsystem::BuildCharacterStatsStateFromWorld(AActor* Character, FSOTS_CharacterStateData& OutState) const
{
    if (!Character)
    {
        OutState.StatValues.Reset();
        return;
    }

    if (USOTS_StatsComponent* StatsComp = Character->FindComponentByClass<USOTS_StatsComponent>())
    {
        StatsComp->BuildCharacterStateData(OutState);
    }
    else
    {
        OutState.StatValues.Reset();
    }
}

void USOTS_ProfileSubsystem::ApplyCharacterStatsStateToWorld(AActor* Character, const FSOTS_CharacterStateData& InState) const
{
    if (!Character)
    {
        return;
    }

    if (USOTS_StatsComponent* StatsComp = Character->FindComponentByClass<USOTS_StatsComponent>())
    {
        StatsComp->ApplyCharacterStateData(InState);
    }
}

void USOTS_ProfileSubsystem::SOTS_DumpCurrentPlayerStatsSnapshot()
{
    FSOTS_CharacterStateData Snapshot;
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                BuildCharacterStatsStateFromWorld(Pawn, Snapshot);
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
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("SOTS_DumpCurrentPlayerStatsSnapshot: player pawn unavailable."));
}
