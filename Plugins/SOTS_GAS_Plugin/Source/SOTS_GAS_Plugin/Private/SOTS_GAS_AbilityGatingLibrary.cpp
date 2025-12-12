#include "SOTS_GAS_AbilityGatingLibrary.h"

#include "SOTS_GAS_SkillTreeLibrary.h"
#include "Subsystems/SOTS_GAS_StealthBridgeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SOTS_TagAccessHelpers.h"

namespace
{
    static USOTS_GAS_StealthBridgeSubsystem* GetStealthBridge(const UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return nullptr;
        }

        const UWorld* World = WorldContextObject->GetWorld();
        if (!World)
        {
            return nullptr;
        }

        if (UGameInstance* GI = World->GetGameInstance())
        {
            return GI->GetSubsystem<USOTS_GAS_StealthBridgeSubsystem>();
        }

        return nullptr;
    }
}

bool USOTS_GAS_AbilityGatingLibrary::PlayerHasAllSkillTags(
    const UObject* WorldContextObject,
    const FGameplayTagContainer& Tags)
{
    if (!WorldContextObject)
    {
        // If there are no tags to check, treat as trivially true.
        return Tags.IsEmpty();
    }

    if (Tags.IsEmpty())
    {
        return true;
    }

    const FGameplayTagContainer AllSkillTags =
        USOTS_GAS_SkillTreeLibrary::GetAllSkillTags(WorldContextObject);

    return AllSkillTags.HasAll(Tags);
}

bool USOTS_GAS_AbilityGatingLibrary::PlayerHasAnySkillTag(
    const UObject* WorldContextObject,
    const FGameplayTagContainer& Tags)
{
    if (!WorldContextObject || Tags.IsEmpty())
    {
        return false;
    }

    const FGameplayTagContainer AllSkillTags =
        USOTS_GAS_SkillTreeLibrary::GetAllSkillTags(WorldContextObject);

    return AllSkillTags.HasAny(Tags);
}

bool USOTS_GAS_AbilityGatingLibrary::PlayerHasGlobalTag(
    const UObject* WorldContextObject,
    FGameplayTag Tag)
{
    if (!Tag.IsValid())
    {
        return false;
    }

    if (!WorldContextObject)
    {
        return false;
    }

    const UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return false;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    const AActor* PlayerActor = PC ? PC->GetPawn() : nullptr;
    if (!PlayerActor)
    {
        return false;
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(WorldContextObject))
    {
        return TagSubsystem->ActorHasTag(PlayerActor, Tag);
    }

    return false;
}

bool USOTS_GAS_AbilityGatingLibrary::IsStealthTierAtOrBelow(
    const UObject* WorldContextObject,
    int32 MaxTierInclusive)
{
    if (USOTS_GAS_StealthBridgeSubsystem* Bridge = GetStealthBridge(WorldContextObject))
    {
        const FSOTS_StealthScoreBreakdown Breakdown = Bridge->GetCurrentStealthBreakdown();
        return Breakdown.StealthTier <= MaxTierInclusive;
    }

    return false;
}

bool USOTS_GAS_AbilityGatingLibrary::IsStealthTierWithinRange(
    const UObject* WorldContextObject,
    int32 MinTierInclusive,
    int32 MaxTierInclusive)
{
    if (MinTierInclusive > MaxTierInclusive)
    {
        return false;
    }

    if (USOTS_GAS_StealthBridgeSubsystem* Bridge = GetStealthBridge(WorldContextObject))
    {
        const FSOTS_StealthScoreBreakdown Breakdown = Bridge->GetCurrentStealthBreakdown();
        return Breakdown.StealthTier >= MinTierInclusive &&
               Breakdown.StealthTier <= MaxTierInclusive;
    }

    return false;
}
