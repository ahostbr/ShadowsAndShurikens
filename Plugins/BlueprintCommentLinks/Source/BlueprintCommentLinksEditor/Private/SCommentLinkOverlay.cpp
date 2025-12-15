#include "SCommentLinkOverlay.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphNode_Comment.h"
#include "Engine/Blueprint.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "SGraphPanel.h"
#include "BlueprintCommentLinksCacheFile.h"
#include "BlueprintCommentLinksState.h"
#include "SBlueprintCommentLinksGraphNode.h"
#include "SGraphNode.h"
#include "SGraphNodeComment.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"        // SVerticalBox, SHorizontalBox
#include "Widgets/Input/SButton.h"
#include "Widgets/SWindow.h"

// Local helpers for resolving comment nodes and style sources.
namespace BlueprintCommentLinksHelpers
{
    static UEdGraphNode_Comment* FindCommentNode(UEdGraph* Graph, const FGuid& Guid)
    {
        if (!Graph || !Guid.IsValid())
        {
            return nullptr;
        }

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(Node))
            {
                if (CommentNode->NodeGuid == Guid)
                {
                    return CommentNode;
                }
            }
        }

        return nullptr;
    }

    /**
     * Find the "style owner" comment for a given comment.
     *
     * Rules:
     *  1) If this comment is a Source in any link, style from itself.
     *  2) Else, if it is only a Target in links, style from the Source of the first link that points to it.
     *  3) If there are no links involving this comment, return nullptr.
     */
    static UEdGraphNode_Comment* FindStyleSourceComment(
        UEdGraph* Graph,
        const FGuid& CommentGuid,
        const TArray<FBlueprintCommentLink>& Links)
    {
        if (!Graph || !CommentGuid.IsValid())
        {
            return nullptr;
        }

        // 1) If this comment is a source in any link, style from itself.
        for (const FBlueprintCommentLink& Link : Links)
        {
            if (Link.SourceCommentGuid == CommentGuid)
            {
                return FindCommentNode(Graph, CommentGuid);
            }
        }

        // 2) Otherwise, if it is a target, style from the first source that points to it.
        for (const FBlueprintCommentLink& Link : Links)
        {
            if (Link.TargetCommentGuid == CommentGuid)
            {
                return FindCommentNode(Graph, Link.SourceCommentGuid);
            }
        }

        // 3) No links touch this comment.
        return nullptr;
    }
}

using namespace BlueprintCommentLinksHelpers;

void SCommentLinkOverlay::Construct(const FArguments& InArgs)
{
    GraphPanelPtr = InArgs._GraphPanel;
    BlueprintPtr  = InArgs._Blueprint;

    ChildSlot
    [
        SNullWidget::NullWidget
    ];

    bLinkModeActive = false;
    bIsCreatingLink = false;
    PendingFromGuid.Invalidate();

    // Start in visual-only mode; no hit-testing until link mode is enabled.
    SetVisibility(EVisibility::HitTestInvisible);

    RefreshCachedLinks();
}

void SCommentLinkOverlay::CacheLinksFromBlueprint()
{
    CachedLinks.Reset();

    if (!GraphPanelPtr)
    {
        return;
    }

    UEdGraph* Graph = GraphPanelPtr->GetGraphObj();
    if (!Graph)
    {
        return;
    }

    FBlueprintCommentLinkGraphData& Data = UBlueprintCommentLinkCacheFile::Get().GetGraphData(Graph);
    CachedLinks = Data.Links;
}

void SCommentLinkOverlay::RefreshCachedLinks()
{
    CacheLinksFromBlueprint();
    Invalidate(EInvalidateWidgetReason::Paint);
}

void SCommentLinkOverlay::SetLinkModeActive(bool bInActive)
{
    if (bLinkModeActive == bInActive)
    {
        return;
    }

    bLinkModeActive = bInActive;

    if (!bLinkModeActive)
    {
        // Cancel any in‑progress drag.
        bIsCreatingLink = false;
        PendingFromGuid.Invalidate();
        PendingMousePos = FVector2D::ZeroVector;

        // Release mouse capture if this widget owns it.
        if (HasMouseCapture())
        {
            FSlateApplication::Get().ReleaseMouseCapture();
        }

        // Clear selection and hit‑test caches.
        SelectedLinkGuid.Invalidate();
        LastRenderedLinks.Empty();
        LastRenderedHandles.Empty();
    }

    // When OFF we still draw but never participate in hit-testing.
    if (bLinkModeActive)
    {
        if (GraphPanelPtr)
        {
            // Ensure children have valid cached geometry before we rely on it.
            GraphPanelPtr->SlatePrepass();
        }
        SetVisibility(EVisibility::Visible);
    }
    else
    {
        SetVisibility(EVisibility::HitTestInvisible);
    }

    Invalidate(EInvalidateWidgetReason::Paint);

    UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: SetLinkModeActive -> %s"),
        bLinkModeActive ? TEXT("ON") : TEXT("OFF"));
}

