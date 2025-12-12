#include "BlueprintCommentLinksTypes.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphNode_Comment.h"
#include "Misc/Base64.h"
#include "UObject/Package.h"
#include "UObject/MetaData.h"

namespace BlueprintCommentLinksInternal
{
    static const TCHAR* MetaDataKey       = TEXT("BlueprintCommentLinks");
    static const TCHAR* StylesMetaDataKey = TEXT("BlueprintCommentLinksStyles");

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7)
    using FBlueprintCommentLinksMetaData = FMetaData;

    static FBlueprintCommentLinksMetaData* GetPackageMetaData(UPackage* Package)
    {
        return Package ? &Package->GetMetaData() : nullptr;
    }
#else
    using FBlueprintCommentLinksMetaData = UMetaData;

    static FBlueprintCommentLinksMetaData* GetPackageMetaData(UPackage* Package)
    {
        return Package ? Package->GetMetaData() : nullptr;
    }
#endif
}

TArray<FBlueprintCommentLink> FBlueprintCommentLinkGraphData::GetLinksForComment(const FGuid& CommentGuid) const
{
    TArray<FBlueprintCommentLink> Result;
    for (const FBlueprintCommentLink& Link : Links)
    {
        if (Link.SourceCommentGuid == CommentGuid || Link.TargetCommentGuid == CommentGuid)
        {
            Result.Add(Link);
        }
    }
    return Result;
}

void FBlueprintCommentLinkGraphData::AddOrUpdateLink(const FBlueprintCommentLink& Link)
{
    // Ensure the link has a valid identifier.
    FBlueprintCommentLink Copy = Link;
    if (!Copy.LinkGuid.IsValid())
    {
        Copy.LinkGuid = FGuid::NewGuid();
    }

    for (FBlueprintCommentLink& Existing : Links)
    {
        if (Existing.LinkGuid == Copy.LinkGuid)
        {
            Existing = Copy;
            return;
        }
    }

    Links.Add(Copy);
}

void FBlueprintCommentLinkGraphData::RemoveLinksForComment(const FGuid& CommentGuid)
{
    Links.RemoveAll(
        [&CommentGuid](const FBlueprintCommentLink& Link)
        {
            return Link.SourceCommentGuid == CommentGuid || Link.TargetCommentGuid == CommentGuid;
        });
}

void FBlueprintCommentLinkGraphData::RemoveLinkByGuid(const FGuid& LinkGuid)
{
    if (!LinkGuid.IsValid())
    {
        return;
    }

    Links.RemoveAll(
        [&LinkGuid](const FBlueprintCommentLink& Link)
        {
            return Link.LinkGuid == LinkGuid;
        });
}

FBlueprintCommentLinkCommentStyle& FBlueprintCommentLinkGraphData::GetOrCreateStyle(const FGuid& CommentGuid)
{
    static FBlueprintCommentLinkCommentStyle DummyStyle;

    if (!CommentGuid.IsValid())
    {
        return DummyStyle;
    }

    for (FBlueprintCommentLinkCommentStyle& Style : CommentStyles)
    {
        if (Style.CommentGuid == CommentGuid)
        {
            return Style;
        }
    }

    FBlueprintCommentLinkCommentStyle NewStyle;
    NewStyle.CommentGuid = CommentGuid;
    CommentStyles.Add(NewStyle);
    return CommentStyles.Last();
}

const FBlueprintCommentLinkCommentStyle* FBlueprintCommentLinkGraphData::FindStyle(const FGuid& CommentGuid) const
{
    if (!CommentGuid.IsValid())
    {
        return nullptr;
    }

    return CommentStyles.FindByPredicate(
        [&CommentGuid](const FBlueprintCommentLinkCommentStyle& Style)
        {
            return Style.CommentGuid == CommentGuid;
        });
}

