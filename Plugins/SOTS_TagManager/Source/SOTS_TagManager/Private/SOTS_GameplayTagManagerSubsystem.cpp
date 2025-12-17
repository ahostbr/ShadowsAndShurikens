#include "SOTS_GameplayTagManagerSubsystem.h"

#include "GameplayTagAssetInterface.h"
#include "GameplayTagsManager.h"
#include "GameFramework/Actor.h"

void USOTS_GameplayTagManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CachedTags.Empty();
    ActorLooseTags.Empty();
    HandleToRecord.Empty();
    ActorScopedTagCounts.Empty();
    ActorEndPlayBindings.Empty();
}

void USOTS_GameplayTagManagerSubsystem::Deinitialize()
{
    CachedTags.Empty();
    ActorLooseTags.Empty();
    HandleToRecord.Empty();
    ActorScopedTagCounts.Empty();
    ActorEndPlayBindings.Empty();
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

void USOTS_GameplayTagManagerSubsystem::AppendActorScopedTags(const AActor* Actor, FGameplayTagContainer& OutTags) const
{
    if (!Actor)
    {
        return;
    }

    const TWeakObjectPtr<const AActor> Key(Actor);
    if (const TMap<FGameplayTag, int32>* Scoped = ActorScopedTagCounts.Find(Key))
    {
        for (const TPair<FGameplayTag, int32>& Pair : *Scoped)
        {
            if (Pair.Key.IsValid() && Pair.Value > 0)
            {
                OutTags.AddTag(Pair.Key);
            }
        }
    }
}

void USOTS_GameplayTagManagerSubsystem::BuildActorTagUnion(const AActor* Actor, FGameplayTagContainer& OutTags) const
{
    OutTags.Reset();

    if (!Actor)
    {
        return;
    }

    if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Actor))
    {
        TagInterface->GetOwnedGameplayTags(OutTags);
    }

    if (const FGameplayTagContainer* ExtraTags = ActorLooseTags.Find(TWeakObjectPtr<const AActor>(Actor)))
    {
        OutTags.AppendTags(*ExtraTags);
    }

    AppendActorScopedTags(Actor, OutTags);
}

bool USOTS_GameplayTagManagerSubsystem::UnionHasTag(const AActor* Actor, const FGameplayTag& Tag) const
{
    if (!Actor || !Tag.IsValid())
    {
        return false;
    }

    FGameplayTagContainer UnionTags;
    BuildActorTagUnion(Actor, UnionTags);
    return UnionTags.HasTag(Tag);
}

bool USOTS_GameplayTagManagerSubsystem::IsTagVisibleOnActor(const AActor* Actor, const FGameplayTag& Tag) const
{
    return UnionHasTag(Actor, Tag);
}

void USOTS_GameplayTagManagerSubsystem::EnsureEndPlayBinding(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    const TWeakObjectPtr<const AActor> Key(Actor);
    if (ActorEndPlayBindings.Contains(Key))
    {
        return;
    }

    const FDelegateHandle Handle = Actor->OnEndPlay.AddUObject(this, &USOTS_GameplayTagManagerSubsystem::HandleActorEndPlay);
    ActorEndPlayBindings.Add(Key, Handle);
}

void USOTS_GameplayTagManagerSubsystem::HandleActorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
    if (!Actor)
    {
        return;
    }

    FGameplayTagContainer UnionBefore;
    BuildActorTagUnion(Actor, UnionBefore);

    const TWeakObjectPtr<const AActor> Key(Actor);

    ActorLooseTags.Remove(Key);
    ActorScopedTagCounts.Remove(Key);

    // Remove any handles referencing this actor so future removes are no-ops.
    TArray<FGuid> HandlesToRemove;
    HandlesToRemove.Reserve(HandleToRecord.Num());
    for (const TPair<FGuid, FScopedTagRecord>& Pair : HandleToRecord)
    {
        if (Pair.Value.Actor.Get() == Actor)
        {
            HandlesToRemove.Add(Pair.Key);
        }
    }
    for (const FGuid& HandleId : HandlesToRemove)
    {
        HandleToRecord.Remove(HandleId);
    }

    if (FDelegateHandle* Binding = ActorEndPlayBindings.Find(Key))
    {
        Actor->OnEndPlay.Remove(*Binding);
        ActorEndPlayBindings.Remove(Key);
    }

    FGameplayTagContainer UnionAfter;
    BuildActorTagUnion(Actor, UnionAfter);

    for (const FGameplayTag& Tag : UnionBefore)
    {
        if (Tag.IsValid() && !UnionAfter.HasTagExact(Tag))
        {
            OnLooseTagRemoved.Broadcast(Actor, Tag);
        }
    }
}

bool USOTS_GameplayTagManagerSubsystem::IsScopedHandleValid(const FSOTS_LooseTagHandle& Handle) const
{
    return Handle.IsValid() && HandleToRecord.Contains(Handle.Id);
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

    FGameplayTagContainer UnionTags;
    BuildActorTagUnion(Actor, UnionTags);
    return UnionTags.HasTag(Tag);
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

    FGameplayTagContainer UnionTags;
    BuildActorTagUnion(Actor, UnionTags);
    return UnionTags.HasAny(Tags);
}