void SCommentLinkOverlay::Tick(const FGeometry& AllottedGeometry,
                               const double InCurrentTime,
                               const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

int32 SCommentLinkOverlay::OnPaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    // Let SCompoundWidget do its normal painting first.
    const int32 BaseLayerId = SCompoundWidget::OnPaint(
        Args,
        AllottedGeometry,
        MyCullingRect,
        OutDrawElements,
        LayerId,
        InWidgetStyle,
        bParentEnabled);

    int32 CurrentLayer = BaseLayerId;

    SGraphPanel* GraphPanel = GraphPanelPtr;
    if (!GraphPanel)
    {
        return CurrentLayer;
    }

    if (bLinkModeActive)
    {
        UE_LOG(LogTemp, Log,
            TEXT("BlueprintCommentLinks: OnPaint link-mode; geom (%.1f, %.1f) zoom %.3f"),
            AllottedGeometry.GetLocalSize().X,
            AllottedGeometry.GetLocalSize().Y,
            GraphPanel->GetZoomAmount());
    }

    LastRenderedLinks.Reset();
    LastRenderedHandles.Reset();

    const bool bEnabled = ShouldBeEnabled(bParentEnabled);
    const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

    // DEBUG: faint blue tint to confirm overlay is painting while link mode is active.
    if (bLinkModeActive)
    {
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            CurrentLayer,
            AllottedGeometry.ToPaintGeometry(),
            FAppStyle::GetBrush("WhiteBrush"),
            DrawEffects,
            FLinearColor(0.f, 0.f, 1.f, 0.04f));
        ++CurrentLayer;
    }

    // Gather handle positions for all comment nodes in the graph and draw
    // compact pin‑like handles so we can confirm geometry is valid.
    UEdGraph* Graph = GraphPanel->GetGraphObj();
    FBlueprintCommentLinkGraphData* GraphData = nullptr;
    if (Graph)
    {
        GraphData = &UBlueprintCommentLinkCacheFile::Get().GetGraphData(Graph);
    }

    int32 TotalComments = 0;
    int32 DrawnHandles = 0;
    int32 ZeroHandles = 0;

    if (bLinkModeActive && Graph && GraphData)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(Node);
            if (!CommentNode)
            {
                continue;
            }

            ++TotalComments;

            const FVector2D HandleScreenPos = GetHandleScreenPosition(CommentNode->NodeGuid, ECommentHandleSide::Right);
            if (HandleScreenPos.IsNearlyZero())
            {
                UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: No handle geometry for comment %s"),
                    *CommentNode->GetName());
                ++ZeroHandles;
                continue;
            }

            ++DrawnHandles;

            // Treat handle coords as panel/overlay-local to avoid absolute transform drift.
            const FVector2D HandleLocalToOverlay = HandleScreenPos;

            FCommentHandleInfo Info;
            Info.CommentGuid    = CommentNode->NodeGuid;
            Info.LocalPosition  = HandleLocalToOverlay;
            Info.ScreenPosition = HandleScreenPos;
            LastRenderedHandles.Add(Info);

            // Style from per-comment handle color (defaults already set).
            const FBlueprintCommentLinkCommentStyle* Style =
                GraphData->FindStyle(CommentNode->NodeGuid);

            const FLinearColor HandleColor =
                Style ? Style->HandleColor
                      : FLinearColor(0.10f, 0.70f, 1.0f, 1.0f);

            // Simple Blueprint-style pin centered on the handle position.
            const float PinSize = 12.0f;
            const FVector2D PinSizeVec(PinSize, PinSize);
            const FVector2D PinTopLeft =
                HandleLocalToOverlay - (PinSizeVec * 0.5f);

            FSlateDrawElement::MakeBox(
                OutDrawElements,
                CurrentLayer,
                AllottedGeometry.ToPaintGeometry(PinTopLeft, PinSizeVec),
                FAppStyle::GetBrush("Graph.Pin.Connected"),
                DrawEffects,
                HandleColor);
            ++CurrentLayer;

            UE_LOG(LogTemp, Log,
                TEXT("BlueprintCommentLinks: Draw handle for comment %s at (%f,%f)"),
                *CommentNode->GetName(),
                HandleLocalToOverlay.X,
                HandleLocalToOverlay.Y);
        }

        UE_LOG(LogTemp, Log,
            TEXT("BlueprintCommentLinks: Handle summary total=%d drawn=%d zero=%d"),
            TotalComments, DrawnHandles, ZeroHandles);
    }

    // Draw existing links using handle positions as anchors.
    for (const FBlueprintCommentLink& Link : CachedLinks)
    {
        const FVector2D FromScreen = GetHandleScreenPosition(Link.SourceCommentGuid, ECommentHandleSide::Right);
        const FVector2D ToScreen   = GetHandleScreenPosition(Link.TargetCommentGuid, ECommentHandleSide::Left);

        if (FromScreen.IsNearlyZero() || ToScreen.IsNearlyZero())
        {
            continue;
        }

        const FVector2D FromLocal = FromScreen;
        const FVector2D ToLocal = ToScreen;

        if (bLinkModeActive)
        {
            LastRenderedLinks.Add({Link.LinkGuid, FromLocal, ToLocal});
        }

        // Resolve per-comment style for the source comment (wire color).
        FLinearColor BaseColor(0.1f, 0.7f, 1.0f, 1.0f);
        float Thickness = 1.5f;

        if (GraphData)
        {
            if (const FBlueprintCommentLinkCommentStyle* Style =
                    GraphData->FindStyle(Link.SourceCommentGuid))
            {
                BaseColor = Style->WireColor;
            }
        }

        // Allow explicit per-link override if set.
        if (!Link.LinkColor.Equals(FLinearColor::White))
        {
            BaseColor = Link.LinkColor;
        }

        const bool bIsSelected = (Link.LinkGuid == SelectedLinkGuid);

        float Alpha = bLinkModeActive ? 0.9f : 0.5f;

        if (bIsSelected)
        {
            Thickness *= 1.5f;
            Alpha     = 1.0f;
            BaseColor *= 1.25f;
        }

        const FLinearColor FinalColor(BaseColor.R, BaseColor.G, BaseColor.B, Alpha);

        const FVector2D Delta   = ToLocal - FromLocal;
        const FVector2D Tangent = FVector2D(Delta.X * 0.5f, 0.0f);

        FSlateDrawElement::MakeSpline(
            OutDrawElements,
            CurrentLayer,
            AllottedGeometry.ToPaintGeometry(),
            FromLocal,
            Tangent,
            ToLocal,
            -Tangent,
            Thickness,
            DrawEffects,
            FinalColor);
        ++CurrentLayer;

        // Direction arrow at the target end; size from per-comment style.
        const FVector2D Dir = Delta.GetSafeNormal();

        float ArrowSize = 7.0f;
        if (GraphData)
        {
            if (const FBlueprintCommentLinkCommentStyle* Style =
                    GraphData->FindStyle(Link.SourceCommentGuid))
            {
                ArrowSize = Style->ArrowSize;
            }
        }
        ArrowSize = FMath::Clamp(ArrowSize, 2.0f, 64.0f);

        const FVector2D Tip  = ToLocal - Dir * (ArrowSize * 0.8f);
        const FVector2D Perp(-Dir.Y, Dir.X);

        TArray<FVector2D> ArrowPoints;
        ArrowPoints.Add(Tip);
        ArrowPoints.Add(Tip - Dir * ArrowSize + Perp * (ArrowSize * 0.5f));
        ArrowPoints.Add(Tip - Dir * ArrowSize - Perp * (ArrowSize * 0.5f));
        ArrowPoints.Add(Tip);

        const FLinearColor ArrowColor = bIsSelected ? FinalColor : FinalColor * 0.9f;

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            CurrentLayer,
            AllottedGeometry.ToPaintGeometry(),
            ArrowPoints,
            DrawEffects,
            ArrowColor,
            true,
            Thickness);
        ++CurrentLayer;
    }

    // Draw preview line when creating a new link
    if (bIsCreatingLink && PendingFromGuid.IsValid() && bLinkModeActive)
    {
        const FVector2D FromScreen = GetHandleScreenPosition(PendingFromGuid, ECommentHandleSide::Right);
        if (!FromScreen.IsNearlyZero())
        {
            const FVector2D FromLocal = FromScreen;

            TArray<FVector2D> Points;
            Points.Add(FromLocal);
            Points.Add(PendingMousePos);

            const FLinearColor LineColor(0.6f, 0.9f, 0.3f, 0.9f);
            const float Thickness = 2.0f;

            FSlateDrawElement::MakeLines(
                OutDrawElements,
                CurrentLayer,
                AllottedGeometry.ToPaintGeometry(),
                Points,
                DrawEffects,
                LineColor,
                true,
                Thickness);
            ++CurrentLayer;
        }
    }

    return CurrentLayer;
}

