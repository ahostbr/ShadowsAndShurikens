#include "SOTS_TagLibrary.h"
#include "SOTS_TagAccessHelpers.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SOTS_GameplayTagManagerSubsystem.h"
#include "GameFramework/Actor.h"

USOTS_GameplayTagManagerSubsystem* USOTS_TagLibrary::GetManager(const UObject* WorldContextObject)
{
    // Use the header-only helper that will perform the minimal safe lookups
    // into the World/GameInstance and obtain the subsystem instance.
    return SOTS_GetTagSubsystem(WorldContextObject);
}

FGameplayTag USOTS_TagLibrary::GetTagByName(const UObject* WorldContextObject, FName TagName)
{
    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        return Manager->GetTagByName(TagName);
    }

    return FGameplayTag();
}

void USOTS_TagLibrary::AddTagToActor(const UObject* WorldContextObject, AActor* Target, FGameplayTag Tag)
{
    if (!Target || !Tag.IsValid())
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        Manager->AddTagToActor(Target, Tag);
    }
}

void USOTS_TagLibrary::RemoveTagFromActor(const UObject* WorldContextObject, AActor* Target, FGameplayTag Tag)
{
    if (!Target || !Tag.IsValid())
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        Manager->RemoveTagFromActor(Target, Tag);
    }
}

// SOTS TAG SPINE (V2): Route global tag writes through the canonical Tag Manager subsystem.
// This avoids ad-hoc mutation of tag containers or actor-local globals elsewhere in the codebase.
void USOTS_TagLibrary::AddTagToActorByName(const UObject* WorldContextObject, AActor* Target, FName TagName)
{
    if (!Target || TagName.IsNone())
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        Manager->AddTagToActorByName(Target, TagName);
    }
}

// SOTS TAG SPINE (V2): Route global tag writes through the canonical Tag Manager subsystem.
// This avoids ad-hoc mutation of tag containers or actor-local globals elsewhere in the codebase.
void USOTS_TagLibrary::RemoveTagFromActorByName(const UObject* WorldContextObject, AActor* Target, FName TagName)
{
    if (!Target || TagName.IsNone())
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        Manager->RemoveTagFromActorByName(Target, TagName);
    }
}

// SOTS TAG SPINE (V2): Route global tag writes through the canonical Tag Manager subsystem.
// This avoids ad-hoc mutation of tag containers or actor-local globals elsewhere in the codebase.
void USOTS_TagLibrary::RemoveTagsFromActor(const UObject* WorldContextObject, AActor* Target, const TArray<FGameplayTag>& Tags)
{
    if (!Target || Tags.Num() == 0)
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        Manager->RemoveTagsFromActor(Target, Tags);
    }
}

bool USOTS_TagLibrary::ActorHasTag(const UObject* WorldContextObject, const AActor* Target, FGameplayTag Tag)
{
    if (!Target || !Tag.IsValid())
    {
        return false;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        return Manager->ActorHasTag(Target, Tag);
    }

    return false;
}

bool USOTS_TagLibrary::ActorHasAnyTag(const UObject* WorldContextObject, const AActor* Target, const FGameplayTagContainer& Tags)
{
    if (!Target)
    {
        return false;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        return Manager->ActorHasAnyTags(Target, Tags);
    }

    return false;
}

bool USOTS_TagLibrary::ActorHasAllTags(const UObject* WorldContextObject, const AActor* Target, const FGameplayTagContainer& Tags)
{
    if (!Target)
    {
        return false;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        return Manager->ActorHasAllTags(Target, Tags);
    }

    return false;
}

FGameplayTagContainer USOTS_TagLibrary::GetActorTags(const UObject* WorldContextObject, const AActor* Target)
{
    FGameplayTagContainer Tags;

    if (Target)
    {
        if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
        {
            Manager->GetActorTags(Target, Tags);
        }
    }

    return Tags;
}

void USOTS_TagLibrary::GetActorTags(const UObject* WorldContextObject, AActor* Target, FGameplayTagContainer& OutTags)
{
    OutTags.Reset();

    if (!Target)
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        Manager->GetActorTags(Target, OutTags);
    }
}

FSOTS_LooseTagHandle USOTS_TagLibrary::AddScopedTagToActor(const UObject* WorldContextObject, AActor* Target, FGameplayTag Tag)
{
    if (!Target || !Tag.IsValid())
    {
        return FSOTS_LooseTagHandle();
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        return Manager->AddScopedTagToActor(Target, Tag);
    }

    return FSOTS_LooseTagHandle();
}

FSOTS_LooseTagHandle USOTS_TagLibrary::AddScopedTagToActorByName(const UObject* WorldContextObject, AActor* Target, FName TagName)
{
    if (!Target || TagName.IsNone())
    {
        return FSOTS_LooseTagHandle();
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        return Manager->AddScopedTagToActorByName(Target, TagName);
    }

    return FSOTS_LooseTagHandle();
}

bool USOTS_TagLibrary::RemoveScopedTagByHandle(const UObject* WorldContextObject, FSOTS_LooseTagHandle Handle)
{
    if (!Handle.IsValid())
    {
        return false;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        return Manager->RemoveScopedTagByHandle(Handle);
    }

    return false;
}

FSOTS_LooseTagHandle USOTS_TagLibrary::AddScopedLooseTag(const UObject* WorldContextObject, AActor* Target, FGameplayTag Tag)
{
    return AddScopedTagToActor(WorldContextObject, Target, Tag);
}

FSOTS_LooseTagHandle USOTS_TagLibrary::AddScopedLooseTagByName(const UObject* WorldContextObject, AActor* Target, FName TagName)
{
    return AddScopedTagToActorByName(WorldContextObject, Target, TagName);
}

bool USOTS_TagLibrary::RemoveScopedLooseTagByHandle(const UObject* WorldContextObject, const FSOTS_LooseTagHandle& Handle)
{
    return RemoveScopedTagByHandle(WorldContextObject, Handle);
}

bool USOTS_TagLibrary::IsScopedHandleValid(const UObject* WorldContextObject, const FSOTS_LooseTagHandle& Handle)
{
    if (!Handle.IsValid())
    {
        return false;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        return Manager->IsScopedHandleValid(Handle);
    }

    return false;
}

bool USOTS_TagLibrary::ActorHasTagByName(const UObject* WorldContextObject, const AActor* Actor, FName TagName)
{
    if (!Actor)
    {
        return false;
    }

    if (USOTS_GameplayTagManagerSubsystem* Manager = GetManager(WorldContextObject))
    {
        return Manager->ActorHasTagByName(Actor, TagName);
    }

    return false;
}
