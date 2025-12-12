#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_GameplayTagManagerSubsystem.generated.h"

class AActor;

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

protected:
    /** Simple cache from FName to tag for faster repeated lookups. */
    UPROPERTY(Transient)
    TMap<FName, FGameplayTag> CachedTags;

    /** Lightweight loose tag storage keyed by actor. */
    UPROPERTY(Transient)
    TMap<TWeakObjectPtr<const AActor>, FGameplayTagContainer> ActorLooseTags;

    /** Internal helper to resolve and cache a tag. */
    FGameplayTag ResolveAndCacheTag(FName TagName);
};
