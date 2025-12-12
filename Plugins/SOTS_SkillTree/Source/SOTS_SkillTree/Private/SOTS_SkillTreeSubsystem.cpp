#include "SOTS_SkillTreeSubsystem.h"

#include "SOTS_SkillTreeDefinition.h"
#include "SOTS_SkillTreeModule.h"
#include "SOTS_FXManagerSubsystem.h"
#include "SOTS_TagAccessHelpers.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    static AActor* SOTS_GetPrimaryPlayerActor(const UObject* WorldContextObject)
    {
        if (!WorldContextObject)
        {
            return nullptr;
        }

        return UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
    }
}

void USOTS_SkillTreeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void USOTS_SkillTreeSubsystem::Deinitialize()
{
    RuntimeStates.Reset();
    RegisteredTrees.Reset();

    Super::Deinitialize();
}

void USOTS_SkillTreeSubsystem::RegisterSkillTree(USOTS_SkillTreeDefinition* TreeDef)
{
    if (!TreeDef || TreeDef->TreeId.IsNone())
    {
        return;
    }

    RegisteredTrees.FindOrAdd(TreeDef->TreeId) = TreeDef;

    FSOTS_SkillTreeRuntimeState& State = RuntimeStates.FindOrAdd(TreeDef->TreeId);
    State.TreeId = TreeDef->TreeId;
}

const FSOTS_SkillNodeDefinition* USOTS_SkillTreeSubsystem::FindNodeDef(FName TreeId, FName NodeId) const
{
    if (TreeId.IsNone() || NodeId.IsNone())
    {
        return nullptr;
    }

    const TObjectPtr<USOTS_SkillTreeDefinition>* TreePtr = RegisteredTrees.Find(TreeId);
    if (!TreePtr)
    {
        return nullptr;
    }

    const USOTS_SkillTreeDefinition* Tree = TreePtr->Get();
    if (!Tree)
    {
        return nullptr;
    }

    for (const FSOTS_SkillNodeDefinition& Node : Tree->Nodes)
    {
        if (Node.NodeId == NodeId)
        {
            return &Node;
        }
    }

    return nullptr;
}

bool USOTS_SkillTreeSubsystem::IsNodeUnlocked(FName TreeId, FName NodeId) const
{
    const FSOTS_SkillTreeRuntimeState* State = RuntimeStates.Find(TreeId);
    if (!State)
    {
        return false;
    }

    for (const FSOTS_UnlockedSkillNode& Node : State->UnlockedNodes)
    {
        if (Node.NodeId == NodeId)
        {
            return true;
        }
    }

    return false;
}

bool USOTS_SkillTreeSubsystem::CanUnlockNode(FName TreeId, FName NodeId) const
{
    const FSOTS_SkillNodeDefinition* NodeDef = FindNodeDef(TreeId, NodeId);
    if (!NodeDef)
    {
        return false;
    }

    if (IsNodeUnlocked(TreeId, NodeId))
    {
        return false;
    }

    // Determine the parent list to use: prefer ParentNodeIds, fall back to legacy PrerequisiteNodeIds.
    const TArray<FName>& Parents =
        (NodeDef->ParentNodeIds.Num() > 0) ? NodeDef->ParentNodeIds : NodeDef->PrerequisiteNodeIds;

    const int32 ParentCount = Parents.Num();

    // No parents and rule says "no parents required".
    if (NodeDef->UnlockRule == ESOTS_SkillUnlockRule::NoParentNeeded)
    {
        return true;
    }

    bool bAllParentsUnlocked = true;
    bool bAnyParentUnlocked = false;

    for (const FName& ParentId : Parents)
    {
        const bool bUnlocked = IsNodeUnlocked(TreeId, ParentId);
        bAllParentsUnlocked &= bUnlocked;
        bAnyParentUnlocked  |= bUnlocked;
    }

    switch (NodeDef->UnlockRule)
    {
    case ESOTS_SkillUnlockRule::AllParents:
        // If there are no parents, treat this as free to unlock.
        return (ParentCount == 0) || bAllParentsUnlocked;

    case ESOTS_SkillUnlockRule::AnyParent:
        // If there are no parents, also treat as free.
        return (ParentCount == 0) || bAnyParentUnlocked;

    case ESOTS_SkillUnlockRule::NoParentNeeded:
    default:
        return true;
    }
}

