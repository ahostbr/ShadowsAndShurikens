#include "SOTS_GameplayTagManagerSubsystem.h"

#include "GameplayTagAssetInterface.h"
#include "GameplayTagsManager.h"
#include "GameFramework/Actor.h"

void USOTS_GameplayTagManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CachedTags.Empty();
    ActorLooseTags.Empty();
}

void USOTS_GameplayTagManagerSubsystem::Deinitialize()
{
    CachedTags.Empty();
    ActorLooseTags.Empty();
    Super::Deinitialize();
}

FGameplayTag USOTS_GameplayTagManagerSubsystem::ResolveAndCacheTag(FName TagName)
{
    if (TagName.IsNone())
    {
        return FGameplayTag();
    }

    if (const FGameplayTag* Found = CachedTags.Find(TagName))
    {
        return *Found;
    }

    FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(TagName, /*ErrorIfNotFound*/ false);
    if (Tag.IsValid())
    {
        CachedTags.Add(TagName, Tag);
    }
    // Intentionally do not cache invalid tags; allows hotfixes if tags are added later.

    return Tag;
}

FGameplayTag USOTS_GameplayTagManagerSubsystem::GetTagByName(FName TagName)
{
    return ResolveAndCacheTag(TagName);
}

FGameplayTag USOTS_GameplayTagManagerSubsystem::GetTagChecked(FName TagName)
{
    const FGameplayTag Tag = ResolveAndCacheTag(TagName);
    if (!Tag.IsValid())
    {
        UE_LOG(LogTemp, Error,
            TEXT("USOTS_GameplayTagManagerSubsystem::GetTagChecked - Tag '%s' is not registered."),
            *TagName.ToString());
    }
    return Tag;
}

bool USOTS_GameplayTagManagerSubsystem::ActorHasTag(const AActor* Actor, FGameplayTag Tag) const
{
    if (!Actor || !Tag.IsValid())
    {
        return false;
    }

    FGameplayTagContainer OwnedTags;

    if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Actor))
    {
        TagInterface->GetOwnedGameplayTags(OwnedTags);
    }

    if (const FGameplayTagContainer* ExtraTags = ActorLooseTags.Find(TWeakObjectPtr<const AActor>(Actor)))
    {
        OwnedTags.AppendTags(*ExtraTags);
    }

    return OwnedTags.HasTag(Tag);
}

bool USOTS_GameplayTagManagerSubsystem::ActorHasTagByName(const AActor* Actor, FName TagName)
{
    USOTS_GameplayTagManagerSubsystem* MutableThis = this;
    const FGameplayTag Tag = MutableThis ? MutableThis->ResolveAndCacheTag(TagName) : FGameplayTag();
    return ActorHasTag(Actor, Tag);
}

bool USOTS_GameplayTagManagerSubsystem::ActorHasAnyTags(const AActor* Actor, const FGameplayTagContainer& Tags) const
{
    if (!Actor || Tags.IsEmpty())
    {
        return false;
    }

    FGameplayTagContainer OwnedTags;

    if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Actor))
    {
        TagInterface->GetOwnedGameplayTags(OwnedTags);
    }

    if (const FGameplayTagContainer* ExtraTags = ActorLooseTags.Find(TWeakObjectPtr<const AActor>(Actor)))
    {
        OwnedTags.AppendTags(*ExtraTags);
    }

    return OwnedTags.HasAny(Tags);
}

bool USOTS_GameplayTagManagerSubsystem::ActorHasAllTags(const AActor* Actor, const FGameplayTagContainer& Tags) const
{
    if (!Actor || Tags.IsEmpty())
    {
        return false;
    }

    FGameplayTagContainer OwnedTags;

    if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Actor))
    {
        TagInterface->GetOwnedGameplayTags(OwnedTags);
    }

    if (const FGameplayTagContainer* ExtraTags = ActorLooseTags.Find(TWeakObjectPtr<const AActor>(Actor)))
    {
        OwnedTags.AppendTags(*ExtraTags);
    }

    return OwnedTags.HasAll(Tags);
}

// SOTS TAG SPINE (V2): This is the canonical write surface for adding loose gameplay tags to actors.
// Use this subsystem or the USOTS_TagLibrary helper functions instead of directly mutating
// actor-owned tag containers to ensure shared tag consistency across plugins.
void USOTS_GameplayTagManagerSubsystem::AddTagToActor(AActor* Actor, FGameplayTag Tag)
{
    if (!Actor || !Tag.IsValid())
    {
        return;
    }

    FGameplayTagContainer& Container = ActorLooseTags.FindOrAdd(TWeakObjectPtr<const AActor>(Actor));
    Container.AddTag(Tag);
}

void USOTS_GameplayTagManagerSubsystem::RemoveTagFromActor(AActor* Actor, FGameplayTag Tag)
{
    if (!Actor || !Tag.IsValid())
    {
        return;
    }

    TWeakObjectPtr<const AActor> Key(Actor);
    if (FGameplayTagContainer* Container = ActorLooseTags.Find(Key))
    {
        Container->RemoveTag(Tag);
        if (Container->IsEmpty())
        {
            ActorLooseTags.Remove(Key);
        }
    }
}

void USOTS_GameplayTagManagerSubsystem::RemoveTagsFromActor(AActor* Actor, const TArray<FGameplayTag>& Tags)
{
    if (!Actor || Tags.IsEmpty())
    {
        return;
    }

    TWeakObjectPtr<const AActor> Key(Actor);
    if (FGameplayTagContainer* Container = ActorLooseTags.Find(Key))
    {
        for (const FGameplayTag& Tag : Tags)
        {
            if (Tag.IsValid())
            {
                Container->RemoveTag(Tag);
            }
        }

        if (Container->IsEmpty())
        {
            ActorLooseTags.Remove(Key);
        }
    }
}

void USOTS_GameplayTagManagerSubsystem::AddTagToActorByName(AActor* Actor, FName TagName)
{
    const FGameplayTag Tag = ResolveAndCacheTag(TagName);
    if (Tag.IsValid())
    {
        AddTagToActor(Actor, Tag);
    }
}

void USOTS_GameplayTagManagerSubsystem::RemoveTagFromActorByName(AActor* Actor, FName TagName)
{
    const FGameplayTag Tag = ResolveAndCacheTag(TagName);
    if (Tag.IsValid())
    {
        RemoveTagFromActor(Actor, Tag);
    }
}
