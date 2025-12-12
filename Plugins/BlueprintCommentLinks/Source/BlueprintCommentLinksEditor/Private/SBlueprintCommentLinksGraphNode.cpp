#include "SBlueprintCommentLinksGraphNode.h"

#include "EdGraphNode_Comment.h"
#include "BlueprintCommentLinksState.h"

void SBlueprintCommentLinksGraphNode::Construct(const FArguments& InArgs)
{
    CommentNode = InArgs._CommentNode;
    GraphNode   = CommentNode;

    SGraphNodeComment::Construct(
        SGraphNodeComment::FArguments(),
        CommentNode);

    if (CommentNode)
    {
        FBlueprintCommentLinksState::Get().RegisterCommentWidget(CommentNode->NodeGuid, SharedThis(this));
        UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: RegisterCommentWidget – %s (%s)"),
            *CommentNode->GetName(),
            *CommentNode->NodeGuid.ToString());
    }
}

void SBlueprintCommentLinksGraphNode::UpdateGraphNode()
{
    SGraphNodeComment::UpdateGraphNode();

    if (CommentNode)
    {
        FBlueprintCommentLinksState::Get().RegisterCommentWidget(CommentNode->NodeGuid, SharedThis(this));
        UE_LOG(LogTemp, Verbose, TEXT("BlueprintCommentLinks: UpdateGraphNode register – %s (%s)"),
            *CommentNode->GetName(),
            *CommentNode->NodeGuid.ToString());
    }
}

