#include "BlueprintCommentLinksState.h"

FBlueprintCommentLinksState& FBlueprintCommentLinksState::Get()
{
    static FBlueprintCommentLinksState Instance;
    return Instance;
}

void FBlueprintCommentLinksState::TearDown()
{
    FBlueprintCommentLinksState& Instance = Get();
    Instance.CommentToWidget.Reset();
}

void FBlueprintCommentLinksState::RegisterCommentWidget(const FGuid& CommentGuid, TSharedPtr<SBlueprintCommentLinksGraphNode> Widget)
{
    if (!CommentGuid.IsValid() || !Widget.IsValid())
    {
        return;
    }

    CommentToWidget.FindOrAdd(CommentGuid) = Widget;
}

void FBlueprintCommentLinksState::UnregisterCommentWidget(const FGuid& CommentGuid)
{
    CommentToWidget.Remove(CommentGuid);
}

TSharedPtr<SBlueprintCommentLinksGraphNode> FBlueprintCommentLinksState::GetCommentWidget(const FGuid& CommentGuid) const
{
    if (!CommentGuid.IsValid())
    {
        return nullptr;
    }

    if (const TWeakPtr<SBlueprintCommentLinksGraphNode>* Found = CommentToWidget.Find(CommentGuid))
    {
        return Found->Pin();
    }

    return nullptr;
}

bool FBlueprintCommentLinksState::HasRegisteredComment(const FGuid& CommentGuid) const
{
    if (!CommentGuid.IsValid())
    {
        return false;
    }

    const TWeakPtr<SBlueprintCommentLinksGraphNode>* Found = CommentToWidget.Find(CommentGuid);
    return Found != nullptr && Found->IsValid();
}
