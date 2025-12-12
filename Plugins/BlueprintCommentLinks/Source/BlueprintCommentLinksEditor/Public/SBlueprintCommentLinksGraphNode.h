#pragma once

#include "CoreMinimal.h"
#include "SGraphNodeComment.h"

class UEdGraphNode_Comment;

/**
 * Custom Slate node for Blueprint comment boxes.
 * For now this behaves like a normal comment node but also registers
 * itself with FBlueprintCommentLinksState so that comment GUIDs can be
 * associated with widgets.
 */
class BLUEPRINTCOMMENTLINKSEDITOR_API SBlueprintCommentLinksGraphNode : public SGraphNodeComment
{
public:
    SLATE_BEGIN_ARGS(SBlueprintCommentLinksGraphNode) {}
        SLATE_ARGUMENT(UEdGraphNode_Comment*, CommentNode)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // SGraphNode interface
    virtual void UpdateGraphNode() override;

protected:
    UEdGraphNode_Comment* CommentNode = nullptr;
};