bool USOTS_SkillTreeSubsystem::UnlockNode(FName TreeId, FName NodeId)
{
    if (!CanUnlockNode(TreeId, NodeId))
    {
        return false;
    }

    FSOTS_SkillTreeRuntimeState& State = RuntimeStates.FindOrAdd(TreeId);
    State.TreeId = TreeId;

    FSOTS_UnlockedSkillNode NewNode;
    NewNode.NodeId = NodeId;
    NewNode.Rank   = 1;

    State.UnlockedNodes.Add(NewNode);

    // Apply granted tags to the global player tag state so that other
    // systems (GAS, MissionDirector, etc.) can reason about unlocked skills.
    const FSOTS_SkillNodeDefinition* NodeDef = FindNodeDef(TreeId, NodeId);
    if (!NodeDef)
    {
        UE_LOG(LogSOTSSkillTree, Warning,
               TEXT("[SkillTree] UnlockNode: Node definition not found for TreeId=%s NodeId=%s."),
               *TreeId.ToString(), *NodeId.ToString());
        return false;
    }

    const int32 EffectiveCost = GetEffectiveCostForNode(*NodeDef);

    // Apply the unlock without performing requirement/point checks. This path
    // is intentionally minimal and primarily used by existing Blueprint graphs
    // that already performed their own validation.
    if (EffectiveCost > 0)
    {
        State.AvailablePoints = FMath::Max(0, State.AvailablePoints - EffectiveCost);
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
    {
        if (AActor* PlayerActor = SOTS_GetPrimaryPlayerActor(this))
        {
            if (NodeDef->SkillTag.IsValid())
            {
                TagSubsystem->AddTagToActor(PlayerActor, NodeDef->SkillTag);
            }

            for (const FGameplayTag& Tag : NodeDef->GrantedTags)
            {
                if (Tag.IsValid())
                {
                    TagSubsystem->AddTagToActor(PlayerActor, Tag);
                }
            }

            for (const FGameplayTag& Tag : NodeDef->Effects.GrantedTags)
            {
                if (Tag.IsValid())
                {
                    TagSubsystem->AddTagToActor(PlayerActor, Tag);
                }
            }
        }
    }

    if (NodeDef->Effects.FXTag_OnUnlock.IsValid())
    {
        if (USOTS_FXManagerSubsystem* FX = USOTS_FXManagerSubsystem::Get())
        {
            FX->TriggerFXByTag(this,
                               NodeDef->Effects.FXTag_OnUnlock,
                               nullptr,
                               nullptr,
                               FVector::ZeroVector,
                               FRotator::ZeroRotator);
        }
    }

    UE_LOG(LogSOTSSkillTree, Log,
           TEXT("[SkillTree] Node unlocked (TreeId=%s, NodeId=%s, Cost=%d, RemainingPoints=%d)."),
           *TreeId.ToString(),
           *NodeId.ToString(),
           EffectiveCost,
           State.AvailablePoints);

    OnSkillTreeStateChanged.Broadcast(State);
    return true;
}

bool USOTS_SkillTreeSubsystem::RefundSkillNode(FName TreeId, FName NodeId)
{
    FSOTS_SkillTreeRuntimeState* State = RuntimeStates.Find(TreeId);
    if (!State)
    {
        return false;
    }

    const FSOTS_SkillNodeDefinition* NodeDef = FindNodeDef(TreeId, NodeId);
    if (!NodeDef || !NodeDef->bRefundable)
    {
        return false;
    }

    int32 IndexToRemove = INDEX_NONE;
    for (int32 Index = 0; Index < State->UnlockedNodes.Num(); ++Index)
    {
        if (State->UnlockedNodes[Index].NodeId == NodeId)
        {
            IndexToRemove = Index;
            break;
        }
    }

    if (IndexToRemove == INDEX_NONE)
    {
        return false;
    }

    State->UnlockedNodes.RemoveAt(IndexToRemove);

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
    {
        if (AActor* PlayerActor = SOTS_GetPrimaryPlayerActor(this))
        {
            if (NodeDef->SkillTag.IsValid())
            {
                TagSubsystem->RemoveTagFromActor(PlayerActor, NodeDef->SkillTag);
            }

            for (const FGameplayTag& Tag : NodeDef->GrantedTags)
            {
                if (Tag.IsValid())
                {
                    TagSubsystem->RemoveTagFromActor(PlayerActor, Tag);
                }
            }
        }
    }

    OnSkillTreeStateChanged.Broadcast(*State);
    return true;
}

void USOTS_SkillTreeSubsystem::AddSkillPoints(FName TreeId, int32 Delta)
{
    if (Delta == 0)
    {
        return;
    }

    FSOTS_SkillTreeRuntimeState& State = RuntimeStates.FindOrAdd(TreeId);
    State.TreeId = TreeId;
    State.AvailablePoints = FMath::Max(0, State.AvailablePoints + Delta);

    OnSkillTreeStateChanged.Broadcast(State);
}

FGameplayTagContainer USOTS_SkillTreeSubsystem::GetGrantedTagsForTree(FName TreeId) const
{
    FGameplayTagContainer Result;

    const FSOTS_SkillTreeRuntimeState* State = RuntimeStates.Find(TreeId);
    const TObjectPtr<USOTS_SkillTreeDefinition>* TreePtr = RegisteredTrees.Find(TreeId);

    if (!State || !TreePtr)
    {
        return Result;
    }

    const USOTS_SkillTreeDefinition* Tree = TreePtr->Get();
    if (!Tree)
    {
        return Result;
    }

    for (const FSOTS_UnlockedSkillNode& Unlocked : State->UnlockedNodes)
    {
        const FSOTS_SkillNodeDefinition* NodeDef = FindNodeDef(TreeId, Unlocked.NodeId);
        if (NodeDef)
        {
            Result.AppendTags(NodeDef->GrantedTags);
        }
    }

    return Result;
}

FSOTS_SkillTreeRuntimeState USOTS_SkillTreeSubsystem::GetRuntimeState(FName TreeId) const
{
    if (const FSOTS_SkillTreeRuntimeState* State = RuntimeStates.Find(TreeId))
    {
        return *State;
    }

    FSOTS_SkillTreeRuntimeState DefaultState;
    DefaultState.TreeId = TreeId;
    return DefaultState;
}

void USOTS_SkillTreeSubsystem::ResetAllSkillTrees()
{
    RuntimeStates.Reset();
}

TArray<FName> USOTS_SkillTreeSubsystem::GetRegisteredTreeIds() const
{
    TArray<FName> Keys;
    RegisteredTrees.GetKeys(Keys);
    return Keys;
}

TArray<FSOTS_SkillTreeRuntimeState> USOTS_SkillTreeSubsystem::GetAllRuntimeStates() const
{
    TArray<FSOTS_SkillTreeRuntimeState> Result;
    RuntimeStates.GenerateValueArray(Result);
    return Result;
}

void USOTS_SkillTreeSubsystem::RestoreRuntimeStates(const TArray<FSOTS_SkillTreeRuntimeState>& InStates)
{
    RuntimeStates.Reset();

    for (const FSOTS_SkillTreeRuntimeState& State : InStates)
    {
        RuntimeStates.Add(State.TreeId, State);
    }

    // Note: TagState synchronization is left to the caller, as the mapping
    // between tree states and global tags may be project-specific.
}

bool USOTS_SkillTreeSubsystem::HasSkillTag(FGameplayTag SkillTag) const
{
    if (!SkillTag.IsValid())
    {
        return false;
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
    {
        if (AActor* PlayerActor = SOTS_GetPrimaryPlayerActor(this))
        {
            return TagSubsystem->ActorHasTag(PlayerActor, SkillTag);
        }
    }

    return false;
}

bool USOTS_SkillTreeSubsystem::CanGrantAbility(FGameplayTag AbilityTag) const
{
    if (!AbilityTag.IsValid())
    {
        return false;
    }

    for (const TPair<FName, FSOTS_SkillTreeRuntimeState>& Pair : RuntimeStates)
    {
        const FSOTS_SkillTreeRuntimeState& State = Pair.Value;
        for (const FSOTS_UnlockedSkillNode& Unlocked : State.UnlockedNodes)
        {
            if (const FSOTS_SkillNodeDefinition* NodeDef = FindNodeDef(Pair.Key, Unlocked.NodeId))
            {
                if (NodeDef->Effects.GrantedAbilityTags.HasTag(AbilityTag))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool USOTS_SkillTreeSubsystem::CanRaiseStat(FGameplayTag StatTag) const
{
    if (!StatTag.IsValid())
    {
        return false;
    }

    for (const TPair<FName, FSOTS_SkillTreeRuntimeState>& Pair : RuntimeStates)
    {
        const FSOTS_SkillTreeRuntimeState& State = Pair.Value;
        for (const FSOTS_UnlockedSkillNode& Unlocked : State.UnlockedNodes)
        {
            if (const FSOTS_SkillNodeDefinition* NodeDef = FindNodeDef(Pair.Key, Unlocked.NodeId))
            {
                if (NodeDef->Effects.GrantedTags.HasTag(StatTag))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool USOTS_SkillTreeSubsystem::TryUnlockNode(
    const UObject* WorldContextObject,
    FName TreeId,
    FName NodeId,
    FText& OutFailureReason)
{
    OutFailureReason = FText::GetEmpty();

    if (TreeId.IsNone() || NodeId.IsNone())
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "InvalidIds", "Invalid tree or node id.");
        return false;
    }

    const FSOTS_SkillNodeDefinition* NodeDef = FindNodeDef(TreeId, NodeId);
    if (!NodeDef)
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "NodeNotFound", "Node not found in skill tree definition.");
        return false;
    }

    if (IsNodeUnlocked(TreeId, NodeId))
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "AlreadyUnlocked", "Node is already unlocked.");
        return false;
    }

    if (!CanUnlockNode(TreeId, NodeId))
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "PrereqsNotMet", "Prerequisite nodes are not unlocked.");
        return false;
    }

    FSOTS_SkillTreeRuntimeState& State = RuntimeStates.FindOrAdd(TreeId);
    State.TreeId = TreeId;

    const int32 EffectiveCost = GetEffectiveCostForNode(*NodeDef);
    if (EffectiveCost > 0 && State.AvailablePoints < EffectiveCost)
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "NotEnoughPoints", "Not enough skill points.");
        return false;
    }

    // Tag/mission/ability requirements evaluated via the central tag manager
    // against the primary player actor.
    USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(WorldContextObject);
    AActor* PlayerActor = SOTS_GetPrimaryPlayerActor(WorldContextObject);

    auto HasAllTagsOnPlayer = [&](const FGameplayTagContainer& RequiredTags) -> bool
    {
        if (!TagSubsystem || !PlayerActor)
        {
            // If we have required tags but no valid context, treat as failure.
            return RequiredTags.IsEmpty();
        }

        for (const FGameplayTag& Tag : RequiredTags)
        {
            if (!Tag.IsValid())
            {
                continue;
            }

            if (!TagSubsystem->ActorHasTag(PlayerActor, Tag))
            {
                return false;
            }
        }

        return true;
    };

    // Required generic tags.
    FGameplayTagContainer CombinedRequiredTags = NodeDef->Requirements.RequiredTags;
    CombinedRequiredTags.AppendTags(NodeDef->RequiredTags);

    if (!CombinedRequiredTags.IsEmpty() && !HasAllTagsOnPlayer(CombinedRequiredTags))
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "RequiredTagsMissing", "Required tags are not present.");
        return false;
    }

    // Mission tags are just additional global tags.
    if (!NodeDef->Requirements.RequiredMissionTags.IsEmpty() &&
        !HasAllTagsOnPlayer(NodeDef->Requirements.RequiredMissionTags))
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "RequiredMissionsMissing", "Required mission state has not been reached.");
        return false;
    }

    // Ability tags are also encoded as global tags in the current design.
    if (!NodeDef->Requirements.RequiredAbilityTags.IsEmpty() &&
        !HasAllTagsOnPlayer(NodeDef->Requirements.RequiredAbilityTags))
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "RequiredAbilitiesMissing", "Required abilities are not yet unlocked.");
        return false;
    }

    UE_LOG(LogSOTSSkillTree, Log,
           TEXT("[SkillTree] TryUnlockNode passed checks (TreeId=%s, NodeId=%s, Cost=%d)."),
           *TreeId.ToString(),
           *NodeId.ToString(),
           EffectiveCost);

    // At this point, all validation has passed; perform the actual unlock
    // using the existing UnlockNode flow.
    if (!UnlockNode(TreeId, NodeId))
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "UnlockFailedInternal", "Internal error while unlocking node.");
        return false;
    }

    return true;
}