FReply SCommentLinkOverlay::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (!bLinkModeActive)
    {
        UE_LOG(LogTemp, Verbose, TEXT("BlueprintCommentLinks: OnMouseButtonDown – link mode OFF, passing through"));
        return FReply::Unhandled();
    }

    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

    // Left-click: handles + link selection.
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        FGuid CommentGuid;
        if (HitTestHandle(LocalPos, CommentGuid))
        {
            bIsCreatingLink = true;
            PendingFromGuid = CommentGuid;
            PendingMousePos = LocalPos;

            UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: OnMouseButtonDown Left – start drag from %s"),
                *PendingFromGuid.ToString());

            return FReply::Handled().CaptureMouse(AsShared());
        }

        FGuid HitGuid;
        if (HitTestLink(LocalPos, HitGuid) && HitGuid.IsValid())
        {
            SelectedLinkGuid = HitGuid;
            Invalidate(EInvalidateWidgetReason::Paint);

            UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: OnMouseButtonDown Left – selected link %s"),
                *SelectedLinkGuid.ToString());

            return FReply::Handled();
        }

        UE_LOG(LogTemp, Verbose, TEXT("BlueprintCommentLinks: OnMouseButtonDown Left – no hit, passing through"));
    }
    // Right-click: context menu on links only.
    else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        FGuid HitGuid;
        if (HitTestLink(LocalPos, HitGuid) && HitGuid.IsValid())
        {
            SelectedLinkGuid = HitGuid;
            Invalidate(EInvalidateWidgetReason::Paint);

            UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: OnMouseButtonDown Right – open context menu for %s"),
                *SelectedLinkGuid.ToString());

            return OpenLinkContextMenu(MyGeometry, MouseEvent, HitGuid);
        }

        UE_LOG(LogTemp, Verbose, TEXT("BlueprintCommentLinks: OnMouseButtonDown Right – no hit, passing through"));
    }

    return FReply::Unhandled();
}

