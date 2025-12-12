#include "SOTS_SkillTreeDefinition.h"

bool USOTS_SkillTreeDefinition::FindNodeDefinition(FName NodeId, FSOTS_SkillNodeDefinition& OutDefinition) const
{
    if (NodeId.IsNone())
    {
        return false;
    }

    for (const FSOTS_SkillNodeDefinition& Node : Nodes)
    {
        if (Node.NodeId == NodeId)
        {
            OutDefinition = Node;
            return true;
        }
    }

    return false;
}