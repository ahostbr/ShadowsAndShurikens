#include "SOTS_StatsComponent.h"

#include "Math/UnrealMathUtility.h"
#include "SOTS_ProfileTypes.h"

USOTS_StatsComponent::USOTS_StatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USOTS_StatsComponent::BeginPlay()
{
    Super::BeginPlay();
}

bool USOTS_StatsComponent::HasStat(FGameplayTag StatTag) const
{
    return StatTag.IsValid() && StatValues.Contains(StatTag);
}

float USOTS_StatsComponent::GetStatValue(FGameplayTag StatTag) const
{
    if (!StatTag.IsValid())
    {
        return 0.f;
    }

    if (const float* Found = StatValues.Find(StatTag))
    {
        return *Found;
    }

    return 0.f;
}

float USOTS_StatsComponent::ClampToBounds(FGameplayTag StatTag, float InValue) const
{
    float Result = InValue;

    if (const float* Min = StatMinValues.Find(StatTag))
    {
        Result = FMath::Max(Result, *Min);
    }

    if (const float* Max = StatMaxValues.Find(StatTag))
    {
        Result = FMath::Min(Result, *Max);
    }

    return Result;
}

void USOTS_StatsComponent::InternalSetStat(FGameplayTag StatTag, float NewValue, bool bBroadcast)
{
    if (!StatTag.IsValid())
    {
        return;
    }

    const float Clamped = ClampToBounds(StatTag, NewValue);
    const float OldValue = GetStatValue(StatTag);
    if (FMath::IsNearlyEqual(OldValue, Clamped))
    {
        return;
    }

    StatValues.Add(StatTag, Clamped);

    if (bBroadcast)
    {
        OnStatChanged.Broadcast(StatTag, OldValue, Clamped);
    }
}

void USOTS_StatsComponent::SetStatValue(FGameplayTag StatTag, float NewValue)
{
    InternalSetStat(StatTag, NewValue, true);
}

void USOTS_StatsComponent::AddToStat(FGameplayTag StatTag, float Delta)
{
    const float Current = GetStatValue(StatTag);
    InternalSetStat(StatTag, Current + Delta, true);
}

void USOTS_StatsComponent::SetAllStats(const TMap<FGameplayTag, float>& NewStats)
{
    StatValues.Reset();
    for (const TPair<FGameplayTag, float>& Pair : NewStats)
    {
        InternalSetStat(Pair.Key, Pair.Value, true);
    }
}

void USOTS_StatsComponent::SetStatBounds(FGameplayTag StatTag, float MinValue, float MaxValue)
{
    if (!StatTag.IsValid())
    {
        return;
    }

    StatMinValues.Add(StatTag, MinValue);
    StatMaxValues.Add(StatTag, MaxValue);

    if (StatValues.Contains(StatTag))
    {
        InternalSetStat(StatTag, StatValues[StatTag], true);
    }
}

void USOTS_StatsComponent::BuildCharacterStateData(FSOTS_CharacterStateData& OutState) const
{
    OutState.StatValues = StatValues;
}

void USOTS_StatsComponent::ApplyCharacterStateData(const FSOTS_CharacterStateData& InState)
{
    StatValues = InState.StatValues;

    for (auto& Entry : StatValues)
    {
        Entry.Value = ClampToBounds(Entry.Key, Entry.Value);
    }
}
