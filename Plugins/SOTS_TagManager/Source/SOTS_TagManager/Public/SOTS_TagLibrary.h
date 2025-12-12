#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_TagLibrary.generated.h"

class AActor;
class USOTS_GameplayTagManagerSubsystem;

/**
 * Simple Blueprint-facing helpers that forward into
 * USOTS_GameplayTagManagerSubsystem.
 *
 * SOTS TAG SPINE (V2): This file presents the canonical blueprint API
 * and central routing for loose actor gameplay tags. Prefer the
 * functions in this library instead of manipulating tag containers or
 * local plugin-level tag maps directly.
 */
UCLASS()
class SOTS_TAGMANAGER_API USOTS_TagLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Resolve a tag by name using the global SOTS tag manager. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static FGameplayTag GetTagByName(const UObject* WorldContextObject, FName TagName);

    /** Adds a loose gameplay tag to the actor through the tag manager. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static void AddTagToActor(const UObject* WorldContextObject, AActor* Target, FGameplayTag Tag);

    /** Removes a loose gameplay tag from the actor through the tag manager. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static void RemoveTagFromActor(const UObject* WorldContextObject, AActor* Target, FGameplayTag Tag);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static void AddTagToActorByName(const UObject* WorldContextObject, AActor* Target, FName TagName);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static void RemoveTagFromActorByName(const UObject* WorldContextObject, AActor* Target, FName TagName);

    /** Returns true if the actor owns the tag. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static bool ActorHasTag(const UObject* WorldContextObject, const AActor* Target, FGameplayTag Tag);

    /** Returns true if the actor owns the tag identified by TagName. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static bool ActorHasTagByName(const UObject* WorldContextObject, const AActor* Actor, FName TagName);

    /** Removes a set of loose gameplay tags previously added to the given actor through the tag manager. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static void RemoveTagsFromActor(const UObject* WorldContextObject, AActor* Target, const TArray<FGameplayTag>& Tags);

    /** Returns true if the actor has any tag in the given container. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static bool ActorHasAnyTag(const UObject* WorldContextObject, const AActor* Target, const FGameplayTagContainer& Tags);

    /** Returns true if the actor has all tags in the given container. */
    UFUNCTION(BlueprintPure, Category = "SOTS|Tags", meta = (WorldContext = "WorldContextObject"))
    static bool ActorHasAllTags(const UObject* WorldContextObject, const AActor* Target, const FGameplayTagContainer& Tags);

private:
    /** Internal helper to get the manager subsystem from a world context. */
    static USOTS_GameplayTagManagerSubsystem* GetManager(const UObject* WorldContextObject);
};
