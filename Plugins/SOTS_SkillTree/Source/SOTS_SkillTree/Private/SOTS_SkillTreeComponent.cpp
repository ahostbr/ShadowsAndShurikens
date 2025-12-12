#include "SOTS_SkillTreeComponent.h"
#include "SOTS_SkillTreeDefinition.h"
#include "SOTS_SkillTreeSubsystem.h"
#include "GameFramework/Actor.h"

USOTS_SkillTreeComponent::USOTS_SkillTreeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    AvailablePoints = 0;
}

void USOTS_SkillTreeComponent::BeginPlay()
{
    Super::BeginPlay();

    if (SkillTreeDefinition && NodeStates.Num() == 0)
    {
        InitializeSkillTree(SkillTreeDefinition, AvailablePoints);
    }
}

void USOTS_SkillTreeComponent::InitializeSkillTree(USOTS_SkillTreeDefinition* InDefinition, int32 StartingPoints)
{
    SkillTreeDefinition = InDefinition;
    AvailablePoints = FMath::Max(0, StartingPoints);
    NodeStates.Empty();

    if (SkillTreeDefinition)
    {
        NodeStates.Reserve(SkillTreeDefinition->Nodes.Num());
        for (const FSOTS_SkillNodeDefinition& NodeDef : SkillTreeDefinition->Nodes)
        {
            FSOTS_SkillNodeState NewState;
            NewState.NodeId = NodeDef.NodeId;
            NewState.Status = ESOTS_SkillNodeStatus::Locked;
            NodeStates.Add(NewState);
        }
        // Ensure the global subsystem is aware of this tree so that
        // granted tags can be aggregated for GAS and other systems.
        if (UWorld* World = GetWorld())
        {
            if (UGameInstance* GI = World->GetGameInstance())
            {
                if (USOTS_SkillTreeSubsystem* Subsys = GI->GetSubsystem<USOTS_SkillTreeSubsystem>())
                {
                    Subsys->RegisterSkillTree(SkillTreeDefinition);
                }
            }
        }
    }

    OnSkillPointsChanged.Broadcast(AvailablePoints);
}

void USOTS_SkillTreeComponent::AddSkillPoints(int32 Amount)
{
    if (Amount == 0)
    {
        return;
    }

    AvailablePoints = FMath::Max(0, AvailablePoints + Amount);
    OnSkillPointsChanged.Broadcast(AvailablePoints);
}

ESOTS_SkillNodeStatus USOTS_SkillTreeComponent::GetNodeStatus(FName NodeId) const
{
    if (const FSOTS_SkillNodeState* State = FindNodeState(NodeId))
    {
        return State->Status;
    }
    return ESOTS_SkillNodeStatus::Locked;
}

bool USOTS_SkillTreeComponent::CanUnlockNode(FName NodeId, FText& OutFailureReason) const
{
    OutFailureReason = FText::GetEmpty();

    if (!SkillTreeDefinition)
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "NoDefinition", "No skill tree definition assigned.");
        return false;
    }

    if (NodeId.IsNone())
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "InvalidNodeId", "Invalid node id.");
        return false;
    }

    FSOTS_SkillNodeDefinition NodeDef;
    if (!SkillTreeDefinition->FindNodeDefinition(NodeId, NodeDef))
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "NodeNotFound", "Node not found in skill tree definition.");
        return false;
    }

    const FSOTS_SkillNodeState* State = FindNodeState(NodeId);
    if (!State)
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "StateNotFound", "Node state not found.");
        return false;
    }

    if (State->Status == ESOTS_SkillNodeStatus::Unlocked)
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "AlreadyUnlocked", "Node is already unlocked.");
        return false;
    }

    if (AvailablePoints < NodeDef.Cost)
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "NotEnoughPoints", "Not enough skill points.");
        return false;
    }

    if (!ArePrereqsMet(NodeDef))
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "PrereqsNotMet", "Prerequisite nodes are not unlocked.");
        return false;
    }

    // Tag-based requirements are intentionally left for SOTS to handle externally.

    return true;
}

bool USOTS_SkillTreeComponent::UnlockNode(FName NodeId, FText& OutFailureReason)
{
    if (!SkillTreeDefinition)
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "NoDefinition", "No skill tree definition assigned.");
        return false;
    }

    FSOTS_SkillNodeDefinition NodeDef;
    if (!SkillTreeDefinition->FindNodeDefinition(NodeId, NodeDef))
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "NodeNotFound", "Node not found in skill tree definition.");
        return false;
    }

    if (!CanUnlockNode(NodeId, OutFailureReason))
    {
        return false;
    }

    FSOTS_SkillNodeState* State = FindNodeStateMutable(NodeId);
    if (!State)
    {
        OutFailureReason = NSLOCTEXT("SOTSSkillTree", "StateNotFound", "Node state not found.");
        return false;
    }

    AvailablePoints -= NodeDef.Cost;
    AvailablePoints = FMath::Max(0, AvailablePoints);
    State->Status = ESOTS_SkillNodeStatus::Unlocked;

    OnSkillPointsChanged.Broadcast(AvailablePoints);
    OnNodeStatusChanged.Broadcast(NodeId, State->Status);

    // Mirror this unlock into the global skill tree subsystem so that
    // granted tags are reflected in USOTS_TagStateSubsystem.
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (USOTS_SkillTreeSubsystem* Subsys = GI->GetSubsystem<USOTS_SkillTreeSubsystem>())
            {
                Subsys->UnlockNode(SkillTreeDefinition ? SkillTreeDefinition->TreeId : NAME_None, NodeId);
            }
        }
    }

    return true;
}

FSOTS_SkillNodeState* USOTS_SkillTreeComponent::FindNodeStateMutable(FName NodeId)
{
    if (NodeId.IsNone())
    {
        return nullptr;
    }

    for (FSOTS_SkillNodeState& State : NodeStates)
    {
        if (State.NodeId == NodeId)
        {
            return &State;
        }
    }

    return nullptr;
}

const FSOTS_SkillNodeState* USOTS_SkillTreeComponent::FindNodeState(FName NodeId) const
{
    if (NodeId.IsNone())
    {
        return nullptr;
    }

    for (const FSOTS_SkillNodeState& State : NodeStates)
    {
        if (State.NodeId == NodeId)
        {
            return &State;
        }
    }

    return nullptr;
}

bool USOTS_SkillTreeComponent::ArePrereqsMet(const FSOTS_SkillNodeDefinition& NodeDef) const
{
    for (const FName& PrereqId : NodeDef.PrerequisiteNodeIds)
    {
        if (GetNodeStatus(PrereqId) != ESOTS_SkillNodeStatus::Unlocked)
        {
            return false;
        }
    }

    return true;
}
