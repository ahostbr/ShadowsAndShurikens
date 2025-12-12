#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SOTS_SkillTreeTypes.h"
#include "SOTS_SkillTreeComponent.generated.h"

class USOTS_SkillTreeDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_SkillTreeNodeChangedSignature, FName, NodeId, ESOTS_SkillNodeStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_SkillTreePointsChangedSignature, int32, NewPoints);

/**
 * Per-player skill tree component.
 *
 * Holds runtime state (unlocked/locked nodes, available points) and exposes
 * a minimal, data-driven API that SOTS can extend and hook into.
 */
UCLASS(BlueprintType, ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_SKILLTREE_API USOTS_SkillTreeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_SkillTreeComponent();

protected:
    virtual void BeginPlay() override;

public:
    // The tree definition that this component instance uses.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SkillTree")
    TObjectPtr<USOTS_SkillTreeDefinition> SkillTreeDefinition;

    // Current available points for this tree.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SkillTree")
    int32 AvailablePoints;

    // Runtime state for each node (mirrors the definition's Nodes array).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SkillTree")
    TArray<FSOTS_SkillNodeState> NodeStates;

    // Broadcast whenever a node status changes (e.g., unlocked).
    UPROPERTY(BlueprintAssignable, Category="SkillTree")
    FSOTS_SkillTreeNodeChangedSignature OnNodeStatusChanged;

    // Broadcast whenever the available point total changes.
    UPROPERTY(BlueprintAssignable, Category="SkillTree")
    FSOTS_SkillTreePointsChangedSignature OnSkillPointsChanged;

public:
    /**
     * Initializes the skill tree from a definition and starting point pool.
     * Safe to call at runtime to reset or swap trees.
     */
    UFUNCTION(BlueprintCallable, Category="SkillTree")
    void InitializeSkillTree(USOTS_SkillTreeDefinition* InDefinition, int32 StartingPoints);

    /**
     * Adds (or removes, if negative) skill points and broadcasts the change.
     */
    UFUNCTION(BlueprintCallable, Category="SkillTree")
    void AddSkillPoints(int32 Amount);

    /**
     * Returns the current status of a node (defaults to Locked if unknown).
     */
    UFUNCTION(BlueprintCallable, Category="SkillTree")
    ESOTS_SkillNodeStatus GetNodeStatus(FName NodeId) const;

    /**
     * Checks whether a node can be unlocked, returning a localized failure reason.
     */
    UFUNCTION(BlueprintCallable, Category="SkillTree")
    bool CanUnlockNode(FName NodeId, FText& OutFailureReason) const;

    /**
     * Attempts to unlock a node. On success, points are spent and events are broadcast.
     */
    UFUNCTION(BlueprintCallable, Category="SkillTree")
    bool UnlockNode(FName NodeId, FText& OutFailureReason);

    UFUNCTION(BlueprintCallable, Category="SkillTree")
    bool IsNodeUnlocked(FName NodeId) const
    {
        return GetNodeStatus(NodeId) == ESOTS_SkillNodeStatus::Unlocked;
    }

private:
    FSOTS_SkillNodeState* FindNodeStateMutable(FName NodeId);
    const FSOTS_SkillNodeState* FindNodeState(FName NodeId) const;
    bool ArePrereqsMet(const FSOTS_SkillNodeDefinition& NodeDef) const;
};