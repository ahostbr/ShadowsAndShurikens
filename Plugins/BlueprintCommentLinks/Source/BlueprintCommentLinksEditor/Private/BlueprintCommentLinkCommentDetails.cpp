#include "BlueprintCommentLinkCommentDetails.h"

#include "BlueprintCommentLinksCacheFile.h"
#include "BlueprintCommentLinksTypes.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FBlueprintCommentLinkCommentDetails::MakeInstance()
{
    return MakeShareable(new FBlueprintCommentLinkCommentDetails());
}

void FBlueprintCommentLinkCommentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> Objects;
    DetailBuilder.GetObjectsBeingCustomized(Objects);

    UEdGraphNode_Comment* CommentNode = nullptr;
    for (TWeakObjectPtr<UObject> Obj : Objects)
    {
        if (UEdGraphNode_Comment* AsComment = Cast<UEdGraphNode_Comment>(Obj.Get()))
        {
            CommentNode = AsComment;
            break;
        }
    }

    if (!CommentNode)
    {
        return;
    }

    BuildCommentLinksCategory(DetailBuilder, CommentNode);
}

void FBlueprintCommentLinkCommentDetails::BuildCommentLinksCategory(
    IDetailLayoutBuilder& DetailBuilder,
    UEdGraphNode_Comment* CommentNode)
{
    IDetailCategoryBuilder& Category =
        DetailBuilder.EditCategory("Comment Links");

    UEdGraph* Graph = CommentNode->GetGraph();
    if (!Graph)
    {
        return;
    }

    UBlueprintCommentLinkCacheFile& Cache = UBlueprintCommentLinkCacheFile::Get();
    FBlueprintCommentLinkGraphData& Data  = Cache.GetGraphData(Graph);
    const FGuid CommentGuid = CommentNode->NodeGuid;
    FBlueprintCommentLinkCommentStyle& Style = Data.GetOrCreateStyle(CommentGuid);

    // Read-only summary of link tags touching this comment.
    {
        TSet<FName> TagSet;
        for (const FBlueprintCommentLink& Link : Data.Links)
        {
            const bool bTouchesComment =
                (Link.SourceCommentGuid == CommentGuid) ||
                (Link.TargetCommentGuid == CommentGuid);

            if (bTouchesComment && !Link.LinkTag.IsNone())
            {
                TagSet.Add(Link.LinkTag);
            }
        }

        FText TagsText;
        if (TagSet.Num() == 0)
        {
            TagsText = NSLOCTEXT("BlueprintCommentLinks", "LinkTags_None", "None");
        }
        else
        {
            TArray<FName> TagArray = TagSet.Array();
            TagArray.Sort(FNameLexicalLess());

            FString Combined;
            for (int32 Index = 0; Index < TagArray.Num(); ++Index)
            {
                if (!Combined.IsEmpty())
                {
                    Combined += TEXT(", ");
                }

                Combined += TagArray[Index].ToString();
            }

            TagsText = FText::FromString(Combined);
        }

        Category.AddCustomRow(NSLOCTEXT("BlueprintCommentLinks", "LinkTagsFilter", "Link Tags"))
        .NameContent()
        [
            SNew(STextBlock)
            .Text(NSLOCTEXT("BlueprintCommentLinks", "LinkTagsLabel", "Link Tags"))
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        .ValueContent()
        .MinDesiredWidth(200.f)
        [
            SNew(STextBlock)
            .Text(TagsText)
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ];
    }

    // Handle color row
    Category.AddCustomRow(NSLOCTEXT("BlueprintCommentLinks", "HandleColorFilter", "Handle Color"))
    .NameContent()
    [
        SNew(STextBlock)
        .Text(NSLOCTEXT("BlueprintCommentLinks", "HandleColor", "Handle Color"))
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ]
    .ValueContent()
    .MinDesiredWidth(200.f)
    [
        SNew(SColorBlock)
        .Color(Style.HandleColor)
        .ShowBackgroundForAlpha(true)
        .AlphaDisplayMode(EColorBlockAlphaDisplayMode::Combined)
        .OnMouseButtonDown_Lambda(
            [Graph, CommentGuid](const FGeometry&, const FPointerEvent&) -> FReply
            {
                UBlueprintCommentLinkCacheFile& LocalCache = UBlueprintCommentLinkCacheFile::Get();
                FBlueprintCommentLinkGraphData& LocalData  = LocalCache.GetGraphData(Graph);
                FBlueprintCommentLinkCommentStyle& LocalStyle = LocalData.GetOrCreateStyle(CommentGuid);

                FColorPickerArgs Args;
                Args.InitialColor = LocalStyle.HandleColor;
                Args.bUseAlpha = true;
                Args.OnColorCommitted = FOnLinearColorValueChanged::CreateLambda(
                    [Graph, CommentGuid](FLinearColor NewColor)
                    {
                        UBlueprintCommentLinkCacheFile& CacheRef = UBlueprintCommentLinkCacheFile::Get();
                        FBlueprintCommentLinkGraphData& DataRef  = CacheRef.GetGraphData(Graph);
                        FBlueprintCommentLinkCommentStyle& StyleRef = DataRef.GetOrCreateStyle(CommentGuid);
                        StyleRef.HandleColor = NewColor;
                        CacheRef.UpdateGraphLinks(Graph);
                    });

                OpenColorPicker(Args);
                return FReply::Handled();
            })
    ];

    // Wire color row
    Category.AddCustomRow(NSLOCTEXT("BlueprintCommentLinks", "WireColorFilter", "Wire Color"))
    .NameContent()
    [
        SNew(STextBlock)
        .Text(NSLOCTEXT("BlueprintCommentLinks", "WireColor", "Wire Color"))
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ]
    .ValueContent()
    .MinDesiredWidth(200.f)
    [
        SNew(SColorBlock)
        .Color(Style.WireColor)
        .ShowBackgroundForAlpha(true)
        .AlphaDisplayMode(EColorBlockAlphaDisplayMode::Combined)
        .OnMouseButtonDown_Lambda(
            [Graph, CommentGuid](const FGeometry&, const FPointerEvent&) -> FReply
            {
                UBlueprintCommentLinkCacheFile& LocalCache = UBlueprintCommentLinkCacheFile::Get();
                FBlueprintCommentLinkGraphData& LocalData  = LocalCache.GetGraphData(Graph);
                FBlueprintCommentLinkCommentStyle& LocalStyle = LocalData.GetOrCreateStyle(CommentGuid);

                FColorPickerArgs Args;
                Args.InitialColor = LocalStyle.WireColor;
                Args.bUseAlpha = true;
                Args.OnColorCommitted = FOnLinearColorValueChanged::CreateLambda(
                    [Graph, CommentGuid](FLinearColor NewColor)
                    {
                        UBlueprintCommentLinkCacheFile& CacheRef = UBlueprintCommentLinkCacheFile::Get();
                        FBlueprintCommentLinkGraphData& DataRef  = CacheRef.GetGraphData(Graph);
                        FBlueprintCommentLinkCommentStyle& StyleRef = DataRef.GetOrCreateStyle(CommentGuid);
                        StyleRef.WireColor = NewColor;
                        CacheRef.UpdateGraphLinks(Graph);
                    });

                OpenColorPicker(Args);
                return FReply::Handled();
            })
    ];

    // Arrow size row
    Category.AddCustomRow(NSLOCTEXT("BlueprintCommentLinks", "ArrowSizeFilter", "Arrow Size"))
    .NameContent()
    [
        SNew(STextBlock)
        .Text(NSLOCTEXT("BlueprintCommentLinks", "ArrowSize", "Arrow Size"))
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ]
    .ValueContent()
    .MinDesiredWidth(200.f)
    [
        SNew(SSpinBox<float>)
        .MinValue(2.0f)
        .MaxValue(64.0f)
        .Value(Style.ArrowSize)
        .OnValueChanged_Lambda([Graph, CommentGuid](float NewValue)
        {
            UBlueprintCommentLinkCacheFile& CacheRef = UBlueprintCommentLinkCacheFile::Get();
            FBlueprintCommentLinkGraphData& DataRef  = CacheRef.GetGraphData(Graph);
            FBlueprintCommentLinkCommentStyle& StyleRef =
                DataRef.GetOrCreateStyle(CommentGuid);

            StyleRef.ArrowSize = NewValue;
            CacheRef.UpdateGraphLinks(Graph);
        })
    ];
}