ESOTS_SkillNodeStatus USOTS_SkillTreeSubsystem::GetNodeStatus(FName TreeId, FName NodeId) const
{
    if (IsNodeUnlocked(TreeId, NodeId))
    {
        return ESOTS_SkillNodeStatus::Unlocked;
    }

    if (CanUnlockNode(TreeId, NodeId))
    {
        return ESOTS_SkillNodeStatus::Available;
    }

    return ESOTS_SkillNodeStatus::Locked;
}

void USOTS_SkillTreeSubsystem::GetSkillTreeOverview(
    FName TreeId,
    TArray<FSOTS_SkillTreeNodeView>& OutNodes) const
{
    OutNodes.Reset();

    const TObjectPtr<USOTS_SkillTreeDefinition>* TreePtr = RegisteredTrees.Find(TreeId);
    if (!TreePtr)
    {
        return;
    }

    const USOTS_SkillTreeDefinition* Tree = TreePtr->Get();
    if (!Tree)
    {
        return;
    }

    for (const FSOTS_SkillNodeDefinition& NodeDef : Tree->Nodes)
    {
        FSOTS_SkillTreeNodeView View;
        View.Definition   = NodeDef;
        View.Status       = GetNodeStatus(TreeId, NodeDef.NodeId);
        View.EffectiveCost = GetEffectiveCostForNode(NodeDef);

        OutNodes.Add(View);
    }
}

