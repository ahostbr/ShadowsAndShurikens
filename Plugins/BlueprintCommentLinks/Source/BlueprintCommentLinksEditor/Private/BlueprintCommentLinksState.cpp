#include "BlueprintCommentLinksState.h"

#include "SGraphNode.h"

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

void FBlueprintCommentLinksState::RegisterCommentWidget(const FGuid& CommentGuid, TSharedPtr<SGraphNode> Widget)
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

TSharedPtr<SGraphNode> FBlueprintCommentLinksState::GetCommentWidget(const FGuid& CommentGuid) const
{
    if (!CommentGuid.IsValid())
    {
        return nullptr;
    }

    if (const TWeakPtr<SGraphNode>* Found = CommentToWidget.Find(CommentGuid))
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

    const TWeakPtr<SGraphNode>* Found = CommentToWidget.Find(CommentGuid);
    return Found != nullptr && Found->IsValid();
}