bool FBlueprintCommentLinkGraphData::LoadFromPackageMetaData(UEdGraph* Graph)
{
    if (!Graph)
    {
        return false;
    }

    UPackage* Package = Graph->GetOutermost();
    if (!Package)
    {
        return false;
    }

    BlueprintCommentLinksInternal::FBlueprintCommentLinksMetaData* MetaData =
        BlueprintCommentLinksInternal::GetPackageMetaData(Package);
    if (!MetaData)
    {
        return false;
    }

    // Load link data.
    const FString Stored = MetaData->GetValue(Graph, BlueprintCommentLinksInternal::MetaDataKey);
    if (!Stored.IsEmpty())
    {
        TArray<FString> Lines;
        Stored.ParseIntoArrayLines(Lines, /*CullEmpty=*/true);

        Links.Reset();

        for (const FString& Line : Lines)
        {
            TArray<FString> Parts;
            Line.ParseIntoArray(Parts, TEXT(";"), /*CullEmpty=*/false);
            if (Parts.Num() < 6)
            {
                continue;
            }

            FBlueprintCommentLink Link;

            FGuid::Parse(Parts[0], Link.LinkGuid);
            FGuid::Parse(Parts[1], Link.SourceCommentGuid);
            FGuid::Parse(Parts[2], Link.TargetCommentGuid);

            Link.LinkTag = Parts[3].IsEmpty() ? NAME_None : FName(*Parts[3]);

            // Color: "R,G,B,A"
            {
                TArray<FString> ColorParts;
                Parts[4].ParseIntoArray(ColorParts, TEXT(","), /*CullEmpty=*/false);
                if (ColorParts.Num() == 4)
                {
                    Link.LinkColor.R = FCString::Atof(*ColorParts[0]);
                    Link.LinkColor.G = FCString::Atof(*ColorParts[1]);
                    Link.LinkColor.B = FCString::Atof(*ColorParts[2]);
                    Link.LinkColor.A = FCString::Atof(*ColorParts[3]);
                }
                else
                {
                    Link.LinkColor = FLinearColor::White;
                }
            }

            Link.bIsDirectional = Parts[5].Equals(TEXT("1")) || Parts[5].Equals(TEXT("true"), ESearchCase::IgnoreCase);

            Links.Add(Link);
        }
    }

    // Load per-comment styles (optional; uses a separate key).
    const FString StylesStored = MetaData->GetValue(Graph, BlueprintCommentLinksInternal::StylesMetaDataKey);
    if (!StylesStored.IsEmpty())
    {
        CommentStyles.Reset();

        TArray<FString> StyleLines;
        StylesStored.ParseIntoArrayLines(StyleLines, /*CullEmpty=*/true);

        for (const FString& Line : StyleLines)
        {
            TArray<FString> StyleParts;
            Line.ParseIntoArray(StyleParts, TEXT(";"), /*CullEmpty=*/false);
            if (StyleParts.Num() < 3)
            {
                continue;
            }

            const FString& GuidStr   = StyleParts[0];
            const FString& HandleStr = StyleParts[1];
            const FString& WireStr   = StyleParts[2];
            const FString  ArrowStr  = StyleParts.Num() >= 4 ? StyleParts[3] : FString();

            FBlueprintCommentLinkCommentStyle Style;
            FGuid::Parse(GuidStr, Style.CommentGuid);

            // HandleColor: "R,G,B,A"
            {
                TArray<FString> Parts;
                HandleStr.ParseIntoArray(Parts, TEXT(","), /*CullEmpty=*/false);
                if (Parts.Num() == 4)
                {
                    Style.HandleColor = FLinearColor(
                        FCString::Atof(*Parts[0]),
                        FCString::Atof(*Parts[1]),
                        FCString::Atof(*Parts[2]),
                        FCString::Atof(*Parts[3]));
                }
            }

            // WireColor: "R,G,B,A"
            {
                TArray<FString> Parts;
                WireStr.ParseIntoArray(Parts, TEXT(","), /*CullEmpty=*/false);
                if (Parts.Num() == 4)
                {
                    Style.WireColor = FLinearColor(
                        FCString::Atof(*Parts[0]),
                        FCString::Atof(*Parts[1]),
                        FCString::Atof(*Parts[2]),
                        FCString::Atof(*Parts[3]));
                }
            }

            // Arrow size (optional, defaults to 7.0 if absent).
            Style.ArrowSize = 7.0f;
            if (!ArrowStr.IsEmpty())
            {
                Style.ArrowSize = FCString::Atof(*ArrowStr);
            }

            if (Style.CommentGuid.IsValid())
            {
                CommentStyles.Add(Style);
            }
        }
    }

    bInitialized = true;
    return Links.Num() > 0 || CommentStyles.Num() > 0;
}

