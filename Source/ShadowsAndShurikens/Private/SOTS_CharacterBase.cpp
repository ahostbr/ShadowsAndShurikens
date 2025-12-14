#include "SOTS_CharacterBase.h"

#include "SOTS_ProfileTypes.h"
ASOTS_CharacterBase::ASOTS_CharacterBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    StatsComponent = CreateDefaultSubobject<USOTS_StatsComponent>(TEXT("StatsComponent"));
    ParkourComponent = CreateDefaultSubobject<USOTS_ParkourComponent>(TEXT("ParkourComponent"));
}

void ASOTS_CharacterBase::BuildCharacterStateSnapshot(FSOTS_CharacterStateData& OutData) const
{
    OutData.Transform = GetActorTransform();

    WriteStatsToCharacterState(OutData);

    OutData.MovementStateTags.Reset();
    OutData.EquippedAbilityTags.Reset();
}

void ASOTS_CharacterBase::ApplyCharacterStateSnapshot(const FSOTS_CharacterStateData& InData)
{
    SetActorTransform(InData.Transform);

    ReadStatsFromCharacterState(InData);
}

void ASOTS_CharacterBase::WriteStatsToCharacterState(FSOTS_CharacterStateData& InOutState) const
{
    if (StatsComponent)
    {
        StatsComponent->WriteToCharacterState(InOutState);
    }
}

void ASOTS_CharacterBase::ReadStatsFromCharacterState(const FSOTS_CharacterStateData& InState)
{
    if (StatsComponent)
    {
        StatsComponent->ReadFromCharacterState(InState);
    }
}

