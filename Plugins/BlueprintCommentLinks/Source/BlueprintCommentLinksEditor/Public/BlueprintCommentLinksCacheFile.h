#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BlueprintCommentLinksTypes.h"
#include "BlueprintCommentLinksCacheFile.generated.h"

class UEdGraph;

/**
 * Global cache responsible for storing per-graph comment link data and
 * persisting it to package metadata.
 */
UCLASS()
class BLUEPRINTCOMMENTLINKSEDITOR_API UBlueprintCommentLinkCacheFile : public UObject
{
    GENERATED_BODY()

public:
    /** Get the singleton instance, creating it if necessary. */
    static UBlueprintCommentLinkCacheFile& Get();

    /** Tear down the singleton and unregister editor delegates. */
    static void TearDown();

    /** Called whenever an object is loaded; used to discover graphs. */
    void OnObjectLoaded(UObject* LoadedObject);

    /** Called when the editor is about to exit. */
    void OnPreExit();

    /** Get (and lazily load) graph data for the specified graph. */
    FBlueprintCommentLinkGraphData& GetGraphData(UEdGraph* Graph);

    /** Save this graph's data back to metadata. */
    void UpdateGraphLinks(UEdGraph* Graph);

    /** Convenience wrappers that operate on the underlying graph data. */
    TArray<FBlueprintCommentLink> GetLinksForComment(UEdGraph* Graph, const FGuid& CommentGuid);
    void AddOrUpdateLink(UEdGraph* Graph, const FBlueprintCommentLink& Link);
    void RemoveLinksForComment(UEdGraph* Graph, const FGuid& CommentGuid);
    void RemoveLinkByGuid(UEdGraph* Graph, const FGuid& LinkGuid);

protected:
    /** Per-graph data, keyed by the graph path name. */
    UPROPERTY()
    TMap<FName, FBlueprintCommentLinkGraphData> CacheData;

    /** Weak references to graphs so we can save them on shutdown. */
    TMap<FName, TWeakObjectPtr<UEdGraph>> GraphLookup;

    FDelegateHandle OnPreExitHandle;

    /** Get or create cache data for a graph without saving. */
    FBlueprintCommentLinkGraphData& GetCacheFileGraphData(UEdGraph* Graph);

    /** Build a stable key for the given graph. */
    FName MakeGraphKey(UEdGraph* Graph) const;
};