FReply SCommentLinkOverlay::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (!bLinkModeActive)
    {
        UE_LOG(LogTemp, Verbose, TEXT("BlueprintCommentLinks: OnMouseButtonUp – link mode OFF, passing through"));
        return FReply::Unhandled();
    }

    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
        bIsCreatingLink && HasMouseCapture())
    {
        const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();

        const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);

        FGuid TargetGuid;
        if (HitTestHandle(LocalPos, TargetGuid) && TargetGuid.IsValid() && TargetGuid != PendingFromGuid)
        {
            // Create a new link between PendingFromGuid and TargetGuid.
            if (SGraphPanel* GraphPanel = GraphPanelPtr)
            {
                if (UEdGraph* Graph = GraphPanel->GetGraphObj())
                {
                    FBlueprintCommentLink NewLink;
                    NewLink.SourceCommentGuid = PendingFromGuid;
                    NewLink.TargetCommentGuid = TargetGuid;

                    UBlueprintCommentLinkCacheFile::Get().AddOrUpdateLink(Graph, NewLink);

                    if (BlueprintPtr.IsValid())
                    {
                        if (UBlueprint* BP = BlueprintPtr.Get())
                        {
                            BP->Modify();
                        }
                    }

                    UE_LOG(LogTemp, Log,
                        TEXT("BlueprintCommentLinks: OnMouseButtonUp – created link %s from %s to %s"),
                        *NewLink.LinkGuid.ToString(),
                        *NewLink.SourceCommentGuid.ToString(),
                        *NewLink.TargetCommentGuid.ToString());
                }
            }

            RefreshCachedLinks();
        }

        UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: OnMouseButtonUp – drag end"));

        bIsCreatingLink = false;
        PendingFromGuid.Invalidate();
        PendingMousePos = FVector2D::ZeroVector;
        Invalidate(EInvalidateWidgetReason::Paint);

        return FReply::Handled().ReleaseMouseCapture();
    }

    return FReply::Unhandled();
}

FReply SCommentLinkOverlay::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (!bLinkModeActive || !bIsCreatingLink || !HasMouseCapture())
    {
        return FReply::Unhandled();
    }

    PendingMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    Invalidate(EInvalidateWidgetReason::Paint);

    UE_LOG(LogTemp, Verbose, TEXT("BlueprintCommentLinks: OnMouseMove drag – PendingMousePos=(%f,%f)"),
        PendingMousePos.X, PendingMousePos.Y);

    return FReply::Handled();
}

