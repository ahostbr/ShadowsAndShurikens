#include "SOTS_BPGenBuildCommandlet.h"

#include "SOTS_BlueprintGen.h"
#include "SOTS_BPGenBuilder.h"

#include "Containers/StringView.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

/**
 * Helper: Load a JSON file into a root object.
 */
static bool SOTS_BPGen_LoadJsonObject(const FString& FilePath, TSharedPtr<FJsonObject>& OutRoot, FString& OutError)
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
	{
		OutError = FString::Printf(TEXT("Failed to load JSON file '%s'."), *FilePath);
		return false;
	}

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, OutRoot) || !OutRoot.IsValid())
	{
		OutError = FString::Printf(TEXT("Failed to parse JSON in '%s'."), *FilePath);
		return false;
	}

	return true;
}

/**
 * Helper: Deserialize an array of UStructs from a JSON array field.
 */
template<typename TStructType>
static void SOTS_BPGen_LoadArrayField(
	const TSharedPtr<FJsonObject>& Root,
	const FString& FieldName,
	TArray<TStructType>& OutArray,
	TArray<FString>& OutWarnings)
{
	if (!Root.IsValid())
	{
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
	if (!Root->TryGetArrayField(FieldName, JsonArray) || !JsonArray)
	{
		return; // Field missing is not an error; just means "none".
	}

	for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
	{
		if (!Value.IsValid())
		{
			continue;
		}

		TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (!Obj.IsValid())
		{
			OutWarnings.Add(FString::Printf(
				TEXT("Field '%s' contains a non-object entry; skipping."), *FieldName));
			continue;
		}

		TStructType Item;
		if (!FJsonObjectConverter::JsonObjectToUStruct(
				Obj.ToSharedRef(),
				TStructType::StaticStruct(),
				&Item,
				/*CheckFlags=*/0,
				/*SkipFlags=*/0))
		{
			OutWarnings.Add(FString::Printf(
				TEXT("Failed to convert entry in '%s' to struct '%s'; skipping."),
				*FieldName,
				*TStructType::StaticStruct()->GetName()));
			continue;
		}

		OutArray.Add(MoveTemp(Item));
	}
}

USOTS_BPGenBuildCommandlet::USOTS_BPGenBuildCommandlet()
{
	IsClient = false;
	IsServer = false;
	LogToConsole = true;
}

int32 USOTS_BPGenBuildCommandlet::Main(const FString& Params)
{
	UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SOTS_BPGenBuildCommandlet starting. Params: %s"), *Params);

	if (FParse::Param(*Params, TEXT("TestAllNodesBPPrintHello")))
	{
		const FSOTS_BPGenApplyResult TestResult =
			USOTS_BPGenBuilder::BuildTestAllNodesGraphForBPPrintHello();

		for (const FString& Warning : TestResult.Warnings)
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SOTS_BPGen: %s"), *Warning);
		}

		if (!TestResult.bSuccess)
		{
			UE_LOG(LogSOTS_BlueprintGen, Error,
				TEXT("SOTS_BPGen: BuildTestAllNodesGraphForBPPrintHello failed: %s"),
				*TestResult.ErrorMessage);
			return 1;
		}

		UE_LOG(LogSOTS_BlueprintGen, Display,
			TEXT("SOTS_BPGen: Built Test_AllNodesGraph on BP_PrintHello."));
		return 0;
	}

	FString JobFilePath;
	FString GraphSpecFilePath;

	// Parse command line params: -JobFile=..., -GraphSpecFile=...
	if (!FParse::Value(*Params, TEXT("JobFile="), JobFilePath))
	{
		UE_LOG(LogSOTS_BlueprintGen, Error,
			TEXT("SOTS_BPGenBuildCommandlet: Missing -JobFile=<path> argument."));
		return 1;
	}
	FParse::Value(*Params, TEXT("GraphSpecFile="), GraphSpecFilePath);

	if (FPaths::IsRelative(JobFilePath))
	{
		JobFilePath = FPaths::ConvertRelativePathToFull(JobFilePath);
	}
	if (!GraphSpecFilePath.IsEmpty() && FPaths::IsRelative(GraphSpecFilePath))
	{
		GraphSpecFilePath = FPaths::ConvertRelativePathToFull(GraphSpecFilePath);
	}

	UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("JobFile: '%s'"), *JobFilePath);
	if (!GraphSpecFilePath.IsEmpty())
	{
		UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("GraphSpecFile: '%s'"), *GraphSpecFilePath);
	}

	// Load job JSON.
	TSharedPtr<FJsonObject> JobRoot;
	FString JsonError;
	if (!SOTS_BPGen_LoadJsonObject(JobFilePath, JobRoot, JsonError))
	{
		UE_LOG(LogSOTS_BlueprintGen, Error,
			TEXT("SOTS_BPGenBuildCommandlet: %s"), *JsonError);
		return 1;
	}

	// Required: "Function" (FSOTS_BPGenFunctionDef)
	FSOTS_BPGenFunctionDef FunctionDef;
	{
		const FStringView FunctionField(TEXT("Function"));
		const TSharedPtr<FJsonObject>* FunctionObjPtr = nullptr;
		if (!JobRoot->TryGetObjectField(FunctionField, FunctionObjPtr) || !FunctionObjPtr || !FunctionObjPtr->IsValid())
		{
			UE_LOG(LogSOTS_BlueprintGen, Error,
				TEXT("SOTS_BPGenBuildCommandlet: Job JSON is missing a valid 'Function' object."));
			return 1;
		}

		TSharedPtr<FJsonObject> FunctionObj = *FunctionObjPtr;
		if (!FJsonObjectConverter::JsonObjectToUStruct(
				FunctionObj.ToSharedRef(),
				FSOTS_BPGenFunctionDef::StaticStruct(),
				&FunctionDef,
				/*CheckFlags=*/0,
				/*SkipFlags=*/0))
		{
			UE_LOG(LogSOTS_BlueprintGen, Error,
				TEXT("SOTS_BPGenBuildCommandlet: Failed to convert 'Function' object to FSOTS_BPGenFunctionDef."));
			return 1;
		}
	}

	// Optional: "StructsToCreate" (array of FSOTS_BPGenStructDef)
	TArray<FSOTS_BPGenStructDef> StructDefs;
	// Optional: "EnumsToCreate" (array of FSOTS_BPGenEnumDef)
	TArray<FSOTS_BPGenEnumDef> EnumDefs;
	// Optional: inline "GraphSpec" (FSOTS_BPGenGraphSpec)
	FSOTS_BPGenGraphSpec InlineGraphSpec;
	bool bHasInlineGraphSpec = false;

	TArray<FString> ParseWarnings;
	SOTS_BPGen_LoadArrayField(JobRoot, TEXT("StructsToCreate"), StructDefs, ParseWarnings);
	SOTS_BPGen_LoadArrayField(JobRoot, TEXT("EnumsToCreate"), EnumDefs, ParseWarnings);

	{
		const FStringView GraphField(TEXT("GraphSpec"));
		const TSharedPtr<FJsonObject>* GraphObjPtr = nullptr;
		if (JobRoot->TryGetObjectField(GraphField, GraphObjPtr) && GraphObjPtr && GraphObjPtr->IsValid())
		{
			TSharedPtr<FJsonObject> GraphObj = *GraphObjPtr;
			if (FJsonObjectConverter::JsonObjectToUStruct(
					GraphObj.ToSharedRef(),
					FSOTS_BPGenGraphSpec::StaticStruct(),
					&InlineGraphSpec,
					/*CheckFlags=*/0,
					/*SkipFlags=*/0))
			{
				bHasInlineGraphSpec = true;
			}
			else
			{
				ParseWarnings.Add(TEXT("Failed to convert inline 'GraphSpec' to FSOTS_BPGenGraphSpec; ignoring."));
			}
		}
	}

	// Optional external GraphSpec file takes precedence if present.
	FSOTS_BPGenGraphSpec ExternalGraphSpec;
	bool bHasExternalGraphSpec = false;

	if (!GraphSpecFilePath.IsEmpty())
	{
		TSharedPtr<FJsonObject> GraphRoot;
		FString GraphError;
		if (!SOTS_BPGen_LoadJsonObject(GraphSpecFilePath, GraphRoot, GraphError))
		{
			UE_LOG(LogSOTS_BlueprintGen, Error,
				TEXT("SOTS_BPGenBuildCommandlet: %s"), *GraphError);
			return 1;
		}

		if (!FJsonObjectConverter::JsonObjectToUStruct(
				GraphRoot.ToSharedRef(),
				FSOTS_BPGenGraphSpec::StaticStruct(),
				&ExternalGraphSpec,
				/*CheckFlags=*/0,
				/*SkipFlags=*/0))
		{
			UE_LOG(LogSOTS_BlueprintGen, Error,
				TEXT("SOTS_BPGenBuildCommandlet: Failed to convert external GraphSpec JSON to FSOTS_BPGenGraphSpec."));
			return 1;
		}

		bHasExternalGraphSpec = true;
	}

	// Log warnings from parse step, if any.
	for (const FString& Warning : ParseWarnings)
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SOTS_BPGenBuildCommandlet: %s"), *Warning);
	}

	// Execute the job via USOTS_BPGenBuilder.
	int32 ErrorCount = 0;

	UE_LOG(LogSOTS_BlueprintGen, Display,
		TEXT("SOTS_BPGenBuildCommandlet: Creating %d struct(s) and %d enum(s)."),
		StructDefs.Num(), EnumDefs.Num());

	for (const FSOTS_BPGenStructDef& StructDef : StructDefs)
	{
		const FSOTS_BPGenAssetResult StructResult =
			USOTS_BPGenBuilder::CreateStructAssetFromDef(this, StructDef);

		if (!StructResult.bSuccess)
		{
			UE_LOG(LogSOTS_BlueprintGen, Error,
				TEXT("Struct generation failed for '%s': %s"),
				*StructResult.AssetPath,
				*StructResult.Message);
			++ErrorCount;
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Display,
				TEXT("Struct OK: %s"), *StructResult.Message);
		}
	}

	for (const FSOTS_BPGenEnumDef& EnumDef : EnumDefs)
	{
		const FSOTS_BPGenAssetResult EnumResult =
			USOTS_BPGenBuilder::CreateEnumAssetFromDef(this, EnumDef);

		if (!EnumResult.bSuccess)
		{
			UE_LOG(LogSOTS_BlueprintGen, Error,
				TEXT("Enum generation failed for '%s': %s"),
				*EnumResult.AssetPath,
				*EnumResult.Message);
			++ErrorCount;
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Display,
				TEXT("Enum OK: %s"), *EnumResult.Message);
		}
	}

	UE_LOG(LogSOTS_BlueprintGen, Display,
		TEXT("SOTS_BPGenBuildCommandlet: Applying function skeleton for '%s' in '%s'."),
		*FunctionDef.FunctionName.ToString(),
		*FunctionDef.TargetBlueprintPath);

	const FSOTS_BPGenApplyResult SkeletonResult =
		USOTS_BPGenBuilder::ApplyFunctionSkeleton(this, FunctionDef);

	if (!SkeletonResult.bSuccess)
	{
		UE_LOG(LogSOTS_BlueprintGen, Error,
			TEXT("Function skeleton failed for '%s' in '%s': %s"),
			*SkeletonResult.FunctionName.ToString(),
			*SkeletonResult.TargetBlueprintPath,
			*SkeletonResult.ErrorMessage);
		++ErrorCount;
	}
	else
	{
		for (const FString& Warn : SkeletonResult.Warnings)
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("Function skeleton warning: %s"), *Warn);
		}
	}

	// Graph spec application (inline or external).
	bool bHasGraphSpec = bHasExternalGraphSpec || bHasInlineGraphSpec;
	if (bHasGraphSpec)
	{
		const FSOTS_BPGenGraphSpec& GraphSpecToUse =
			bHasExternalGraphSpec ? ExternalGraphSpec : InlineGraphSpec;

		UE_LOG(LogSOTS_BlueprintGen, Display,
			TEXT("SOTS_BPGenBuildCommandlet: Applying graph spec with %d node(s) and %d link(s)."),
			GraphSpecToUse.Nodes.Num(),
			GraphSpecToUse.Links.Num());

		const FSOTS_BPGenApplyResult GraphResult =
			USOTS_BPGenBuilder::ApplyGraphSpecToFunction(this, FunctionDef, GraphSpecToUse);

		if (!GraphResult.bSuccess)
		{
			UE_LOG(LogSOTS_BlueprintGen, Error,
				TEXT("Graph spec apply failed for '%s' in '%s': %s"),
				*GraphResult.FunctionName.ToString(),
				*GraphResult.TargetBlueprintPath,
				*GraphResult.ErrorMessage);
			++ErrorCount;
		}
		else
		{
			for (const FString& Warn : GraphResult.Warnings)
			{
				UE_LOG(LogSOTS_BlueprintGen, Warning,
					TEXT("Graph apply warning: %s"), *Warn);
			}
		}
	}
	else
	{
		UE_LOG(LogSOTS_BlueprintGen, Display,
			TEXT("SOTS_BPGenBuildCommandlet: No graph spec provided; only function skeleton applied."));
	}

	if (ErrorCount > 0)
	{
		UE_LOG(LogSOTS_BlueprintGen, Error,
			TEXT("SOTS_BPGenBuildCommandlet finished with %d error(s)."), ErrorCount);
		return 1;
	}

	UE_LOG(LogSOTS_BlueprintGen, Display,
		TEXT("SOTS_BPGenBuildCommandlet finished successfully with 0 errors."));
	return 0;
}
