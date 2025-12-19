#include "SOTS_BPGenDebug.h"

#include "BlueprintEditor.h"
#include "Editor.h"
#include "Math/Color.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SOTS_BPGenInspector.h"

#if WITH_EDITOR
#include "Subsystems/AssetEditorSubsystem.h"
#endif

namespace
{
	static UBlueprint* LoadBlueprintForEdit(const FString& BlueprintAssetPath)
	{
#if WITH_EDITOR
		return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *BlueprintAssetPath));
#else
		return nullptr;
#endif
	}

	static UEdGraph* FindFunctionGraph(UBlueprint* Blueprint, FName FunctionName)
	{
#if WITH_EDITOR
		if (!Blueprint || FunctionName.IsNone())
		{
			return nullptr;
		}

		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetFName() == FunctionName)
			{
				return Graph;
			}
		}

		return nullptr;
#else
		return nullptr;
#endif
	}

	static FString GetBPGenNodeId(const UEdGraphNode* Node)
	{
		if (!Node)
		{
			return FString();
		}

		if (!Node->NodeComment.IsEmpty())
		{
			return Node->NodeComment;
		}

		return Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens);
	}

	static bool MatchesAnyId(const UEdGraphNode* Node, const TSet<FString>& TargetIds)
	{
		return Node && TargetIds.Contains(GetBPGenNodeId(Node));
	}

	static bool IsBPGenAnnotation(const FString& Message)
	{
		return Message.StartsWith(TEXT("[BPGEN]"), ESearchCase::IgnoreCase);
	}
}

bool USOTS_BPGenDebug::AnnotateNodes(const UObject* WorldContextObject, const FString& BlueprintAssetPath, FName FunctionName, const TArray<FString>& NodeIds, const FString& AnnotationText)
{
#if WITH_EDITOR
	UBlueprint* Blueprint = LoadBlueprintForEdit(BlueprintAssetPath);
	if (!Blueprint)
	{
		return false;
	}

	UEdGraph* Graph = FindFunctionGraph(Blueprint, FunctionName);
	if (!Graph)
	{
		return false;
	}

	TSet<FString> Targets(NodeIds);
	const FString Prefix = AnnotationText.IsEmpty() ? TEXT("[BPGEN]") : AnnotationText;
	bool bChanged = false;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!MatchesAnyId(Node, Targets))
		{
			continue;
		}

		const FString ExistingId = GetBPGenNodeId(Node);
		Node->ErrorMsg = Prefix;
		Node->bHasCompilerMessage = true;
		Node->bCommentBubbleVisible = true;
		Node->bCommentBubblePinned = true;
		Node->NodeComment = ExistingId; // preserve NodeId storage
		bChanged = true;
	}

	if (bChanged)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}

	return bChanged;
#else
	return false;
#endif
}

bool USOTS_BPGenDebug::ClearAnnotations(const UObject* WorldContextObject, const FString& BlueprintAssetPath, FName FunctionName)
{
#if WITH_EDITOR
	UBlueprint* Blueprint = LoadBlueprintForEdit(BlueprintAssetPath);
	if (!Blueprint)
	{
		return false;
	}

	UEdGraph* Graph = FindFunctionGraph(Blueprint, FunctionName);
	if (!Graph)
	{
		return false;
	}

	bool bChanged = false;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node)
		{
			continue;
		}

		if (IsBPGenAnnotation(Node->ErrorMsg))
		{
			Node->ErrorMsg.Reset();
			Node->bHasCompilerMessage = false;
			Node->bCommentBubbleVisible = false;
			Node->bCommentBubblePinned = false;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}

	return bChanged;
#else
	return false;
#endif
}

bool USOTS_BPGenDebug::FocusNodeById(const UObject* WorldContextObject, const FString& BlueprintAssetPath, FName FunctionName, const FString& NodeId)
{
#if WITH_EDITOR
	UBlueprint* Blueprint = LoadBlueprintForEdit(BlueprintAssetPath);
	if (!Blueprint || NodeId.IsEmpty())
	{
		return false;
	}

	UEdGraph* Graph = FindFunctionGraph(Blueprint, FunctionName);
	if (!Graph)
	{
		return false;
	}

	UEdGraphNode* TargetNode = nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && GetBPGenNodeId(Node) == NodeId)
		{
			TargetNode = Node;
			break;
		}
	}

	if (!TargetNode)
	{
		return false;
	}

	// Open the blueprint editor and focus on the node.
	IAssetEditorInstance* EditorInstance = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Blueprint, true) : nullptr;
	if (!EditorInstance)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Blueprint);
	}

	FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Blueprint, true));
	if (BlueprintEditor)
	{
		BlueprintEditor->JumpToNode(TargetNode, false);
		return true;
	}

	return false;
#else
	return false;
#endif
}
