#pragma once

#include "CoreMinimal.h"
#include "BlueprintCommentLinksTypes.generated.h"

class UEdGraph;

/**
 * Represents a single visual link between two Blueprint comment boxes.
 */
USTRUCT()
struct BLUEPRINTCOMMENTLINKSEDITOR_API FBlueprintCommentLink
{
    GENERATED_BODY()

public:
    /** Unique identifier for this link. */
    UPROPERTY()
    FGuid LinkGuid = FGuid();

    /** Source comment node (UEdGraphNode_Comment::NodeGuid). */
    UPROPERTY()
    FGuid SourceCommentGuid;

    /** Target comment node (UEdGraphNode_Comment::NodeGuid). */
    UPROPERTY()
    FGuid TargetCommentGuid;

    /** Optional label/tag for this link. */
    UPROPERTY()
    FName LinkTag = NAME_None;

    /** Optional color override for the link wire. */
    UPROPERTY()
    FLinearColor LinkColor = FLinearColor::White;

    /** Whether this link is directional (Source -> Target). */
    UPROPERTY()
    bool bIsDirectional = true;

    FBlueprintCommentLink() = default;
};

/**
 * Per-comment visual style for comment links.
 */
USTRUCT()
struct BLUEPRINTCOMMENTLINKSEDITOR_API FBlueprintCommentLinkCommentStyle
{
    GENERATED_BODY()

public:
    /** The NodeGuid of the comment that owns this style. */
    UPROPERTY()
    FGuid CommentGuid;

    /** Color of the little square handle rendered on the comment edge. */
    UPROPERTY()
    FLinearColor HandleColor = FLinearColor(0.95f, 0.35f, 0.0f, 1.0f);

    /** Color of the link spline, arrow, and tag text for links sourced from this comment. */
    UPROPERTY()
    FLinearColor WireColor   = FLinearColor(0.1f, 0.7f, 1.0f, 1.0f);

    /** Size of the arrow head for links sourced from this comment (graph-space units). */
    UPROPERTY()
    float ArrowSize = 7.0f;
};

/**
 * All comment links associated with a single graph.
 */
USTRUCT()
struct BLUEPRINTCOMMENTLINKSEDITOR_API FBlueprintCommentLinkGraphData
{
    GENERATED_BODY()

public:
    /** All links defined for this graph. */
    UPROPERTY()
    TArray<FBlueprintCommentLink> Links;

    /** Per-comment style data (independent of comment color). */
    UPROPERTY()
    TArray<FBlueprintCommentLinkCommentStyle> CommentStyles;

    /** Internal flag indicating whether metadata has been loaded. */
    bool bInitialized = false;

    bool IsEmpty() const
    {
        return Links.Num() == 0;
    }

    /** Returns all links that touch the specified comment. */
    TArray<FBlueprintCommentLink> GetLinksForComment(const FGuid& CommentGuid) const;

    /** Add a new link or replace an existing one with the same LinkGuid. */
    void AddOrUpdateLink(const FBlueprintCommentLink& Link);

    /** Remove all links that are connected to the given comment. */
    void RemoveLinksForComment(const FGuid& CommentGuid);

    /** Remove a link by its unique identifier. */
    void RemoveLinkByGuid(const FGuid& LinkGuid);

    /** Find or create a style entry for the given comment GUID. */
    FBlueprintCommentLinkCommentStyle& GetOrCreateStyle(const FGuid& CommentGuid);

    /** Find a style entry for the given comment GUID, or nullptr if none exists. */
    const FBlueprintCommentLinkCommentStyle* FindStyle(const FGuid& CommentGuid) const;

    /**
     * Load this graph's link data from its package metadata.
     * Returns true if any data was loaded.
     */
    bool LoadFromPackageMetaData(UEdGraph* Graph);

    /**
     * Save this graph's link data back to package metadata.
     */
    void SaveToPackageMetaData(UEdGraph* Graph);

    /**
     * Remove links that reference comments which no longer exist in the graph.
     */
    void CleanupGraph(UEdGraph* Graph);
};
