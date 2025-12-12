#include "BlueprintCommentLinksGraphPanelNodeFactory.h"

#include "EdGraphNode_Comment.h"
#include "SBlueprintCommentLinksGraphNode.h"

TSharedPtr<SGraphNode> FBlueprintCommentLinksGraphPanelNodeFactory::CreateNode(UEdGraphNode* InNode) const
{
    if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(InNode))
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: NodeFactory replacing comment node %s"), *CommentNode->GetName());

        TSharedRef<SBlueprintCommentLinksGraphNode> GraphNode =
            SNew(SBlueprintCommentLinksGraphNode)
            .CommentNode(CommentNode);

        GraphNode->SlatePrepass();
        return GraphNode;
    }

    return nullptr;
}