bool SCommentLinkOverlay::HitTestLink(const FVector2D& LocalMousePos, FGuid& OutLinkGuid) const
{
    OutLinkGuid.Invalidate();

    const float Threshold = 6.0f;

    for (const FRenderedLink& Link : LastRenderedLinks)
    {
        const FVector2D A = Link.From;
        const FVector2D B = Link.To;

        const FVector2D AB = B - A;
        const float ABLengthSq = AB.SizeSquared();
        if (ABLengthSq <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const float T = FMath::Clamp(FVector2D::DotProduct(LocalMousePos - A, AB) / ABLengthSq, 0.0f, 1.0f);
        const FVector2D ClosestPoint = A + AB * T;
        const float DistSq = (LocalMousePos - ClosestPoint).SizeSquared();

        if (DistSq <= Threshold * Threshold)
        {
            OutLinkGuid = Link.LinkGuid;
            return true;
        }
    }

    return false;
}

bool SCommentLinkOverlay::HitTestHandle(const FVector2D& ScreenPos, FGuid& OutCommentGuid) const
{
    OutCommentGuid.Invalidate();

    const float RadiusSquared = HandleHitRadius * HandleHitRadius;

    for (const FCommentHandleInfo& Info : LastRenderedHandles)
    {
        const float DistSqr = FVector2D::DistSquared(ScreenPos, Info.LocalPosition);
        if (DistSqr <= RadiusSquared)
        {
            OutCommentGuid = Info.CommentGuid;
            return true;
        }
    }

    return false;
}

bool SCommentLinkOverlay::TryGetHandleScreenPosition(const FGuid& CommentGuid, FVector2D& OutScreenPos) const
{
    for (const FCommentHandleInfo& Info : LastRenderedHandles)
    {
        if (Info.CommentGuid == CommentGuid)
        {
            OutScreenPos = Info.ScreenPosition;
            return true;
        }
    }

    return false;
}

bool SCommentLinkOverlay::GetCommentNodeUnderMouse(const FGeometry& MyGeometry,
                                                   const FPointerEvent& MouseEvent,
                                                   FGuid& OutNodeGuid) const
{
    OutNodeGuid.Invalidate();
    return false;
}

FVector2D SCommentLinkOverlay::GetHandleScreenPosition(const FGuid& CommentGuid, ECommentHandleSide Side) const
{
    if (!CommentGuid.IsValid())
    {
        return FVector2D::ZeroVector;
    }

    // Helper: compute handle position directly from graph-space data without a widget.
    const auto GraphSpacePosition = [this, Side](UEdGraphNode_Comment* CommentNode) -> FVector2D
    {
        if (!GraphPanelPtr || !CommentNode)
        {
            return FVector2D::ZeroVector;
        }

        const FVector2D GraphPos(CommentNode->NodePosX, CommentNode->NodePosY);
        FVector2D GraphSize(CommentNode->NodeWidth, CommentNode->NodeHeight);
        if (GraphSize.IsNearlyZero())
        {
            GraphSize = FVector2D(300.f, 150.f); // sensible default comment size
        }

        FVector2D LocalPos;
        switch (Side)
        {
        case ECommentHandleSide::Left:
            LocalPos = FVector2D(0.0f, GraphSize.Y * 0.5f);
            break;
        case ECommentHandleSide::Right:
            LocalPos = FVector2D(GraphSize.X, GraphSize.Y * 0.5f);
            break;
        case ECommentHandleSide::Top:
            LocalPos = FVector2D(GraphSize.X * 0.5f, 0.0f);
            break;
        case ECommentHandleSide::Bottom:
            LocalPos = FVector2D(GraphSize.X * 0.5f, GraphSize.Y);
            break;
        default:
            LocalPos = GraphSize * 0.5f;
            break;
        }

        const float Zoom = GraphPanelPtr->GetZoomAmount();
        const FVector2D ViewOffset = GraphPanelPtr->GetViewOffset();
        // Return panel-local coords; overlay shares the graph panel space.
        return (GraphPos + LocalPos - ViewOffset) * Zoom;
    };

    TSharedPtr<SGraphNode> CommentWidget =
        FBlueprintCommentLinksState::Get().GetCommentWidget(CommentGuid);

    // UE 5.7+ creates a new comment widget (SGraphNodeCommentWithMoveHandles) via
    // UEdGraphNode_Comment::CreateVisualWidget, which bypasses our node factory.
    // Fallback: scan the graph panel children for any comment widget that owns
    // this guid and register it so handle positions resolve.
    UEdGraphNode_Comment* ResolvedCommentNode = nullptr;

    if (!CommentWidget.IsValid())
    {
        if (SGraphPanel* GraphPanel = GraphPanelPtr)
        {
            const FChildren* Children = GraphPanel->GetChildren();
            if (Children)
            {
                for (int32 Index = 0; Index < Children->Num(); ++Index)
                {
                    const TSharedRef<const SWidget> ChildConst = Children->GetChildAt(Index);
                    const TSharedRef<SWidget> Child = ConstCastSharedRef<SWidget>(ChildConst);
                    const FName ChildType = Child->GetType();

                    const bool bIsCommentWidget =
                        ChildType == SBlueprintCommentLinksGraphNode::StaticWidgetClass().GetWidgetType() ||
                        ChildType == SGraphNodeComment::StaticWidgetClass().GetWidgetType() ||
                        ChildType == FName(TEXT("SGraphNodeCommentWithMoveHandles"));

                    if (!bIsCommentWidget)
                    {
                        continue;
                    }

                    TSharedPtr<SGraphNode> GraphNodeWidget = StaticCastSharedRef<SGraphNode>(Child);
                    if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(GraphNodeWidget->GetNodeObj()))
                    {
                        if (CommentNode->NodeGuid == CommentGuid)
                        {
                            FBlueprintCommentLinksState::Get().RegisterCommentWidget(CommentGuid, GraphNodeWidget);
                            CommentWidget = GraphNodeWidget;
                            ResolvedCommentNode = CommentNode;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (!CommentWidget.IsValid())
    {
        // Final fallback: compute purely from graph-space data so handles always draw.
        if (SGraphPanel* GraphPanel = GraphPanelPtr)
        {
            if (UEdGraph* Graph = GraphPanel->GetGraphObj())
            {
                for (UEdGraphNode* Node : Graph->Nodes)
                {
                    if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(Node))
                    {
                        if (CommentNode->NodeGuid == CommentGuid)
                        {
                            UE_LOG(LogTemp, Log,
                                TEXT("BlueprintCommentLinks: HandlePos fallback to graph space for %s (widget missing)"),
                                *CommentGuid.ToString());
                            return GraphSpacePosition(CommentNode);
                        }
                    }
                }
            }
        }

        return FVector2D::ZeroVector;
    }

    const FGeometry& NodeGeometry = CommentWidget->GetCachedGeometry();
    const FVector2D NodeSize = NodeGeometry.GetLocalSize();

    if (NodeSize.IsNearlyZero())
    {
        UE_LOG(LogTemp, Log,
            TEXT("BlueprintCommentLinks: Comment widget %s has zero geometry; using graph-space fallback"),
            *CommentGuid.ToString());
    }

    // If we still have a tiny/zero size (can happen with new comment widgets before first pre-pass),
    // fall back to graph coordinates from the comment node itself.
    if (NodeSize.IsNearlyZero() && GraphPanelPtr)
    {
        if (!ResolvedCommentNode)
        {
            ResolvedCommentNode = Cast<UEdGraphNode_Comment>(CommentWidget->GetNodeObj());
        }

        if (ResolvedCommentNode)
        {
            const FVector2D GraphPos(ResolvedCommentNode->NodePosX, ResolvedCommentNode->NodePosY);
            FVector2D GraphSize(ResolvedCommentNode->NodeWidth, ResolvedCommentNode->NodeHeight);
            if (GraphSize.IsNearlyZero())
            {
                GraphSize = FVector2D(300.f, 150.f); // reasonable default for comments
            }

            UE_LOG(LogTemp, Log,
                TEXT("BlueprintCommentLinks: Using graph-space fallback size (%f,%f) for comment %s"),
                GraphSize.X, GraphSize.Y, *CommentGuid.ToString());

            FVector2D LocalPos;
            switch (Side)
            {
            case ECommentHandleSide::Left:
                LocalPos = FVector2D(0.0f, GraphSize.Y * 0.5f);
                break;
            case ECommentHandleSide::Right:
                LocalPos = FVector2D(GraphSize.X, GraphSize.Y * 0.5f);
                break;
            case ECommentHandleSide::Top:
                LocalPos = FVector2D(GraphSize.X * 0.5f, 0.0f);
                break;
            case ECommentHandleSide::Bottom:
                LocalPos = FVector2D(GraphSize.X * 0.5f, GraphSize.Y);
                break;
            default:
                LocalPos = GraphSize * 0.5f;
                break;
            }

            const float Zoom = GraphPanelPtr->GetZoomAmount();
            const FVector2D ViewOffset = GraphPanelPtr->GetViewOffset();
            return (GraphPos + LocalPos - ViewOffset) * Zoom;
        }
    }

    FVector2D LocalPos;
    switch (Side)
    {
    case ECommentHandleSide::Left:
        LocalPos = FVector2D(0.0f, NodeSize.Y * 0.5f);
        break;
    case ECommentHandleSide::Right:
        LocalPos = FVector2D(NodeSize.X, NodeSize.Y * 0.5f);
        break;
    case ECommentHandleSide::Top:
        LocalPos = FVector2D(NodeSize.X * 0.5f, 0.0f);
        break;
    case ECommentHandleSide::Bottom:
        LocalPos = FVector2D(NodeSize.X * 0.5f, NodeSize.Y);
        break;
    default:
        LocalPos = NodeSize * 0.5f;
        break;
    }

    // Convert node-local to panel-local so overlay draws in the same space.
    const FVector2D NodeAbsolute = NodeGeometry.LocalToAbsolute(LocalPos);
    return GraphPanelPtr->GetCachedGeometry().AbsoluteToLocal(NodeAbsolute);
}

FReply SCommentLinkOverlay::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (!bLinkModeActive || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return FReply::Unhandled();
    }

    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

    FGuid HitGuid;
    if (HitTestLink(LocalPos, HitGuid) && HitGuid.IsValid())
    {
        SelectedLinkGuid = HitGuid;
        JumpToLinkTarget(HitGuid);

        UE_LOG(LogTemp, Log, TEXT("BlueprintCommentLinks: OnMouseButtonDoubleClick – jump to link %s"),
            *HitGuid.ToString());

        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply SCommentLinkOverlay::OpenLinkContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, const FGuid& LinkGuid)
{
    FMenuBuilder MenuBuilder(true, nullptr);

    MenuBuilder.AddMenuEntry(
        NSLOCTEXT("BlueprintCommentLinks", "FocusSource", "Focus Source Comment"),
        NSLOCTEXT("BlueprintCommentLinks", "FocusSource_Tooltip", "Jump to the source comment node."),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateSP(this, &SCommentLinkOverlay::FocusLinkSource)));

    MenuBuilder.AddMenuEntry(
        NSLOCTEXT("BlueprintCommentLinks", "FocusTarget", "Focus Target Comment"),
        NSLOCTEXT("BlueprintCommentLinks", "FocusTarget_Tooltip", "Jump to the target comment node."),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateSP(this, &SCommentLinkOverlay::FocusLinkTarget)));

    MenuBuilder.AddMenuSeparator();

    MenuBuilder.AddMenuEntry(
        NSLOCTEXT("BlueprintCommentLinks", "EditTag", "Edit Tag..."),
        NSLOCTEXT("BlueprintCommentLinks", "EditTag_Tooltip", "Edit the tag associated with this link."),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([this, LinkGuid]()
        {
            EditLinkTag(LinkGuid);
        })));

    MenuBuilder.AddMenuEntry(
        NSLOCTEXT("BlueprintCommentLinks", "DeleteLink", "Delete Link"),
        NSLOCTEXT("BlueprintCommentLinks", "DeleteLink_Tooltip", "Remove this comment link."),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateSP(this, &SCommentLinkOverlay::DeleteLink)));

    const TSharedRef<SWidget> MenuWidget = MenuBuilder.MakeWidget();

    FSlateApplication::Get().PushMenu(
        AsShared(),
        FWidgetPath(),
        MenuWidget,
        MouseEvent.GetScreenSpacePosition(),
        FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

    return FReply::Handled();
}

void SCommentLinkOverlay::JumpToLinkTarget(const FGuid& LinkGuid)
{
    if (!GraphPanelPtr || !LinkGuid.IsValid())
    {
        return;
    }

    UEdGraph* Graph = GraphPanelPtr->GetGraphObj();
    if (!Graph)
    {
        return;
    }

    UBlueprintCommentLinkCacheFile& Cache = UBlueprintCommentLinkCacheFile::Get();
    FBlueprintCommentLinkGraphData& Data   = Cache.GetGraphData(Graph);

    const FBlueprintCommentLink* LinkPtr = Data.Links.FindByPredicate(
        [&LinkGuid](const FBlueprintCommentLink& L) { return L.LinkGuid == LinkGuid; });

    if (!LinkPtr)
    {
        return;
    }

    const FGuid TargetGuid = LinkPtr->TargetCommentGuid;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node && Node->NodeGuid == TargetGuid)
        {
            GraphPanelPtr->JumpToNode(Node, false, true);
            break;
        }
    }
}

