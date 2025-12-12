#include "SOTS_GAS_SkillTreeLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SOTS_SkillTreeSubsystem.h"

USOTS_SkillTreeSubsystem* USOTS_GAS_SkillTreeLibrary::GetSkillTreeSubsystem(const UObject* WorldContextObject)
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

bool USOTS_GAS_SkillTreeLibrary::HasSkillTagOnTree(
    const UObject* WorldContextObject,
    FName TreeId,
    FGameplayTag SkillTag)
{
    if (!SkillTag.IsValid() || TreeId.IsNone())
    {
        return false;
    }

    if (USOTS_SkillTreeSubsystem* Subsys = GetSkillTreeSubsystem(WorldContextObject))
    {
        const FGameplayTagContainer Tags = Subsys->GetGrantedTagsForTree(TreeId);
        return Tags.HasTag(SkillTag);
    }

    return false;
}

FGameplayTagContainer USOTS_GAS_SkillTreeLibrary::GetAllSkillTagsForTree(
    const UObject* WorldContextObject,
    FName TreeId)
{
    FGameplayTagContainer Result;

    if (TreeId.IsNone())
    {
        return Result;
    }

    if (USOTS_SkillTreeSubsystem* Subsys = GetSkillTreeSubsystem(WorldContextObject))
    {
        Result = Subsys->GetGrantedTagsForTree(TreeId);
    }

    return Result;
}

FGameplayTagContainer USOTS_GAS_SkillTreeLibrary::GetAllSkillTags(
    const UObject* WorldContextObject)
{
    FGameplayTagContainer Result;

    if (USOTS_SkillTreeSubsystem* Subsys = GetSkillTreeSubsystem(WorldContextObject))
    {
        const TArray<FName> TreeIds = Subsys->GetRegisteredTreeIds();
        for (const FName& TreeId : TreeIds)
        {
            const FGameplayTagContainer TreeTags = Subsys->GetGrantedTagsForTree(TreeId);
            Result.AppendTags(TreeTags);
        }
    }

    return Result;
}

