#include "SOTS_BPGenExamples.h"

#include "SOTS_BPGenBuilder.h"
#include "SOTS_BPGenTypes.h"

FSOTS_BPGenApplyResult USOTS_BPGenExamples::Example_ApplyPrintStringGraph(const FString& TargetBlueprintPath, FName FunctionName, const FString& MessageText)
{
	// Ensure a function exists.
	FSOTS_BPGenFunctionDef FunctionDef;
	FunctionDef.TargetBlueprintPath = TargetBlueprintPath;
	FunctionDef.FunctionName = FunctionName.IsNone() ? FName(TEXT("BPGen_PrintString")) : FunctionName;

	FSOTS_BPGenApplyResult SkeletonResult = USOTS_BPGenBuilder::ApplyFunctionSkeleton(nullptr, FunctionDef);
	if (!SkeletonResult.bSuccess)
	{
		return SkeletonResult;
	}

	// Build graph spec: Entry -> PrintString -> Result.
	FSOTS_BPGenGraphSpec GraphSpec;

	FSOTS_BPGenGraphNode PrintStringNode;
	PrintStringNode.Id = TEXT("PrintString");
	PrintStringNode.SpawnerKey = TEXT("/Script/Engine.KismetSystemLibrary:PrintString");
	PrintStringNode.NodeType = FName(TEXT("K2Node_CallFunction"));
	PrintStringNode.FunctionPath = TEXT("/Script/Engine.KismetSystemLibrary:PrintString");
	PrintStringNode.NodePosition = FVector2D(300.f, 0.f);
	PrintStringNode.ExtraData.Add(FName(TEXT("InString.DefaultValue")), MessageText);
	GraphSpec.Nodes.Add(PrintStringNode);

	FSOTS_BPGenGraphLink EntryToPrint;
	EntryToPrint.FromNodeId = TEXT("Entry");
	EntryToPrint.FromPinName = FName(TEXT("then"));
	EntryToPrint.ToNodeId = PrintStringNode.Id;
	EntryToPrint.ToPinName = FName(TEXT("execute"));
	GraphSpec.Links.Add(EntryToPrint);

	FSOTS_BPGenGraphLink PrintToResult;
	PrintToResult.FromNodeId = PrintStringNode.Id;
	PrintToResult.FromPinName = FName(TEXT("then"));
	PrintToResult.ToNodeId = TEXT("Result");
	PrintToResult.ToPinName = FName(TEXT("execute"));
	GraphSpec.Links.Add(PrintToResult);

	return USOTS_BPGenBuilder::ApplyGraphSpecToFunction(nullptr, FunctionDef, GraphSpec);
}