void SCommentLinkOverlay::FocusLinkSource()
{
    if (!GraphPanelPtr || !SelectedLinkGuid.IsValid())
    {
        return;
    }

    UEdGraph* Graph = GraphPanelPtr->GetGraphObj();
    if (!Graph)
    {
        return;
    }

    UBlueprintCommentLinkCacheFile& Cache = UBlueprintCommentLinkCacheFile::Get();
    FBlueprintCommentLinkGraphData& Data   = Cache.GetGraphData(Graph);

    const FBlueprintCommentLink* LinkPtr = Data.Links.FindByPredicate(
        [this](const FBlueprintCommentLink& L) { return L.LinkGuid == SelectedLinkGuid; });

    if (!LinkPtr)
    {
        return;
    }

    const FGuid SourceGuid = LinkPtr->SourceCommentGuid;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node && Node->NodeGuid == SourceGuid)
        {
            GraphPanelPtr->JumpToNode(Node, false, true);
            break;
        }
    }
}

void SCommentLinkOverlay::FocusLinkTarget()
{
    if (SelectedLinkGuid.IsValid())
    {
        JumpToLinkTarget(SelectedLinkGuid);
    }
}

void SCommentLinkOverlay::DeleteLink()
{
    if (!GraphPanelPtr || !SelectedLinkGuid.IsValid())
    {
        return;
    }

    UEdGraph* Graph = GraphPanelPtr->GetGraphObj();
    if (!Graph)
    {
        return;
    }

    UBlueprintCommentLinkCacheFile& Cache = UBlueprintCommentLinkCacheFile::Get();
    Cache.RemoveLinkByGuid(Graph, SelectedLinkGuid);

    if (BlueprintPtr.IsValid())
    {
        if (UBlueprint* BP = BlueprintPtr.Get())
        {
            BP->Modify();
        }
    }

    RefreshCachedLinks();
}

