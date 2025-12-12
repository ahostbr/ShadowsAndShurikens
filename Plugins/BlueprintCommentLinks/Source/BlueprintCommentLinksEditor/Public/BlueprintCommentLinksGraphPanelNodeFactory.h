#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

/**
 * Graph panel node factory that replaces the default Slate widget for
 * Blueprint comment nodes with SBlueprintCommentLinksGraphNode.
 */
class FBlueprintCommentLinksGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
public:
    virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* InNode) const override;
};