void FBlueprintCommentLinkGraphData::SaveToPackageMetaData(UEdGraph* Graph)
{
    if (!Graph)
    {
        return;
    }

    UPackage* Package = Graph->GetOutermost();
    if (!Package)
    {
        return;
    }

    BlueprintCommentLinksInternal::FBlueprintCommentLinksMetaData* MetaData =
        BlueprintCommentLinksInternal::GetPackageMetaData(Package);
    if (!MetaData)
    {
        return;
    }

    FString Combined;
    for (const FBlueprintCommentLink& Link : Links)
    {
        const FString GuidStr = Link.LinkGuid.ToString(EGuidFormats::DigitsWithHyphens);
        const FString SourceStr = Link.SourceCommentGuid.ToString(EGuidFormats::DigitsWithHyphens);
        const FString TargetStr = Link.TargetCommentGuid.ToString(EGuidFormats::DigitsWithHyphens);

        const FString TagStr = Link.LinkTag.IsNone() ? FString() : Link.LinkTag.ToString();

        const FString ColorStr = FString::Printf(
            TEXT("%0.9g,%0.9g,%0.9g,%0.9g"),
            Link.LinkColor.R,
            Link.LinkColor.G,
            Link.LinkColor.B,
            Link.LinkColor.A);

        const FString DirectionStr = Link.bIsDirectional ? TEXT("1") : TEXT("0");

        Combined += FString::Printf(
            TEXT("%s;%s;%s;%s;%s;%s\n"),
            *GuidStr,
            *SourceStr,
            *TargetStr,
            *TagStr,
            *ColorStr,
            *DirectionStr);
    }

    MetaData->SetValue(Graph, BlueprintCommentLinksInternal::MetaDataKey, *Combined);

    // Save per-comment styles under a separate metadata key.
    FString StylesCombined;
    for (const FBlueprintCommentLinkCommentStyle& Style : CommentStyles)
    {
        if (!Style.CommentGuid.IsValid())
        {
            continue;
        }

        if (!StylesCombined.IsEmpty())
        {
            StylesCombined += LINE_TERMINATOR;
        }

        StylesCombined += FString::Printf(
            TEXT("%s;%0.9g,%0.9g,%0.9g,%0.9g;%0.9g,%0.9g,%0.9g,%0.9g;%0.9g"),
            *Style.CommentGuid.ToString(EGuidFormats::DigitsWithHyphens),
            Style.HandleColor.R, Style.HandleColor.G, Style.HandleColor.B, Style.HandleColor.A,
            Style.WireColor.R,   Style.WireColor.G,   Style.WireColor.B,   Style.WireColor.A,
            Style.ArrowSize);
    }

    MetaData->SetValue(Graph, BlueprintCommentLinksInternal::StylesMetaDataKey, *StylesCombined);
}

void FBlueprintCommentLinkGraphData::CleanupGraph(UEdGraph* Graph)
{
    if (!Graph)
    {
        return;
    }

    TSet<FGuid> ValidCommentGuids;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(Node))
        {
            ValidCommentGuids.Add(CommentNode->NodeGuid);
        }
    }

    Links.RemoveAll(
        [&ValidCommentGuids](const FBlueprintCommentLink& Link)
        {
            return !ValidCommentGuids.Contains(Link.SourceCommentGuid) ||
                   !ValidCommentGuids.Contains(Link.TargetCommentGuid);
        });

    // Remove any styles that reference comments that no longer exist.
    CommentStyles.RemoveAll(
        [&ValidCommentGuids](const FBlueprintCommentLinkCommentStyle& Style)
        {
            return !ValidCommentGuids.Contains(Style.CommentGuid);
        });
}
