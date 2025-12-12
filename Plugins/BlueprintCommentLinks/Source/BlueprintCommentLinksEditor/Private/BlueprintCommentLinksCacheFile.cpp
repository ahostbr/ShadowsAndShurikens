#include "BlueprintCommentLinksCacheFile.h"

#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "BlueprintCommentLinksTypes.h"
#include "CoreGlobals.h"

UBlueprintCommentLinkCacheFile* GCommentLinkCacheSingleton = nullptr;

UBlueprintCommentLinkCacheFile& UBlueprintCommentLinkCacheFile::Get()
{
    if (!GCommentLinkCacheSingleton)
    {
        GCommentLinkCacheSingleton = NewObject<UBlueprintCommentLinkCacheFile>(GetTransientPackage(), TEXT("BlueprintCommentLinkCacheFile"));
        GCommentLinkCacheSingleton->AddToRoot();

        // No global delegate needed; the editor module calls TearDown() on shutdown.
    }

    return *GCommentLinkCacheSingleton;
}

void UBlueprintCommentLinkCacheFile::TearDown()
{
    if (!GCommentLinkCacheSingleton)
    {
        return;
    }

    // If we're in exit purge, the UObject array is already shutting down and
    // touching the root set can assert. At that point the process is exiting
    // anyway, so just drop the pointer.
    if (GExitPurge || !GCommentLinkCacheSingleton->IsValidLowLevelFast(false))
    {
        GCommentLinkCacheSingleton = nullptr;
        return;
    }

    // Ensure everything is saved one last time while it's still safe.
    GCommentLinkCacheSingleton->OnPreExit();

    GCommentLinkCacheSingleton->RemoveFromRoot();
    GCommentLinkCacheSingleton = nullptr;
}

void UBlueprintCommentLinkCacheFile::OnObjectLoaded(UObject* LoadedObject)
{
    UEdGraph* Graph = Cast<UEdGraph>(LoadedObject);
    if (!Graph)
    {
        return;
    }

    FBlueprintCommentLinkGraphData& Data = GetCacheFileGraphData(Graph);

    if (!Data.bInitialized)
    {
        Data.LoadFromPackageMetaData(Graph);
        Data.CleanupGraph(Graph);
        Data.bInitialized = true;
    }
}

void UBlueprintCommentLinkCacheFile::OnPreExit()
{
    // Persist all known graphs back to metadata.
    for (const TPair<FName, TWeakObjectPtr<UEdGraph>>& Pair : GraphLookup)
    {
        if (UEdGraph* Graph = Pair.Value.Get())
        {
            if (FBlueprintCommentLinkGraphData* Data = CacheData.Find(Pair.Key))
            {
                Data->SaveToPackageMetaData(Graph);
            }
        }
    }
}

FBlueprintCommentLinkGraphData& UBlueprintCommentLinkCacheFile::GetGraphData(UEdGraph* Graph)
{
    return GetCacheFileGraphData(Graph);
}

void UBlueprintCommentLinkCacheFile::UpdateGraphLinks(UEdGraph* Graph)
{
    if (!Graph)
    {
        return;
    }

    FBlueprintCommentLinkGraphData& Data = GetCacheFileGraphData(Graph);
    Data.SaveToPackageMetaData(Graph);
}

TArray<FBlueprintCommentLink> UBlueprintCommentLinkCacheFile::GetLinksForComment(UEdGraph* Graph, const FGuid& CommentGuid)
{
    if (!Graph)
    {
        return {};
    }

    FBlueprintCommentLinkGraphData& Data = GetCacheFileGraphData(Graph);
    return Data.GetLinksForComment(CommentGuid);
}

void UBlueprintCommentLinkCacheFile::AddOrUpdateLink(UEdGraph* Graph, const FBlueprintCommentLink& Link)
{
    if (!Graph)
    {
        return;
    }

    FBlueprintCommentLinkGraphData& Data = GetCacheFileGraphData(Graph);
    Data.AddOrUpdateLink(Link);
    UpdateGraphLinks(Graph);
}

void UBlueprintCommentLinkCacheFile::RemoveLinksForComment(UEdGraph* Graph, const FGuid& CommentGuid)
{
    if (!Graph)
    {
        return;
    }

    FBlueprintCommentLinkGraphData& Data = GetCacheFileGraphData(Graph);
    Data.RemoveLinksForComment(CommentGuid);
    UpdateGraphLinks(Graph);
}

void UBlueprintCommentLinkCacheFile::RemoveLinkByGuid(UEdGraph* Graph, const FGuid& LinkGuid)
{
    if (!Graph)
    {
        return;
    }

    FBlueprintCommentLinkGraphData& Data = GetCacheFileGraphData(Graph);
    Data.RemoveLinkByGuid(LinkGuid);
    UpdateGraphLinks(Graph);
}

FBlueprintCommentLinkGraphData& UBlueprintCommentLinkCacheFile::GetCacheFileGraphData(UEdGraph* Graph)
{
    check(Graph);

    const FName Key = MakeGraphKey(Graph);

    if (FBlueprintCommentLinkGraphData* Existing = CacheData.Find(Key))
    {
        return *Existing;
    }

    FBlueprintCommentLinkGraphData& NewData = CacheData.Add(Key);
    NewData.LoadFromPackageMetaData(Graph);
    NewData.CleanupGraph(Graph);
    NewData.bInitialized = true;

    GraphLookup.FindOrAdd(Key) = Graph;

    return NewData;
}

FName UBlueprintCommentLinkCacheFile::MakeGraphKey(UEdGraph* Graph) const
{
    check(Graph);
    return FName(*Graph->GetPathName());
}
