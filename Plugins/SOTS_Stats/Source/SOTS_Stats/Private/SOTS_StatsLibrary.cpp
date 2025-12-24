#include "SOTS_StatsLibrary.h"

#include "GameFramework/Actor.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_StatsComponent.h"

static USOTS_StatsComponent* FindStatsComponent(AActor* Target);
static USOTS_StatsComponent* ResolveOrAddStatsComponent(AActor* TargetActor);

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

    if (USOTS_StatsComponent* StatsComp = FindStatsComponent(Target))
    {
        StatsComp->AddToStat(StatTag, Delta);
        return;
    }

    if (USOTS_StatsComponent* NewComp = ResolveOrAddStatsComponent(Target))
    {
        NewComp->AddToStat(StatTag, Delta);
    }
}

void USOTS_StatsLibrary::SetActorStatValue(const UObject* /*WorldContextObject*/, AActor* Target, FGameplayTag StatTag, float NewValue)
{
    if (!Target || !StatTag.IsValid())
    {
        return;
    }

    if (USOTS_StatsComponent* StatsComp = FindStatsComponent(Target))
    {
        StatsComp->SetStatValue(StatTag, NewValue);
        return;
    }

    if (USOTS_StatsComponent* NewComp = ResolveOrAddStatsComponent(Target))
    {
        NewComp->SetStatValue(StatTag, NewValue);
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
        return;
    }

    if (USOTS_StatsComponent* NewComp = ResolveOrAddStatsComponent(Target))
    {
        NewComp->ApplyCharacterStateData(Snapshot);
    }
}

USOTS_StatsComponent* USOTS_StatsLibrary::ResolveStatsComponentForActor(AActor* Target)
{
    return ResolveOrAddStatsComponent(Target);
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

static USOTS_StatsComponent* FindStatsComponent(AActor* Target)
{
    return Target ? Target->FindComponentByClass<USOTS_StatsComponent>() : nullptr;
}

static USOTS_StatsComponent* ResolveOrAddStatsComponent(AActor* TargetActor)
{
    if (!TargetActor)
    {
        return nullptr;
    }

    if (USOTS_StatsComponent* Existing = TargetActor->FindComponentByClass<USOTS_StatsComponent>())
    {
        return Existing;
    }

    return NewObject<USOTS_StatsComponent>(TargetActor, USOTS_StatsComponent::StaticClass(), NAME_None, RF_Transactional);
}
