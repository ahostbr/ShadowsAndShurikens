#include "SOTS_BPGenInspector.h"

#include "SOTS_BPGenGraphResolver.h"
#include "SOTS_BlueprintGen.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"


namespace
{
	static FString Inspector_GetBPGenNodeId(const UEdGraphNode* Node)
	{
		return Node ? Node->NodeComment : FString();
	}

	static FString Inspector_GetNodeStableId(const UEdGraphNode* Node)
	{
		if (!Node)
		{
			return FString();
		}

		const FString NodeId = Inspector_GetBPGenNodeId(Node);
		return !NodeId.IsEmpty() ? NodeId : Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens);
	}

	static int32 GetPinDirectionOrder(ESOTS_BPGenPinDirection Direction)
	{
		return Direction == ESOTS_BPGenPinDirection::Input ? 0 : 1;
	}

	static FString ContainerTypeToString(EPinContainerType ContainerType)
	{
		switch (ContainerType)
		{
		case EPinContainerType::Array:
			return TEXT("Array");
		case EPinContainerType::Set:
			return TEXT("Set");
		case EPinContainerType::Map:
			return TEXT("Map");
		default:
			return TEXT("None");
		}
	}

	static void Inspector_SortPinsStable(TArray<FSOTS_BPGenDiscoveredPinDescriptor>& Pins)
	{
		Pins.Sort([](const FSOTS_BPGenDiscoveredPinDescriptor& A, const FSOTS_BPGenDiscoveredPinDescriptor& B)
		{
			const int32 DirA = GetPinDirectionOrder(A.Direction);
			const int32 DirB = GetPinDirectionOrder(B.Direction);
			if (DirA != DirB)
			{
				return DirA < DirB;
			}

			return A.PinName.LexicalLess(B.PinName);
		});
	}

	static void Inspector_SortLinksStable(TArray<FSOTS_BPGenNodeLink>& Links)
	{
		Links.Sort([](const FSOTS_BPGenNodeLink& A, const FSOTS_BPGenNodeLink& B)
		{
			int32 Cmp = A.FromNodeId.Compare(B.FromNodeId);
			if (Cmp != 0)
			{
				return Cmp < 0;
			}

			Cmp = A.FromPinName.Compare(B.FromPinName);
			if (Cmp != 0)
			{
				return Cmp < 0;
			}

			Cmp = A.ToNodeId.Compare(B.ToNodeId);
			if (Cmp != 0)
			{
				return Cmp < 0;
			}

			return A.ToPinName.Compare(B.ToPinName) < 0;
		});
	}

	static void Inspector_SortNodeSummariesStable(TArray<FSOTS_BPGenNodeSummary>& Nodes)
	{
		Nodes.Sort([](const FSOTS_BPGenNodeSummary& A, const FSOTS_BPGenNodeSummary& B)
		{
			const bool bAHasId = !A.NodeId.IsEmpty();
			const bool bBHasId = !B.NodeId.IsEmpty();
			if (bAHasId != bBHasId)
			{
				return bAHasId; // ids first
			}

			int32 Cmp = A.NodeId.Compare(B.NodeId);
			if (Cmp != 0)
			{
				return Cmp < 0;
			}

			Cmp = A.NodeClass.Compare(B.NodeClass);
			if (Cmp != 0)
			{
				return Cmp < 0;
			}

			if (A.NodePosY != B.NodePosY)
			{
				return A.NodePosY < B.NodePosY;
			}

			return A.NodePosX < B.NodePosX;
		});
	}

	static FSOTS_BPGenDiscoveredPinDescriptor Inspector_MakePinDescriptor(const UEdGraphPin* Pin)
	{
		FSOTS_BPGenDiscoveredPinDescriptor Descriptor;
		if (!Pin)
		{
			return Descriptor;
		}

		Descriptor.PinName = Pin->PinName;
		Descriptor.Direction = (Pin->Direction == EGPD_Output) ? ESOTS_BPGenPinDirection::Output : ESOTS_BPGenPinDirection::Input;
		Descriptor.PinCategory = Pin->PinType.PinCategory;
		Descriptor.PinSubCategory = Pin->PinType.PinSubCategory;
		if (UObject* SubObject = Pin->PinType.PinSubCategoryObject.Get())
		{
			Descriptor.SubObjectPath = SubObject->GetPathName();
		}
		Descriptor.ContainerType = ContainerTypeToString(Pin->PinType.ContainerType);
		Descriptor.DefaultValue = Pin->DefaultValue;
		Descriptor.bIsHidden = Pin->bHidden;
		Descriptor.bIsAdvanced = Pin->bAdvancedView;
		Descriptor.Tooltip = Pin->PinFriendlyName.ToString();
		return Descriptor;
	}

	static FSOTS_BPGenNodeSummary Inspector_MakeNodeSummary(UEdGraphNode* Node, bool bIncludePins)
	{
		FSOTS_BPGenNodeSummary Summary;
		if (!Node)
		{
			return Summary;
		}

		Summary.NodeId = Inspector_GetBPGenNodeId(Node);
		Summary.NodeGuid = Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens);
		Summary.NodeClass = Node->GetClass()->GetName();
		Summary.NodeClassPath = Node->GetClass()->GetPathName();
		Summary.Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
		Summary.RawName = Node->GetName();
		Summary.NodePosX = Node->NodePosX;
		Summary.NodePosY = Node->NodePosY;

		if (bIncludePins)
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				Summary.Pins.Add(Inspector_MakePinDescriptor(Pin));
			}

			Inspector_SortPinsStable(Summary.Pins);
		}

		return Summary;
	}

	static UBlueprint* Inspector_LoadBlueprintAsset(const FString& BlueprintPath)
	{
		if (BlueprintPath.IsEmpty())
		{
			return nullptr;
		}

		return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *BlueprintPath));
	}

	static UEdGraph* Inspector_FindFunctionGraph(UBlueprint* Blueprint, FName FunctionName)
	{
		if (!Blueprint)
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

		if (FunctionName.IsNone())
		{
			return nullptr;
		}

		FString ResolveError;
		FString ResolveErrorCode;
		FSOTS_BPGenGraphTarget EventTarget;
		EventTarget.BlueprintAssetPath = Blueprint->GetPathName();
		EventTarget.TargetType = TEXT("EventGraph");
		EventTarget.Name = FunctionName.ToString();
		EventTarget.bCreateIfMissing = false;
		UBlueprint* ResolverBlueprint = nullptr;
		UEdGraph* EventGraph = nullptr;
		if (USOTS_BPGenGraphResolver::ResolveTargetGraph(ResolverBlueprint, EventGraph, EventTarget, ResolveError, ResolveErrorCode))
		{
			return EventGraph;
		}

		return nullptr;
	}

	static bool Inspector_LoadBlueprintAndGraph(const FString& BlueprintPath, FName FunctionName, UBlueprint*& OutBlueprint, UEdGraph*& OutGraph, TArray<FString>& OutErrors)
	{
		OutBlueprint = Inspector_LoadBlueprintAsset(BlueprintPath);
		if (!OutBlueprint)
		{
			OutErrors.Add(FString::Printf(TEXT("Failed to load Blueprint '%s'."), *BlueprintPath));
			return false;
		}

		OutGraph = Inspector_FindFunctionGraph(OutBlueprint, FunctionName);
		if (!OutGraph)
		{
			OutErrors.Add(FString::Printf(TEXT("Function graph '%s' not found in '%s'."), *FunctionName.ToString(), *BlueprintPath));
			return false;
		}

		return true;
	}

	static bool Inspector_SaveBlueprintInternal(UBlueprint* Blueprint, FString& OutError)
	{
		if (!Blueprint)
		{
			OutError = TEXT("SaveBlueprintAsset: Blueprint was null.");
			return false;
		}

		UPackage* Package = Blueprint->GetOutermost();
		if (!Package)
		{
			OutError = TEXT("SaveBlueprintAsset: Package was null.");
			return false;
		}

		const FString FileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.bWarnOfLongFilename = false;

		if (!UPackage::SavePackage(Package, Blueprint, *FileName, SaveArgs))
		{
			OutError = TEXT("SaveBlueprintAsset: SavePackage returned false.");
			return false;
		}

		return true;
	}
}

