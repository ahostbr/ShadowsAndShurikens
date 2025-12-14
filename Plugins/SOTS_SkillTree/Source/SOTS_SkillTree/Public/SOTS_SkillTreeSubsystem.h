#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_SkillTreeTypes.h"
#include "SOTS_SkillTreeSubsystem.generated.h"

class USOTS_SkillTreeDefinition;
class USOTS_GameplayTagManagerSubsystem;
class AActor;

/**
 * Global, backend-only skill tree manager for SOTS.
 *
 * Current structure (v1.0 pattern):
 * - Data: USOTS_SkillTreeDefinition holds a TreeId and an array of
 *   FSOTS_SkillNodeDefinition records (id, text, layout, requirements,
 *   effects). Trees are authored as content-only DataAssets.
 * - Runtime: USOTS_SkillTreeSubsystem owns FSOTS_SkillTreeRuntimeState per
 *   TreeId (unlocked nodes + available points) and mirrors granted tags into
 *   the global tag state so other systems (GAS, MissionDirector, GSM) can
 *   reason about skills via tags.
 * - Frontend: optional USOTS_SkillTreeComponent instances live on actors and
 *   forward unlocks into this subsystem; UI and other systems are expected
 *   to call the subsystem directly for queries and save/load.
 */
UCLASS()
class SOTS_SKILLTREE_API USOTS_SkillTreeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillTreeStateChanged, const FSOTS_SkillTreeRuntimeState&, NewState);

    // Registers a tree definition for runtime use.
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    void RegisterSkillTree(USOTS_SkillTreeDefinition* TreeDef);

    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    bool IsNodeUnlocked(FName TreeId, FName NodeId) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    bool CanUnlockNode(FName TreeId, FName NodeId) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    bool UnlockNode(FName TreeId, FName NodeId);

    // High-level unlock helper that evaluates prerequisites, requirements,
    // and available points, returning a localized failure reason on error.
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree", meta=(WorldContext="WorldContextObject"))
    bool TryUnlockNode(const UObject* WorldContextObject, FName TreeId, FName NodeId, FText& OutFailureReason);

    // Convenience wrapper matching the naming used in higher-level
    // systems (UnlockSkillNode -> UnlockNode).
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    bool UnlockSkillNode(FName TreeId, FName NodeId) { return UnlockNode(TreeId, NodeId); }

    // Attempts to refund a previously unlocked node. This will remove its
    // granted tags from the global tag state and mark it as locked again.
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    bool RefundSkillNode(FName TreeId, FName NodeId);

    // Adjusts the point pool associated with a tree. The subsystem
    // does not enforce cost rules directly; callers may use this for
    // save/load or meta-progression.
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    void AddSkillPoints(FName TreeId, int32 Delta);

    // Tags granted by all unlocked nodes in this tree.
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    FGameplayTagContainer GetGrantedTagsForTree(FName TreeId) const;

    // Debug: get runtime state snapshot for a single tree.
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    FSOTS_SkillTreeRuntimeState GetRuntimeState(FName TreeId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree")
    TArray<FSOTS_SkillTreeRuntimeState> GetAllRuntimeStates() const;

    // Clear all runtime unlock state for all trees.
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    void ResetAllSkillTrees();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree")
    TArray<FName> GetRegisteredTreeIds() const;

    // Allows an external save system to restore the entire runtime state.
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    void RestoreRuntimeStates(const TArray<FSOTS_SkillTreeRuntimeState>& InStates);

    // UI-friendly helpers

    // Returns the current status for a given node in a tree
    // (Locked / Available / Unlocked).
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree")
    ESOTS_SkillNodeStatus GetNodeStatus(FName TreeId, FName NodeId) const;

    // Returns a per-node overview for the specified tree including
    // definition data, status, and effective cost.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree")
    void GetSkillTreeOverview(FName TreeId, TArray<FSOTS_SkillTreeNodeView>& OutNodes) const;

    // Returns the node ids that are currently unlockable for the given tree.
    // This uses basic parent gating and runtime state; more detailed checks
    // (mission/ability requirements) can be performed via TryUnlockNode.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree")
    void GetAvailableNodes(FName TreeId, TArray<FName>& OutNodeIds) const;

    // Blessed gating helpers (read-only). These operate on node gameplay tags
    // authored on FSOTS_SkillNodeDefinition::SkillTag and never mutate state.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree|Gating", meta=(DisplayName="Is Node Unlocked (Tag)"))
    bool IsNodeUnlockedByTag(FGameplayTag NodeTag) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree|Gating")
    bool AreAllNodesUnlocked(const TArray<FGameplayTag>& NodeTags) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree|Gating")
    bool IsAnyNodeUnlocked(const TArray<FGameplayTag>& NodeTags) const;

    // Broadcast whenever a tree's runtime state changes.
    UPROPERTY(BlueprintAssignable, Category="SOTS|SkillTree")
    FOnSkillTreeStateChanged OnSkillTreeStateChanged;

    // Convenience helper for code that only cares about the presence of a
    // single skill tag. By default this simply queries the global tag state.
    UFUNCTION(BlueprintPure, Category="SOTS|SkillTree")
    bool HasSkillTag(FGameplayTag SkillTag) const;

    // Gating helpers for other subsystems that consume skill tags.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree|Gating")
    bool CanGrantAbility(FGameplayTag AbilityTag) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree|Gating")
    bool CanRaiseStat(FGameplayTag StatTag) const;

    // --- Save / load helpers ---

    void BuildProfileData(FSOTS_SkillTreeProfileData& OutData) const;
    void ApplyProfileData(const FSOTS_SkillTreeProfileData& InData);

    // Serializes all tree runtime states into a profile-level container.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree")
    FSOTS_SkillTreeProfileState SaveSkillTreeState() const;

    // Restores previously saved tree runtime states. This function does not
    // mutate the global tag state; callers can optionally re-apply tags or
    // call RestoreRuntimeStates directly if needed.
    UFUNCTION(BlueprintCallable, Category="SOTS|SkillTree")
    void LoadSkillTreeState(const FSOTS_SkillTreeProfileState& InState);

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    // Registered tree definitions by ID.
    UPROPERTY()
    TMap<FName, TObjectPtr<USOTS_SkillTreeDefinition>> RegisteredTrees;

    // Runtime unlocked states per tree ID.
    UPROPERTY()
    TMap<FName, FSOTS_SkillTreeRuntimeState> RuntimeStates;

    const FSOTS_SkillNodeDefinition* FindNodeDef(FName TreeId, FName NodeId) const;

    int32 GetEffectiveCostForNode(const FSOTS_SkillNodeDefinition& NodeDef) const;

    void AddTagsFromContainer(USOTS_GameplayTagManagerSubsystem* TagSubsystem, AActor* PlayerActor, const FGameplayTagContainer& Tags) const;
    void ReapplyTagsForRuntimeState(AActor* PlayerActor, USOTS_GameplayTagManagerSubsystem* TagSubsystem) const;
};
