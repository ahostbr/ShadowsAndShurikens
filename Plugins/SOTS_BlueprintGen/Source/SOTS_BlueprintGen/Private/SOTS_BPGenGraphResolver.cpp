#include "SOTS_BPGenGraphResolver.h"

#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "WidgetBlueprint.h"

namespace
{
	static const FString ErrorUnsupportedTarget = TEXT("ERR_UNSUPPORTED_TARGET");
	static const FString ErrorTargetNotFound = TEXT("ERR_TARGET_NOT_FOUND");
	static const FString ErrorTargetCreateFailed = TEXT("ERR_TARGET_CREATE_FAILED");
	static const FString ErrorBlueprintMissing = TEXT("ERR_BLUEPRINT_MISSING");
}

bool USOTS_BPGenGraphResolver::ResolveTargetGraph(UBlueprint*& OutBlueprint, UEdGraph*& OutGraph, const FSOTS_BPGenGraphTarget& GraphTarget, FString& OutError, FString& OutErrorCode)
{
	OutBlueprint = nullptr;
	OutGraph = nullptr;
	OutError.Reset();
	OutErrorCode.Reset();

	if (!LoadBlueprint(GraphTarget, OutBlueprint, OutError, OutErrorCode))
	{
		return false;
	}

	const FString TargetType = NormalizeTargetType(GraphTarget.TargetType.IsEmpty() ? TEXT("Function") : GraphTarget.TargetType);
	const FString TargetName = GraphTarget.Name.IsEmpty() ? TEXT("EventGraph") : GraphTarget.Name;

	if (TargetType == TEXT("FUNCTION"))
	{
		return ResolveFunctionGraph(OutBlueprint, TargetName, GraphTarget.bCreateIfMissing, OutGraph, OutError, OutErrorCode);
	}

	if (TargetType == TEXT("EVENTGRAPH"))
	{
		return ResolveEventGraph(OutBlueprint, TargetName, GraphTarget.bCreateIfMissing, OutGraph, OutError, OutErrorCode);
	}

	if (TargetType == TEXT("CONSTRUCTIONSCRIPT"))
	{
		return ResolveConstructionScript(OutBlueprint, TargetName, GraphTarget.bCreateIfMissing, OutGraph, OutError, OutErrorCode);
	}

	if (TargetType == TEXT("MACRO"))
	{
		return ResolveMacroGraph(OutBlueprint, TargetName, GraphTarget.bCreateIfMissing, OutGraph, OutError, OutErrorCode);
	}

	if (TargetType == TEXT("WIDGETBINDING"))
	{
		return ResolveWidgetBindingGraph(OutBlueprint, TargetName, GraphTarget.bCreateIfMissing, OutGraph, OutError, OutErrorCode);
	}

	OutErrorCode = ErrorUnsupportedTarget;
	OutError = FString::Printf(TEXT("Unsupported target_type '%s'."), *GraphTarget.TargetType);
	return false;
}

bool USOTS_BPGenGraphResolver::LoadBlueprint(const FSOTS_BPGenGraphTarget& GraphTarget, UBlueprint*& OutBlueprint, FString& OutError, FString& OutErrorCode)
{
	OutBlueprint = nullptr;

	if (GraphTarget.BlueprintAssetPath.IsEmpty())
	{
		OutErrorCode = ErrorBlueprintMissing;
		OutError = TEXT("BlueprintAssetPath is empty.");
		return false;
	}

	OutBlueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *GraphTarget.BlueprintAssetPath));
	if (!OutBlueprint)
	{
		OutErrorCode = ErrorTargetNotFound;
		OutError = FString::Printf(TEXT("Failed to load Blueprint '%s'."), *GraphTarget.BlueprintAssetPath);
		return false;
	}

	return true;
}

bool USOTS_BPGenGraphResolver::ResolveFunctionGraph(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode)
{
	if (!Blueprint)
	{
		OutErrorCode = ErrorBlueprintMissing;
		OutError = TEXT("Blueprint was null in ResolveFunctionGraph.");
		return false;
	}

	const FName GraphFName(*TargetName);
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph && Graph->GetFName() == GraphFName)
		{
			OutGraph = Graph;
			return true;
		}
	}

	if (!bCreateIfMissing)
	{
		OutErrorCode = ErrorTargetNotFound;
		OutError = FString::Printf(TEXT("Function graph '%s' not found."), *TargetName);
		return false;
	}

	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, GraphFName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!NewGraph)
	{
		OutErrorCode = ErrorTargetCreateFailed;
		OutError = FString::Printf(TEXT("Could not create function graph '%s'."), *TargetName);
		return false;
	}

	FBlueprintEditorUtils::AddFunctionGraph<UFunction>(Blueprint, NewGraph, /*bIsUserCreated=*/true, nullptr);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	OutGraph = NewGraph;
	return true;
}