FSOTS_BPGenNodeListResult USOTS_BPGenInspector::ListFunctionGraphNodes(const UObject* WorldContextObject, const FString& BlueprintPath, FName FunctionName, bool bIncludePins)
{
	FSOTS_BPGenNodeListResult Result;
	Result.BlueprintPath = BlueprintPath;
	Result.FunctionName = FunctionName;

	UBlueprint* Blueprint = nullptr;
	UEdGraph* FunctionGraph = nullptr;
	if (!Inspector_LoadBlueprintAndGraph(BlueprintPath, FunctionName, Blueprint, FunctionGraph, Result.Errors))
	{
		return Result;
	}

	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (!Node)
		{
			Result.Warnings.Add(TEXT("Encountered null node while listing graph."));
			continue;
		}

		Result.Nodes.Add(Inspector_MakeNodeSummary(Node, bIncludePins));
	}
	
	Inspector_SortNodeSummariesStable(Result.Nodes);

	Result.bSuccess = true;
	return Result;
}

FSOTS_BPGenNodeDescribeResult USOTS_BPGenInspector::DescribeNodeById(const UObject* WorldContextObject, const FString& BlueprintPath, FName FunctionName, const FString& NodeId, bool bIncludePins)
{
	FSOTS_BPGenNodeDescribeResult Result;
	Result.BlueprintPath = BlueprintPath;
	Result.FunctionName = FunctionName;
	Result.NodeId = NodeId;

	if (NodeId.IsEmpty())
	{
		Result.Errors.Add(TEXT("DescribeNodeById: NodeId was empty."));
		return Result;
	}

	UBlueprint* Blueprint = nullptr;
	UEdGraph* FunctionGraph = nullptr;
	if (!Inspector_LoadBlueprintAndGraph(BlueprintPath, FunctionName, Blueprint, FunctionGraph, Result.Errors))
	{
		return Result;
	}

	UEdGraphNode* TargetNode = nullptr;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (!Node)
		{
			continue;
		}

		const FString NodeStableId = Inspector_GetNodeStableId(Node);
		if (NodeStableId == NodeId)
		{
			TargetNode = Node;
			break;
		}
	}

	if (!TargetNode)
	{
		Result.Errors.Add(FString::Printf(TEXT("DescribeNodeById: NodeId '%s' not found."), *NodeId));
		return Result;
	}

	Result.Description.Summary = Inspector_MakeNodeSummary(TargetNode, bIncludePins);

	for (UEdGraphPin* Pin : TargetNode->Pins)
	{
		if (!Pin)
		{
			continue;
		}

		if (Pin->Direction == EGPD_Input)
		{
			FSOTS_BPGenPinDefault& PinDefault = Result.Description.PinDefaults.AddDefaulted_GetRef();
			PinDefault.PinName = Pin->PinName;
			PinDefault.DefaultValue = Pin->DefaultValue;
		}

		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			if (!LinkedPin)
			{
				continue;
			}

			FSOTS_BPGenNodeLink& Link = Result.Description.Links.AddDefaulted_GetRef();
			Link.FromNodeId = Inspector_GetNodeStableId(TargetNode);
			Link.FromPinName = Pin->PinName;

			UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
			Link.ToNodeId = Inspector_GetNodeStableId(LinkedNode);
			Link.ToPinName = LinkedPin->PinName;
		}
	}

	Inspector_SortPinsStable(Result.Description.Summary.Pins);
	Result.Description.PinDefaults.Sort([](const FSOTS_BPGenPinDefault& A, const FSOTS_BPGenPinDefault& B)
	{
		return A.PinName.LexicalLess(B.PinName);
	});
	Inspector_SortLinksStable(Result.Description.Links);

	Result.bSuccess = true;
	return Result;
}

