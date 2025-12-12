#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "BlueprintCommentLinksTypes.h"

class SGraphPanel;
class UBlueprint;

/**
 * Overlay drawn on top of an SGraphPanel to visualize links between comment nodes.
 */
class BLUEPRINTCOMMENTLINKSEDITOR_API SCommentLinkOverlay : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCommentLinkOverlay) {}
        SLATE_ARGUMENT(SGraphPanel*, GraphPanel)
        SLATE_ARGUMENT(TWeakObjectPtr<UBlueprint>, Blueprint)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // SWidget overrides
    virtual int32 OnPaint(const FPaintArgs& Args,
                          const FGeometry& AllottedGeometry,
                          const FSlateRect& MyCullingRect,
                          FSlateWindowElementList& OutDrawElements,
                          int32 LayerId,
                          const FWidgetStyle& InWidgetStyle,
                          bool bParentEnabled) const override;

    virtual void Tick(const FGeometry& AllottedGeometry,
                      const double InCurrentTime,
                      const float InDeltaTime) override;

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
    /** Returns true if this overlay is associated with the given graph panel. */
    bool IsAttachedTo(SGraphPanel* InPanel) const { return GraphPanelPtr == InPanel; }

    void SetLinkModeActive(bool bInActive);
    bool IsLinkModeActive() const { return bLinkModeActive; }

    /** Called by the module/editor when the underlying links change. */
    void RefreshCachedLinks();

private:
    struct FCommentHandleInfo
    {
        FGuid CommentGuid;
        // Handle position in overlay local space (for hit testing)
        FVector2D LocalPosition = FVector2D::ZeroVector;
        // Absolute screen position of the handle (in desktop pixels) – useful for debug
        FVector2D ScreenPosition = FVector2D::ZeroVector;
    };

    SGraphPanel* GraphPanelPtr = nullptr;
    TWeakObjectPtr<UBlueprint> BlueprintPtr;

    // Whether we're in "link creation" mode globally
    bool bLinkModeActive = false;

    // Dragging state when creating a new link
    bool bIsCreatingLink = false;
    FGuid PendingFromGuid;
    FVector2D PendingMousePos = FVector2D::ZeroVector;

    struct FRenderedLink
    {
        FGuid LinkGuid;
        FVector2D From = FVector2D::ZeroVector;
        FVector2D To = FVector2D::ZeroVector;
    };

    /** Cached list of links for the current graph. */
    TArray<FBlueprintCommentLink> CachedLinks;
    mutable TArray<FRenderedLink> LastRenderedLinks;
    // Handle info for the last paint pass (used for hit testing).
    mutable TArray<FCommentHandleInfo> LastRenderedHandles;

    // Radius in screen pixels used to detect clicks on a handle.
    static constexpr float HandleHitRadius = 8.0f;

    /** Currently selected link, if any. */
    FGuid SelectedLinkGuid;

private:
    void CacheLinksFromBlueprint();

    /** Hit test against rendered links, used for click/selection. */
    bool HitTestLink(const FVector2D& LocalMousePos, FGuid& OutLinkGuid) const;

    /** Hit test against handle points on comment boxes. */
    bool HitTestHandle(const FVector2D& ScreenPos, FGuid& OutCommentGuid) const;

    /** Look up a handle screen position for a given comment node guid. */
    bool TryGetHandleScreenPosition(const FGuid& CommentGuid, FVector2D& OutScreenPos) const;

    /** Helper: find comment node under mouse (by NodeGuid) using the graph panel. */
    bool GetCommentNodeUnderMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, FGuid& OutNodeGuid) const;

    enum class ECommentHandleSide : uint8
    {
        Left,
        Right,
        Top,
        Bottom,
        Center
    };

    /** Compute the absolute screen position for a handle on a given comment node. */
    FVector2D GetHandleScreenPosition(const FGuid& CommentGuid, ECommentHandleSide Side) const;

    FReply OpenLinkContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, const FGuid& LinkGuid);

    void JumpToLinkTarget(const FGuid& LinkGuid);
    void FocusLinkSource();
    void FocusLinkTarget();
    void DeleteLink();
    void EditLinkTag(const FGuid& LinkGuid);
};
