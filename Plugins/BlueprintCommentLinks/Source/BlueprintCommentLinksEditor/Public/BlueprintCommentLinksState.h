#pragma once

#include "CoreMinimal.h"

class SBlueprintCommentLinksGraphNode;

/**
 * Transient singleton mapping from comment GUIDs to Slate widgets.
 * This is used purely for editor-time highlighting and navigation.
 */
struct BLUEPRINTCOMMENTLINKSEDITOR_API FBlueprintCommentLinksState
{
public:
    static FBlueprintCommentLinksState& Get();
    static void TearDown();

    /** Map from comment node guid to the associated Slate widget. */
    TMap<FGuid, TWeakPtr<SBlueprintCommentLinksGraphNode>> CommentToWidget;

    void RegisterCommentWidget(const FGuid& CommentGuid, TSharedPtr<SBlueprintCommentLinksGraphNode> Widget);
    void UnregisterCommentWidget(const FGuid& CommentGuid);

    TSharedPtr<SBlueprintCommentLinksGraphNode> GetCommentWidget(const FGuid& CommentGuid) const;
    bool HasRegisteredComment(const FGuid& CommentGuid) const;
};
