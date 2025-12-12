#include "SOTS_StatsLibrary.h"

#include "GameFramework/Actor.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_StatsComponent.h"

float USOTS_StatsLibrary::GetActorStatValue(const UObject* /*WorldContextObject*/, AActor* Target, FGameplayTag StatTag, float DefaultValue)
{
    if (!Target || !StatTag.IsValid())
    {
        return DefaultValue;
    }

    if (USOTS_StatsComponent* StatsComp = Target->FindComponentByClass<USOTS_StatsComponent>())
    {
        return StatsComp->GetStatValue(StatTag);
    }

    return DefaultValue;
}

void USOTS_StatsLibrary::AddToActorStat(const UObject* /*WorldContextObject*/, AActor* Target, FGameplayTag StatTag, float Delta)
{
    if (!Target || !StatTag.IsValid())
    {
        return;
    }

    if (USOTS_StatsComponent* StatsComp = Target->FindComponentByClass<USOTS_StatsComponent>())
    {
        StatsComp->AddToStat(StatTag, Delta);
    }
}

void USOTS_StatsLibrary::SetActorStatValue(const UObject* /*WorldContextObject*/, AActor* Target, FGameplayTag StatTag, float NewValue)
{
    if (!Target || !StatTag.IsValid())
    {
        return;
    }

    if (USOTS_StatsComponent* StatsComp = Target->FindComponentByClass<USOTS_StatsComponent>())
    {
        StatsComp->SetStatValue(StatTag, NewValue);
    }
}

namespace
{
    USOTS_StatsComponent* FindStatsComponent(AActor* Target)
    {
        return Target ? Target->FindComponentByClass<USOTS_StatsComponent>() : nullptr;
    }
}

void USOTS_StatsLibrary::SnapshotActorStats(AActor* Target, FSOTS_CharacterStateData& OutState)
{
    OutState.StatValues.Reset();
    if (USOTS_StatsComponent* StatsComp = FindStatsComponent(Target))
    {
        StatsComp->BuildCharacterStateData(OutState);
    }
}

void USOTS_StatsLibrary::ApplySnapshotToActor(AActor* Target, const FSOTS_CharacterStateData& Snapshot)
{
    if (USOTS_StatsComponent* StatsComp = FindStatsComponent(Target))
    {
        StatsComp->ApplyCharacterStateData(Snapshot);
    }
}

void USOTS_StatsLibrary::SOTS_BuildCharacterStateFromStats(const UObject* /*WorldContextObject*/, AActor* Target, FSOTS_CharacterStateData& OutState)
{
    SnapshotActorStats(Target, OutState);
}

void USOTS_StatsLibrary::SOTS_ApplyCharacterStateToStats(const UObject* /*WorldContextObject*/, AActor* Target, const FSOTS_CharacterStateData& InState)
{
    ApplySnapshotToActor(Target, InState);
}

FString USOTS_StatsLibrary::SOTS_DumpStatsToString(const UObject* /*WorldContextObject*/, AActor* Target)
{
    if (USOTS_StatsComponent* StatsComp = FindStatsComponent(Target))
    {
        const TMap<FGameplayTag, float>& Stats = StatsComp->GetAllStats();
        if (Stats.Num() == 0)
        {
            return TEXT("Stats: empty");
        }

        FString Result;
        for (const auto& Pair : Stats)
        {
            if (!Result.IsEmpty())
            {
                Result += TEXT(", ");
            }
            Result += FString::Printf(TEXT("%s=%.2f"), *Pair.Key.ToString(), Pair.Value);
        }

        return FString::Printf(TEXT("Stats: %s"), *Result);
    }

    return TEXT("Stats: component missing");
}
