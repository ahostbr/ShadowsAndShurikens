#pragma once

#include "IDetailCustomization.h"

/**
 * Details customization for UEdGraphNode_Comment to expose Comment Link style
 * options (handle color, wire color, arrow size, and read-only tag summary).
 */
class BLUEPRINTCOMMENTLINKSEDITOR_API FBlueprintCommentLinkCommentDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    void BuildCommentLinksCategory(IDetailLayoutBuilder& DetailBuilder, class UEdGraphNode_Comment* CommentNode);
};