FSOTS_BPGenMaintenanceResult USOTS_BPGenInspector::CompileBlueprintAsset(const UObject* WorldContextObject, const FString& BlueprintPath)
{
	FSOTS_BPGenMaintenanceResult Result;
	Result.BlueprintPath = BlueprintPath;

	UBlueprint* Blueprint = Inspector_LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		Result.Errors.Add(FString::Printf(TEXT("CompileBlueprintAsset: Failed to load Blueprint '%s'."), *BlueprintPath));
		return Result;
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	Result.bSuccess = true;
	Result.Message = TEXT("Compiled blueprint.");
	return Result;
}

FSOTS_BPGenMaintenanceResult USOTS_BPGenInspector::SaveBlueprintAsset(const UObject* WorldContextObject, const FString& BlueprintPath)
{
	FSOTS_BPGenMaintenanceResult Result;
	Result.BlueprintPath = BlueprintPath;

	UBlueprint* Blueprint = Inspector_LoadBlueprintAsset(BlueprintPath);
	if (!Blueprint)
	{
		Result.Errors.Add(FString::Printf(TEXT("SaveBlueprintAsset: Failed to load Blueprint '%s'."), *BlueprintPath));
		return Result;
	}

	FString SaveError;
	if (!Inspector_SaveBlueprintInternal(Blueprint, SaveError))
	{
		Result.Errors.Add(SaveError);
		return Result;
	}

	Result.bSuccess = true;
	Result.Message = TEXT("Saved blueprint.");
	return Result;
}

FSOTS_BPGenMaintenanceResult USOTS_BPGenInspector::RefreshAllNodesInFunction(const UObject* WorldContextObject, const FString& BlueprintPath, FName FunctionName, bool bIncludePins)
{
	FSOTS_BPGenMaintenanceResult Result;
	Result.BlueprintPath = BlueprintPath;
	Result.FunctionName = FunctionName;

	UBlueprint* Blueprint = nullptr;
	UEdGraph* FunctionGraph = nullptr;
	if (!Inspector_LoadBlueprintAndGraph(BlueprintPath, FunctionName, Blueprint, FunctionGraph, Result.Errors))
	{
		return Result;
	}

	int32 RefreshedCount = 0;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (!Node)
		{
			Result.Warnings.Add(TEXT("RefreshAllNodesInFunction: Encountered null node."));
			continue;
		}

		Node->ReconstructNode();
		Node->NodeConnectionListChanged();
		++RefreshedCount;
	}

	FunctionGraph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	if (bIncludePins)
	{
		FSOTS_BPGenNodeListResult ListResult = ListFunctionGraphNodes(WorldContextObject, BlueprintPath, FunctionName, true);
		Result.Nodes = ListResult.Nodes;
		Result.Warnings.Append(ListResult.Warnings);
		Result.Errors.Append(ListResult.Errors);
	}

	Result.bSuccess = Result.Errors.Num() == 0;
	Result.Message = FString::Printf(TEXT("Refreshed %d node(s) and compiled."), RefreshedCount);
	return Result;
}
