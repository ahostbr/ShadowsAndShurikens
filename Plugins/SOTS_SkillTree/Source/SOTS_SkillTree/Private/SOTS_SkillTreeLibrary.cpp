#include "SOTS_SkillTreeLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SOTS_SkillTreeSubsystem.h"

USOTS_SkillTreeSubsystem* USOTS_SkillTreeLibrary::GetSkillTreeSubsystem(const UObject* WorldContextObject)
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

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        return nullptr;
    }

    return GI->GetSubsystem<USOTS_SkillTreeSubsystem>();
}

void USOTS_SkillTreeLibrary::GetAllSkillTags(
    const UObject* WorldContextObject,
    FGameplayTagContainer& OutSkillTags)
{
    OutSkillTags.Reset();

    if (USOTS_SkillTreeSubsystem* Subsys = GetSkillTreeSubsystem(WorldContextObject))
    {
        const TArray<FName> TreeIds = Subsys->GetRegisteredTreeIds();
        for (const FName& TreeId : TreeIds)
        {
            const FGameplayTagContainer TreeTags = Subsys->GetGrantedTagsForTree(TreeId);
            OutSkillTags.AppendTags(TreeTags);
        }
    }
}

bool USOTS_SkillTreeLibrary::IsSkillUnlocked(
    const UObject* WorldContextObject,
    const FGameplayTag& SkillTag)
{
    if (!SkillTag.IsValid())
    {
        return false;
    }

    FGameplayTagContainer AllTags;
    GetAllSkillTags(WorldContextObject, AllTags);

    return AllTags.HasTag(SkillTag);
}

bool USOTS_SkillTreeLibrary::SOTS_PlayerHasSkillTag(
    const UObject* WorldContextObject,
    FGameplayTag SkillTag)
{
    if (!WorldContextObject || !SkillTag.IsValid())
    {
        return false;
    }

    if (USOTS_SkillTreeSubsystem* Subsys = GetSkillTreeSubsystem(WorldContextObject))
    {
        return Subsys->HasSkillTag(SkillTag);
    }

    return false;
}

bool USOTS_SkillTreeLibrary::SkillTree_IsNodeUnlocked(
    const UObject* WorldContextObject,
    FGameplayTag NodeTag)
{
    if (!NodeTag.IsValid())
    {
        return false;
    }

    if (USOTS_SkillTreeSubsystem* Subsys = GetSkillTreeSubsystem(WorldContextObject))
    {
        return Subsys->IsNodeUnlockedByTag(NodeTag);
    }

    return false;
}

bool USOTS_SkillTreeLibrary::SkillTree_AreAllNodesUnlocked(
    const UObject* WorldContextObject,
    const TArray<FGameplayTag>& NodeTags)
{
    if (NodeTags.Num() == 0)
    {
        return true;
    }

    if (USOTS_SkillTreeSubsystem* Subsys = GetSkillTreeSubsystem(WorldContextObject))
    {
        return Subsys->AreAllNodesUnlocked(NodeTags);
    }

    return false;
}

bool USOTS_SkillTreeLibrary::SkillTree_IsAnyNodeUnlocked(
    const UObject* WorldContextObject,
    const TArray<FGameplayTag>& NodeTags)
{
    if (NodeTags.Num() == 0)
    {
        return false;
    }

    if (USOTS_SkillTreeSubsystem* Subsys = GetSkillTreeSubsystem(WorldContextObject))
    {
        return Subsys->IsAnyNodeUnlocked(NodeTags);
    }

    return false;
}
