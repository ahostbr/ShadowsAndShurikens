#include "Subsystems/SOTS_AbilitySubsystem.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Math/UnrealMathUtility.h"
#include "SOTS_ProfileSubsystem.h"

void USOTS_AbilitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    GrantedAbilityTags.Reset();
    AbilityRanks.Reset();
    AbilityCooldownRemaining.Reset();

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_ProfileSubsystem* ProfileSubsystem = GI->GetSubsystem<USOTS_ProfileSubsystem>())
        {
            ProfileSubsystem->RegisterProvider(this, 0);
        }
    }
}

void USOTS_AbilitySubsystem::Deinitialize()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (USOTS_ProfileSubsystem* ProfileSubsystem = GI->GetSubsystem<USOTS_ProfileSubsystem>())
        {
            ProfileSubsystem->UnregisterProvider(this);
        }
    }

    GrantedAbilityTags.Reset();
    AbilityRanks.Reset();
    AbilityCooldownRemaining.Reset();
    Super::Deinitialize();
}

USOTS_AbilitySubsystem* USOTS_AbilitySubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (const UWorld* World = WorldContextObject->GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<USOTS_AbilitySubsystem>();
        }
    }

    return nullptr;
}

void USOTS_AbilitySubsystem::BuildProfileData(FSOTS_AbilityProfileData& OutData) const
{
    OutData.GrantedAbilityTags = GrantedAbilityTags;
    OutData.AbilityRanks = AbilityRanks;
    OutData.CooldownsRemaining = AbilityCooldownRemaining;
}

void USOTS_AbilitySubsystem::ApplyProfileData(const FSOTS_AbilityProfileData& InData)
{
    GrantedAbilityTags = InData.GrantedAbilityTags;
    AbilityRanks = InData.AbilityRanks;
    AbilityCooldownRemaining = InData.CooldownsRemaining;
}

void USOTS_AbilitySubsystem::BuildProfileSnapshot(FSOTS_ProfileSnapshot& InOutSnapshot)
{
    BuildProfileData(InOutSnapshot.Ability);
}

void USOTS_AbilitySubsystem::ApplyProfileSnapshot(const FSOTS_ProfileSnapshot& Snapshot)
{
    ApplyProfileData(Snapshot.Ability);
}

void USOTS_AbilitySubsystem::GrantAbility(FGameplayTag AbilityTag, int32 Rank)
{
    if (!AbilityTag.IsValid())
    {
        return;
    }

    if (!GrantedAbilityTags.Contains(AbilityTag))
    {
        GrantedAbilityTags.Add(AbilityTag);
    }

    if (Rank > 0)
    {
        AbilityRanks.Add(AbilityTag, Rank);
    }
}

void USOTS_AbilitySubsystem::RemoveAbility(FGameplayTag AbilityTag)
{
    GrantedAbilityTags.Remove(AbilityTag);
    AbilityRanks.Remove(AbilityTag);
    AbilityCooldownRemaining.Remove(AbilityTag);
}

bool USOTS_AbilitySubsystem::HasAbility(FGameplayTag AbilityTag) const
{
    return GrantedAbilityTags.Contains(AbilityTag);
}

void USOTS_AbilitySubsystem::SetAbilityRank(FGameplayTag AbilityTag, int32 NewRank)
{
    if (!AbilityTag.IsValid())
    {
        return;
    }

    if (NewRank <= 0)
    {
        AbilityRanks.Remove(AbilityTag);
    }
    else
    {
        AbilityRanks.Add(AbilityTag, NewRank);
    }
}

int32 USOTS_AbilitySubsystem::GetAbilityRank(FGameplayTag AbilityTag) const
{
    if (const int32* Found = AbilityRanks.Find(AbilityTag))
    {
        return *Found;
    }

    return 0;
}

void USOTS_AbilitySubsystem::SetAbilityCooldownRemaining(FGameplayTag AbilityTag, float RemainingSeconds)
{
    if (!AbilityTag.IsValid())
    {
        return;
    }

    if (RemainingSeconds <= 0.f)
    {
        AbilityCooldownRemaining.Remove(AbilityTag);
    }
    else
    {
        AbilityCooldownRemaining.Add(AbilityTag, RemainingSeconds);
    }
}

float USOTS_AbilitySubsystem::GetAbilityCooldownRemaining(FGameplayTag AbilityTag) const
{
    if (const float* Found = AbilityCooldownRemaining.Find(AbilityTag))
    {
        return *Found;
    }

    return 0.f;
}

FString USOTS_AbilitySubsystem::GetAbilityProfileSummary() const
{
    return FString::Printf(TEXT("Abilities=%d Ranks=%d Cooldowns=%d"),
        GrantedAbilityTags.Num(),
        AbilityRanks.Num(),
        AbilityCooldownRemaining.Num());
}