void USOTS_SkillTreeSubsystem::GetAvailableNodes(FName TreeId, TArray<FName>& OutNodeIds) const
{
    OutNodeIds.Reset();

    const TObjectPtr<USOTS_SkillTreeDefinition>* TreePtr = RegisteredTrees.Find(TreeId);
    if (!TreePtr)
    {
        return;
    }

    const USOTS_SkillTreeDefinition* Tree = TreePtr->Get();
    if (!Tree)
    {
        return;
    }

    for (const FSOTS_SkillNodeDefinition& NodeDef : Tree->Nodes)
    {
        if (GetNodeStatus(TreeId, NodeDef.NodeId) == ESOTS_SkillNodeStatus::Available)
        {
            OutNodeIds.Add(NodeDef.NodeId);
        }
    }
}

void USOTS_SkillTreeSubsystem::BuildProfileData(FSOTS_SkillTreeProfileData& OutData) const
{
    OutData.UnlockedSkillNodes.Reset();
    OutData.UnspentSkillPoints = 0;

    for (const TPair<FName, FSOTS_SkillTreeRuntimeState>& Pair : RuntimeStates)
    {
        const FSOTS_SkillTreeRuntimeState& State = Pair.Value;
        OutData.UnspentSkillPoints += State.AvailablePoints;

        for (const FSOTS_UnlockedSkillNode& Unlocked : State.UnlockedNodes)
        {
            if (const FSOTS_SkillNodeDefinition* NodeDef = FindNodeDef(Pair.Key, Unlocked.NodeId))
            {
                if (NodeDef->SkillTag.IsValid())
                {
                    OutData.UnlockedSkillNodes.AddUnique(NodeDef->SkillTag);
                }
            }
        }
    }
}