bool USOTS_GameplayTagManagerSubsystem::ActorHasAllTags(const AActor* Actor, const FGameplayTagContainer& Tags) const
{
    if (!Actor || Tags.IsEmpty())
    {
        return false;
    }

    FGameplayTagContainer UnionTags;
    BuildActorTagUnion(Actor, UnionTags);
    return UnionTags.HasAll(Tags);
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

    const bool bWasVisible = UnionHasTag(Actor, Tag);

    FGameplayTagContainer& Container = ActorLooseTags.FindOrAdd(TWeakObjectPtr<const AActor>(Actor));
    Container.AddTag(Tag);

    EnsureEndPlayBinding(Actor);

    const bool bIsVisible = UnionHasTag(Actor, Tag);
    if (!bWasVisible && bIsVisible)
    {
        OnLooseTagAdded.Broadcast(Actor, Tag);
    }
}

void USOTS_GameplayTagManagerSubsystem::RemoveTagFromActor(AActor* Actor, FGameplayTag Tag)
{
    if (!Actor || !Tag.IsValid())
    {
        return;
    }

    const bool bWasVisible = UnionHasTag(Actor, Tag);

    TWeakObjectPtr<const AActor> Key(Actor);
    if (FGameplayTagContainer* Container = ActorLooseTags.Find(Key))
    {
        Container->RemoveTag(Tag);
        if (Container->IsEmpty())
        {
            ActorLooseTags.Remove(Key);
        }
    }

    const bool bIsVisible = UnionHasTag(Actor, Tag);
    if (bWasVisible && !bIsVisible)
    {
        OnLooseTagRemoved.Broadcast(Actor, Tag);
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
                const bool bWasVisible = UnionHasTag(Actor, Tag);
                Container->RemoveTag(Tag);

                const bool bIsVisible = UnionHasTag(Actor, Tag);
                if (bWasVisible && !bIsVisible)
                {
                    OnLooseTagRemoved.Broadcast(Actor, Tag);
                }
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

FSOTS_LooseTagHandle USOTS_GameplayTagManagerSubsystem::AddScopedTagToActor(AActor* Actor, FGameplayTag Tag)
{
    FSOTS_LooseTagHandle Handle;

    if (!Actor || !Tag.IsValid())
    {
        return Handle;
    }

    const bool bWasVisible = UnionHasTag(Actor, Tag);

    Handle.Id = FGuid::NewGuid();

    const TWeakObjectPtr<const AActor> Key(Actor);
    int32& Count = ActorScopedTagCounts.FindOrAdd(Key).FindOrAdd(Tag);
    ++Count;

    FScopedTagRecord Record;
    Record.Actor = Actor;
    Record.Tag = Tag;
    HandleToRecord.Add(Handle.Id, Record);

    EnsureEndPlayBinding(Actor);

    const bool bIsVisible = UnionHasTag(Actor, Tag);
    if (!bWasVisible && bIsVisible)
    {
        OnLooseTagAdded.Broadcast(Actor, Tag);
    }

    return Handle;
}

FSOTS_LooseTagHandle USOTS_GameplayTagManagerSubsystem::AddScopedTagToActorByName(AActor* Actor, FName TagName)
{
    const FGameplayTag Tag = ResolveAndCacheTag(TagName);
    return AddScopedTagToActor(Actor, Tag);
}

bool USOTS_GameplayTagManagerSubsystem::RemoveScopedTagByHandle(FSOTS_LooseTagHandle Handle)
{
    if (!Handle.IsValid())
    {
        return false;
    }

    FScopedTagRecord* Record = HandleToRecord.Find(Handle.Id);
    if (!Record)
    {
        return false;
    }

    TWeakObjectPtr<AActor> ActorPtr = Record->Actor;
    const FGameplayTag Tag = Record->Tag;

    HandleToRecord.Remove(Handle.Id);

    AActor* Actor = ActorPtr.Get();
    if (!Actor || !Tag.IsValid())
    {
        return false;
    }

    const bool bWasVisible = UnionHasTag(Actor, Tag);

    const TWeakObjectPtr<const AActor> Key(Actor);
    if (TMap<FGameplayTag, int32>* ScopedCounts = ActorScopedTagCounts.Find(Key))
    {
        if (int32* Count = ScopedCounts->Find(Tag))
        {
            *Count = FMath::Max(0, *Count - 1);
            if (*Count <= 0)
            {
                ScopedCounts->Remove(Tag);
            }
        }

        if (ScopedCounts->Num() == 0)
        {
            ActorScopedTagCounts.Remove(Key);
        }
    }

    const bool bIsVisible = UnionHasTag(Actor, Tag);
    if (bWasVisible && !bIsVisible)
    {
        OnLooseTagRemoved.Broadcast(Actor, Tag);
    }

    return true;
}

FSOTS_LooseTagHandle USOTS_GameplayTagManagerSubsystem::AddScopedLooseTag(AActor* Actor, FGameplayTag Tag)
{
    return AddScopedTagToActor(Actor, Tag);
}

FSOTS_LooseTagHandle USOTS_GameplayTagManagerSubsystem::AddScopedLooseTagByName(AActor* Actor, FName TagName)
{
    return AddScopedTagToActorByName(Actor, TagName);
}

bool USOTS_GameplayTagManagerSubsystem::RemoveScopedLooseTagByHandle(const FSOTS_LooseTagHandle& Handle)
{
    return RemoveScopedTagByHandle(Handle);
}
