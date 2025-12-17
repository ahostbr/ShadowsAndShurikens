#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_LooseTagHandle.h"
#include "SOTS_GameplayTagManagerSubsystem.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_LooseTagChangedSignature, AActor*, Actor, FGameplayTag, Tag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_OnLooseTagChanged, AActor*, Actor, FGameplayTag, Tag);

USTRUCT()
struct FScopedTagRecord
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> Actor;

    UPROPERTY()
    FGameplayTag Tag;
};

/**
 * Central gameplay tag helper used by SOTS plugins.
 *
 * SOTS TAG SPINE (V2): The single authoritative surface for shared loose actor
 * gameplay tags. Prefer routing writes and queries through this subsystem
 * or the `USOTS_TagLibrary` blueprint helpers.
 * Provides cached tag lookup by name and convenience
 * helpers for querying tags on actors.
 */
UCLASS()
class SOTS_TAGMANAGER_API USOTS_GameplayTagManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // --- UGameInstanceSubsystem overrides ---
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- Core API ---

    /** Lazily resolves a tag by name, e.g. "Player.States.Aiming". Never crashes; returns empty tag on failure. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    FGameplayTag GetTagByName(FName TagName);

    /** Same as GetTagByName but logs an error if not found. Still returns an empty tag on failure. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    FGameplayTag GetTagChecked(FName TagName);

    /** Returns true if the actor has the given tag. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Tags")
    bool ActorHasTag(const AActor* Actor, FGameplayTag Tag) const;

    /** Convenience: returns true if the actor has the tag identified by TagName. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Tags")
    bool ActorHasTagByName(const AActor* Actor, FName TagName);

    /** Returns true if the actor has any tag in the given container. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Tags")
    bool ActorHasAnyTags(const AActor* Actor, const FGameplayTagContainer& Tags) const;

    /** Returns true if the actor has all tags in the given container. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Tags")
    bool ActorHasAllTags(const AActor* Actor, const FGameplayTagContainer& Tags) const;

    /** Adds a loose gameplay tag to the given actor for SOTS-global state. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    void AddTagToActor(AActor* Actor, FGameplayTag Tag);

    /** Removes a loose gameplay tag previously added to the given actor. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    void RemoveTagFromActor(AActor* Actor, FGameplayTag Tag);

    /** Removes a set of loose gameplay tags previously added to the given actor. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    void RemoveTagsFromActor(AActor* Actor, const TArray<FGameplayTag>& Tags);

    /** Convenience helpers that resolve the tag by name before adding/removing. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    void AddTagToActorByName(AActor* Actor, FName TagName);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    void RemoveTagFromActorByName(AActor* Actor, FName TagName);

    /** Adds a scoped loose gameplay tag to the actor. Returns a handle that must be passed back to remove this specific contribution. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    FSOTS_LooseTagHandle AddScopedTagToActor(AActor* Actor, FGameplayTag Tag);

    /** Adds a scoped loose gameplay tag to the actor, resolving the tag by name. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    FSOTS_LooseTagHandle AddScopedTagToActorByName(AActor* Actor, FName TagName);

    /** Removes a scoped loose gameplay tag contribution using the handle returned from AddScopedTagToActor. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    bool RemoveScopedTagByHandle(FSOTS_LooseTagHandle Handle);

    /** SPINE1 alias: scoped loose tag add (Blueprint) using requested naming. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    FSOTS_LooseTagHandle AddScopedLooseTag(AActor* Actor, FGameplayTag Tag);

    /** SPINE1 alias: scoped loose tag add by name. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    FSOTS_LooseTagHandle AddScopedLooseTagByName(AActor* Actor, FName TagName);

    /** SPINE1 alias: scoped loose tag remove by handle. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags")
    bool RemoveScopedLooseTagByHandle(const FSOTS_LooseTagHandle& Handle);

    /** Optional helper: returns true if a handle is currently valid in the manager. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Tags")
    bool IsScopedHandleValid(const FSOTS_LooseTagHandle& Handle) const;

    /** Fired when the union-visible loose tag state transitions from absent to present. */
    UPROPERTY(BlueprintAssignable, Category = "SOTS|Tags|Events")
    FSOTS_OnLooseTagChanged OnLooseTagAdded;

    /** Fired when the union-visible loose tag state transitions from present to absent. */
    UPROPERTY(BlueprintAssignable, Category = "SOTS|Tags|Events")
    FSOTS_OnLooseTagChanged OnLooseTagRemoved;

protected:
    /** Simple cache from FName to tag for faster repeated lookups. */
    UPROPERTY(Transient)
    TMap<FName, FGameplayTag> CachedTags;

    /** Lightweight loose tag storage keyed by actor. */
    UPROPERTY(Transient)
    TMap<TWeakObjectPtr<const AActor>, FGameplayTagContainer> ActorLooseTags;

    /** Scoped tag handle record linking the GUID back to actor and tag. */
    UPROPERTY(Transient)
    TMap<FGuid, FScopedTagRecord> HandleToRecord;

    /** Scoped tag counts per actor, allowing multiple handles per tag without losing legacy semantics. */
    TMap<TWeakObjectPtr<const AActor>, TMap<FGameplayTag, int32>> ActorScopedTagCounts;

    /** Optional end-play bindings for cleanup per actor. */
    TSet<TWeakObjectPtr<const AActor>> ActorEndPlayBindings;

    /** Internal helper to resolve and cache a tag. */
    FGameplayTag ResolveAndCacheTag(FName TagName);

    /** Append scoped tags for the actor into the union container. */
    void AppendActorScopedTags(const AActor* Actor, FGameplayTagContainer& OutTags) const;

    /** Build the union of owned (interface), unscoped loose, and scoped-count tags for the actor. */
    void BuildActorTagUnion(const AActor* Actor, FGameplayTagContainer& OutTags) const;

    /** Returns true if the union-visible tags include the provided tag. */
    bool IsTagVisibleOnActor(const AActor* Actor, const FGameplayTag& Tag) const;

    /** Returns true if the union-visible tags include the provided tag (utility wrapper). */
    bool UnionHasTag(const AActor* Actor, const FGameplayTag& Tag) const;

    /** Ensure we are listening for EndPlay on this actor to perform cleanup. */
    void EnsureEndPlayBinding(AActor* Actor);

    /** Clean up all loose and scoped tags when an actor ends play or is destroyed. */
    UFUNCTION()
    void HandleActorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

};