void USOTS_SkillTreeSubsystem::ApplyProfileData(const FSOTS_SkillTreeProfileData& InData)
{
    RuntimeStates.Reset();

    for (const TPair<FName, TObjectPtr<USOTS_SkillTreeDefinition>>& Pair : RegisteredTrees)
    {
        if (!Pair.Value)
        {
            continue;
        }

        FSOTS_SkillTreeRuntimeState& State = RuntimeStates.FindOrAdd(Pair.Key);
        State.TreeId = Pair.Key;
        State.AvailablePoints = InData.UnspentSkillPoints;

        const USOTS_SkillTreeDefinition* TreeDef = Pair.Value.Get();
        for (const FSOTS_SkillNodeDefinition& NodeDef : TreeDef->Nodes)
        {
            if (NodeDef.SkillTag.IsValid() && InData.UnlockedSkillNodes.Contains(NodeDef.SkillTag))
            {
                FSOTS_UnlockedSkillNode Unlocked;
                Unlocked.NodeId = NodeDef.NodeId;
                State.UnlockedNodes.Add(Unlocked);
            }
        }
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(this))
    {
        if (AActor* PlayerActor = SOTS_GetPrimaryPlayerActor(this))
        {
            ReapplyTagsForRuntimeState(PlayerActor, TagSubsystem);
        }
    }
}