void SCommentLinkOverlay::EditLinkTag(const FGuid& LinkGuid)
{
    // Basic sanity checks
    if (!GraphPanelPtr || !LinkGuid.IsValid())
    {
        return;
    }

    UEdGraph* Graph = GraphPanelPtr->GetGraphObj();
    if (!Graph)
    {
        return;
    }

    // Look up the current link so we can pre-fill the tag text
    UBlueprintCommentLinkCacheFile& Cache = UBlueprintCommentLinkCacheFile::Get();
    FBlueprintCommentLinkGraphData& Data  = Cache.GetGraphData(Graph);

    FBlueprintCommentLink* LinkPtr = Data.Links.FindByPredicate(
        [&LinkGuid](const FBlueprintCommentLink& L)
        {
            return L.LinkGuid == LinkGuid;
        });

    if (!LinkPtr)
    {
        return;
    }

    const FString CurrentTagString = LinkPtr->LinkTag.IsNone()
        ? FString()
        : LinkPtr->LinkTag.ToString();

    // Weak references to the graph & owning blueprint so the dialog is safe if those go away
    TWeakObjectPtr<UEdGraph>   GraphWeak     = Graph;
    TWeakObjectPtr<UBlueprint> BlueprintWeak = BlueprintPtr;
    const FGuid CapturedLinkGuid = LinkGuid;

    // Build the text box first so we can reference it in the window content
    TSharedRef<SEditableTextBox> TextBox =
        SNew(SEditableTextBox)
        .Text(FText::FromString(CurrentTagString));

    // Create the window itself
    TSharedRef<SWindow> Window =
        SNew(SWindow)
        .Title(NSLOCTEXT("BlueprintCommentLinks", "EditLinkTagTitle", "Edit Link Tag"))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        .ClientSize(FVector2D(320.f, 90.f));

    // Weak pointers to Slate widgets so lambdas don’t crash if they’re destroyed
    TWeakPtr<SWindow>          WindowWeak = Window;
    TWeakPtr<SEditableTextBox> TextWeak   = TextBox;

    // Now set the window content
    Window->SetContent(
        SNew(SVerticalBox)

        + SVerticalBox::Slot()
        .Padding(4.f)
        .AutoHeight()
        [
            TextBox
        ]

        + SVerticalBox::Slot()
        .Padding(4.f)
        .HAlign(HAlign_Right)
        .AutoHeight()
        [
            SNew(SHorizontalBox)

            // OK button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2.f)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("BlueprintCommentLinks", "OK", "OK"))
                .OnClicked_Lambda([WindowWeak, TextWeak, GraphWeak, BlueprintWeak, CapturedLinkGuid]()
                {
                    TSharedPtr<SWindow> WindowPinned = WindowWeak.Pin();
                    TSharedPtr<SEditableTextBox> TextPinned = TextWeak.Pin();

                    // If the window or text box is gone, just bail
                    if (!WindowPinned.IsValid() || !TextPinned.IsValid())
                    {
                        return FReply::Handled();
                    }

                    UEdGraph* LocalGraph = GraphWeak.Get();
                    if (!LocalGraph)
                    {
                        FSlateApplication::Get().RequestDestroyWindow(WindowPinned.ToSharedRef());
                        return FReply::Handled();
                    }

                    // Re-query the cache and link *by GUID* at click time
                    UBlueprintCommentLinkCacheFile& LocalCache = UBlueprintCommentLinkCacheFile::Get();
                    FBlueprintCommentLinkGraphData& LocalData  = LocalCache.GetGraphData(LocalGraph);

                    FBlueprintCommentLink* LocalLink = LocalData.Links.FindByPredicate(
                        [CapturedLinkGuid](const FBlueprintCommentLink& L)
                        {
                            return L.LinkGuid == CapturedLinkGuid;
                        });

                    if (LocalLink)
                    {
                        const FString NewTagStr = TextPinned->GetText().ToString();
                        LocalLink->LinkTag = NewTagStr.IsEmpty() ? NAME_None : FName(*NewTagStr);

                        // Persist back to metadata
                        LocalCache.UpdateGraphLinks(LocalGraph);

                        if (UBlueprint* BP = BlueprintWeak.Get())
                        {
                            BP->Modify();
                        }
                    }

                    // Same as clicking the X: just close the window
                    FSlateApplication::Get().RequestDestroyWindow(WindowPinned.ToSharedRef());
                    return FReply::Handled();
                })
            ]

            // Cancel button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2.f)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("BlueprintCommentLinks", "Cancel", "Cancel"))
                .OnClicked_Lambda([WindowWeak]()
                {
                    TSharedPtr<SWindow> WindowPinned = WindowWeak.Pin();
                    if (WindowPinned.IsValid())
                    {
                        // EXACTLY same behavior as clicking the X – just close, no edits, no cache writes
                        FSlateApplication::Get().RequestDestroyWindow(WindowPinned.ToSharedRef());
                    }
                    return FReply::Handled();
                })
            ]
        ]
    );

    // Show the window
    FSlateApplication::Get().AddWindow(Window);
}