bool USOTS_BPGenGraphResolver::ResolveEventGraph(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode)
{
	if (!Blueprint)
	{
		OutErrorCode = ErrorBlueprintMissing;
		OutError = TEXT("Blueprint was null in ResolveEventGraph.");
		return false;
	}

	const FName GraphFName(*TargetName);
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph && Graph->GetFName() == GraphFName)
		{
			OutGraph = Graph;
			return true;
		}
	}

	if (!bCreateIfMissing)
	{
		OutErrorCode = ErrorTargetNotFound;
		OutError = FString::Printf(TEXT("EventGraph '%s' not found."), *TargetName);
		return false;
	}

	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, GraphFName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!NewGraph)
	{
		OutErrorCode = ErrorTargetCreateFailed;
		OutError = FString::Printf(TEXT("Could not create EventGraph '%s'."), *TargetName);
		return false;
	}

	Blueprint->UbergraphPages.Add(NewGraph);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	OutGraph = NewGraph;
	return true;
}

bool USOTS_BPGenGraphResolver::ResolveConstructionScript(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode)
{
	if (!Blueprint)
	{
		OutErrorCode = ErrorBlueprintMissing;
		OutError = TEXT("Blueprint was null in ResolveConstructionScript.");
		return false;
	}

	const FName GraphFName(*TargetName);
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph && Graph->GetFName() == GraphFName)
		{
			OutGraph = Graph;
			return true;
		}
	}

	if (!bCreateIfMissing)
	{
		OutErrorCode = ErrorTargetNotFound;
		OutError = FString::Printf(TEXT("ConstructionScript '%s' not found."), *TargetName);
		return false;
	}

	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, GraphFName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!NewGraph)
	{
		OutErrorCode = ErrorTargetCreateFailed;
		OutError = FString::Printf(TEXT("Could not create ConstructionScript '%s'."), *TargetName);
		return false;
	}

	Blueprint->UbergraphPages.Add(NewGraph);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	OutGraph = NewGraph;
	return true;
}

bool USOTS_BPGenGraphResolver::ResolveMacroGraph(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode)
{
	if (!Blueprint)
	{
		OutErrorCode = ErrorBlueprintMissing;
		OutError = TEXT("Blueprint was null in ResolveMacroGraph.");
		return false;
	}

	const FName GraphFName(*TargetName);
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		if (Graph && Graph->GetFName() == GraphFName)
		{
			OutGraph = Graph;
			return true;
		}
	}

	if (!bCreateIfMissing)
	{
		OutErrorCode = ErrorTargetNotFound;
		OutError = FString::Printf(TEXT("Macro graph '%s' not found."), *TargetName);
		return false;
	}

	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, GraphFName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!NewGraph)
	{
		OutErrorCode = ErrorTargetCreateFailed;
		OutError = FString::Printf(TEXT("Could not create macro graph '%s'."), *TargetName);
		return false;
	}

	FBlueprintEditorUtils::AddMacroGraph(Blueprint, NewGraph, /*bIsUserCreated=*/true, nullptr);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	OutGraph = NewGraph;
	return true;
}

bool USOTS_BPGenGraphResolver::ResolveWidgetBindingGraph(UBlueprint* Blueprint, const FString& TargetName, bool bCreateIfMissing, UEdGraph*& OutGraph, FString& OutError, FString& OutErrorCode)
{
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint);
	if (!WidgetBlueprint)
	{
		OutErrorCode = ErrorUnsupportedTarget;
		OutError = TEXT("Target blueprint is not a WidgetBlueprint.");
		return false;
	}

	return ResolveFunctionGraph(WidgetBlueprint, TargetName, bCreateIfMissing, OutGraph, OutError, OutErrorCode);
}

FString USOTS_BPGenGraphResolver::NormalizeTargetType(const FString& InType)
{
	FString Upper = InType;
	Upper.TrimStartAndEndInline();
	Upper = Upper.Replace(TEXT(" "), TEXT(""));
	Upper = Upper.ToUpper();
	return Upper;
}