FSOTS_SkillTreeProfileState USOTS_SkillTreeSubsystem::SaveSkillTreeState() const
{
    FSOTS_SkillTreeProfileState Result;
    RuntimeStates.GenerateValueArray(Result.Trees);
    return Result;
}

void USOTS_SkillTreeSubsystem::LoadSkillTreeState(const FSOTS_SkillTreeProfileState& InState)
{
    RestoreRuntimeStates(InState.Trees);
}

int32 USOTS_SkillTreeSubsystem::GetEffectiveCostForNode(const FSOTS_SkillNodeDefinition& NodeDef) const
{
    if (NodeDef.Requirements.RequiredPoints > 0)
    {
        return NodeDef.Requirements.RequiredPoints;
    }

    return FMath::Max(0, NodeDef.Cost);
}

void USOTS_SkillTreeSubsystem::AddTagsFromContainer(
    USOTS_GameplayTagManagerSubsystem* TagSubsystem,
    AActor* PlayerActor,
    const FGameplayTagContainer& Tags) const
{
    if (!TagSubsystem || !PlayerActor)
    {
        return;
    }

    for (const FGameplayTag& Tag : Tags)
    {
        if (Tag.IsValid())
        {
            TagSubsystem->AddTagToActor(PlayerActor, Tag);
        }
    }
}

void USOTS_SkillTreeSubsystem::ReapplyTagsForRuntimeState(
    AActor* PlayerActor,
    USOTS_GameplayTagManagerSubsystem* TagSubsystem) const
{
    if (!TagSubsystem || !PlayerActor)
    {
        return;
    }

    for (const TPair<FName, FSOTS_SkillTreeRuntimeState>& Pair : RuntimeStates)
    {
        for (const FSOTS_UnlockedSkillNode& Unlocked : Pair.Value.UnlockedNodes)
        {
            if (const FSOTS_SkillNodeDefinition* NodeDef = FindNodeDef(Pair.Key, Unlocked.NodeId))
            {
                if (NodeDef->SkillTag.IsValid())
                {
                    TagSubsystem->AddTagToActor(PlayerActor, NodeDef->SkillTag);
                }

                AddTagsFromContainer(TagSubsystem, PlayerActor, NodeDef->GrantedTags);
                AddTagsFromContainer(TagSubsystem, PlayerActor, NodeDef->Effects.GrantedTags);
                AddTagsFromContainer(TagSubsystem, PlayerActor, NodeDef->Effects.GrantedAbilityTags);
            }
        }
    }
}
