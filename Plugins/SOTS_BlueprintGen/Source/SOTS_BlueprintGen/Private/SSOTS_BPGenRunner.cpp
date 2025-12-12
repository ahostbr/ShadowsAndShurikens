#include "SSOTS_BPGenRunner.h"

#include "SOTS_BlueprintGen.h"
#include "SOTS_BPGenBuilder.h"
#include "SOTS_BPGenTypes.h"

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** Load a JSON file into a root object. */
	static bool LoadJsonObject(const FString& FilePath, TSharedPtr<FJsonObject>& OutRoot, FString& OutError)
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

	/** Deserialize an array of UStructs from a JSON array field. Mirrors commandlet behavior. */
	template<typename TStructType>
	static void LoadArrayField(
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
}

void SSOTS_BPGenRunner::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("SOTS_BPGenRunner", "JobPathLabel", "Job JSON Path"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SAssignNew(JobPathTextBox, SEditableTextBox)
			.HintText(NSLOCTEXT("SOTS_BPGenRunner", "JobPathHint", "e.g. Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_HelloWorld_Job.json"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("SOTS_BPGenRunner", "GraphPathLabel", "GraphSpec JSON Path (optional)"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SAssignNew(GraphPathTextBox, SEditableTextBox)
			.HintText(NSLOCTEXT("SOTS_BPGenRunner", "GraphPathHint", "e.g. Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_HelloWorld_GraphSpec.json"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SButton)
			.Text(NSLOCTEXT("SOTS_BPGenRunner", "RunButtonLabel", "Run BPGen Job"))
			.OnClicked(this, &SSOTS_BPGenRunner::OnRunClicked)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SSeparator)
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(4.0f)
		[
			SAssignNew(LogTextBox, SMultiLineEditableTextBox)
			.IsReadOnly(true)
			.AlwaysShowScrollbars(true)
			.AutoWrapText(true)
		]
	];
}

void SSOTS_BPGenRunner::AppendLogLine(const FString& Line)
{
	if (!LogTextBox.IsValid())
	{
		return;
	}

	FString Existing = LogTextBox->GetText().ToString();
	if (!Existing.IsEmpty())
	{
		Existing.AppendChar(TEXT('\n'));
	}
	Existing.Append(Line);
	LogTextBox->SetText(FText::FromString(Existing));
}

void SSOTS_BPGenRunner::SetLogLines(const TArray<FString>& Lines)
{
	if (!LogTextBox.IsValid())
	{
		return;
	}

	FString Combined = FString::Join(Lines, TEXT("\n"));
	LogTextBox->SetText(FText::FromString(Combined));
}

FReply SSOTS_BPGenRunner::OnRunClicked()
{
	TArray<FString> RunLog;
	auto LogLine = [&RunLog](const FString& Msg)
	{
		RunLog.Add(Msg);
		UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("%s"), *Msg);
	};

	FString JobPath = JobPathTextBox.IsValid() ? JobPathTextBox->GetText().ToString() : FString();
	FString GraphPath = GraphPathTextBox.IsValid() ? GraphPathTextBox->GetText().ToString() : FString();

	if (JobPath.IsEmpty())
	{
		LogLine(TEXT("Job path is empty. Please provide a Job JSON file."));
		SetLogLines(RunLog);
		return FReply::Handled();
	}

	if (FPaths::IsRelative(JobPath))
	{
		JobPath = FPaths::ConvertRelativePathToFull(JobPath);
	}
	if (!GraphPath.IsEmpty() && FPaths::IsRelative(GraphPath))
	{
		GraphPath = FPaths::ConvertRelativePathToFull(GraphPath);
	}

	LogLine(FString::Printf(TEXT("JobFile: %s"), *JobPath));
	if (!GraphPath.IsEmpty())
	{
		LogLine(FString::Printf(TEXT("GraphSpecFile: %s"), *GraphPath));
	}

	TSharedPtr<FJsonObject> JobRoot;
	FString JsonError;
	if (!LoadJsonObject(JobPath, JobRoot, JsonError))
	{
		LogLine(JsonError);
		SetLogLines(RunLog);
		return FReply::Handled();
	}

	// Required Function field.
	FSOTS_BPGenFunctionDef FunctionDef;
	{
		const TSharedPtr<FJsonObject>* FunctionObjPtr = nullptr;
		if (!JobRoot->TryGetObjectField(TEXT("Function"), FunctionObjPtr) || !FunctionObjPtr || !FunctionObjPtr->IsValid())
		{
			LogLine(TEXT("Job JSON is missing a valid 'Function' object."));
			SetLogLines(RunLog);
			return FReply::Handled();
		}

		if (!FJsonObjectConverter::JsonObjectToUStruct(
				FunctionObjPtr->ToSharedRef(),
				FSOTS_BPGenFunctionDef::StaticStruct(),
				&FunctionDef,
				/*CheckFlags=*/0,
				/*SkipFlags=*/0))
		{
			LogLine(TEXT("Failed to convert 'Function' to FSOTS_BPGenFunctionDef."));
			SetLogLines(RunLog);
			return FReply::Handled();
		}
	}

	TArray<FSOTS_BPGenStructDef> StructDefs;
	TArray<FSOTS_BPGenEnumDef> EnumDefs;
	FSOTS_BPGenGraphSpec InlineGraphSpec;
	bool bHasInlineGraphSpec = false;
	TArray<FString> ParseWarnings;

	LoadArrayField(JobRoot, TEXT("StructsToCreate"), StructDefs, ParseWarnings);
	LoadArrayField(JobRoot, TEXT("EnumsToCreate"), EnumDefs, ParseWarnings);

	{
		const TSharedPtr<FJsonObject>* GraphObjPtr = nullptr;
		if (JobRoot->TryGetObjectField(TEXT("GraphSpec"), GraphObjPtr) && GraphObjPtr && GraphObjPtr->IsValid())
		{
			if (FJsonObjectConverter::JsonObjectToUStruct(
					GraphObjPtr->ToSharedRef(),
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

	// External GraphSpec override.
	FSOTS_BPGenGraphSpec ExternalGraphSpec;
	bool bHasExternalGraphSpec = false;
	if (!GraphPath.IsEmpty())
	{
		TSharedPtr<FJsonObject> GraphRoot;
		FString GraphError;
		if (!LoadJsonObject(GraphPath, GraphRoot, GraphError))
		{
			LogLine(GraphError);
			SetLogLines(RunLog);
			return FReply::Handled();
		}

		if (!FJsonObjectConverter::JsonObjectToUStruct(
				GraphRoot.ToSharedRef(),
				FSOTS_BPGenGraphSpec::StaticStruct(),
				&ExternalGraphSpec,
				/*CheckFlags=*/0,
				/*SkipFlags=*/0))
		{
			LogLine(TEXT("Failed to convert external GraphSpec JSON to FSOTS_BPGenGraphSpec."));
			SetLogLines(RunLog);
			return FReply::Handled();
		}

		bHasExternalGraphSpec = true;
	}

	for (const FString& Warning : ParseWarnings)
	{
		LogLine(FString::Printf(TEXT("Warning: %s"), *Warning));
	}

	// Execute job via builder (mirrors commandlet).
	int32 ErrorCount = 0;
	int32 StructsOk = 0;
	int32 EnumsOk = 0;

	LogLine(FString::Printf(TEXT("Creating %d struct(s) and %d enum(s)..."), StructDefs.Num(), EnumDefs.Num()));

	for (const FSOTS_BPGenStructDef& StructDef : StructDefs)
	{
		const FSOTS_BPGenAssetResult StructResult =
			USOTS_BPGenBuilder::CreateStructAssetFromDef(GEditor, StructDef);

		if (!StructResult.bSuccess)
		{
			LogLine(FString::Printf(TEXT("Struct FAILED '%s': %s"), *StructResult.AssetPath, *StructResult.Message));
			++ErrorCount;
		}
		else
		{
			LogLine(FString::Printf(TEXT("Struct OK: %s"), *StructResult.Message));
			++StructsOk;
		}
	}

	for (const FSOTS_BPGenEnumDef& EnumDef : EnumDefs)
	{
		const FSOTS_BPGenAssetResult EnumResult =
			USOTS_BPGenBuilder::CreateEnumAssetFromDef(GEditor, EnumDef);

		if (!EnumResult.bSuccess)
		{
			LogLine(FString::Printf(TEXT("Enum FAILED '%s': %s"), *EnumResult.AssetPath, *EnumResult.Message));
			++ErrorCount;
		}
		else
		{
			LogLine(FString::Printf(TEXT("Enum OK: %s"), *EnumResult.Message));
			++EnumsOk;
		}
	}

	LogLine(FString::Printf(TEXT("Applying function skeleton: %s in %s"),
		*FunctionDef.FunctionName.ToString(),
		*FunctionDef.TargetBlueprintPath));

	const FSOTS_BPGenApplyResult SkeletonResult =
		USOTS_BPGenBuilder::ApplyFunctionSkeleton(GEditor, FunctionDef);

	if (!SkeletonResult.bSuccess)
	{
		LogLine(FString::Printf(TEXT("Function skeleton FAILED: %s"), *SkeletonResult.ErrorMessage));
		++ErrorCount;
	}
	else
	{
		for (const FString& Warn : SkeletonResult.Warnings)
		{
			LogLine(FString::Printf(TEXT("Function skeleton warning: %s"), *Warn));
		}
	}

	// Graph spec application.
	const bool bHasGraphSpec = bHasExternalGraphSpec || bHasInlineGraphSpec;
	if (bHasGraphSpec)
	{
		const FSOTS_BPGenGraphSpec& GraphSpecToUse = bHasExternalGraphSpec ? ExternalGraphSpec : InlineGraphSpec;
		LogLine(FString::Printf(TEXT("Applying graph spec with %d node(s) and %d link(s)."),
			GraphSpecToUse.Nodes.Num(),
			GraphSpecToUse.Links.Num()));

		const FSOTS_BPGenApplyResult GraphResult =
			USOTS_BPGenBuilder::ApplyGraphSpecToFunction(GEditor, FunctionDef, GraphSpecToUse);

		if (!GraphResult.bSuccess)
		{
			LogLine(FString::Printf(TEXT("Graph apply FAILED: %s"), *GraphResult.ErrorMessage));
			++ErrorCount;
		}
		else
		{
			for (const FString& Warn : GraphResult.Warnings)
			{
				LogLine(FString::Printf(TEXT("Graph apply warning: %s"), *Warn));
			}
		}
	}
	else
	{
		LogLine(TEXT("No GraphSpec provided; only function skeleton applied."));
	}

	if (ErrorCount > 0)
	{
		LogLine(FString::Printf(TEXT("BPGen run finished with %d error(s)."), ErrorCount));
	}
	else
	{
		LogLine(FString::Printf(
			TEXT("BPGen run finished successfully. Structs: %d ok, Enums: %d ok, Function: %s"),
			StructsOk, EnumsOk, *FunctionDef.FunctionName.ToString()));
	}

	SetLogLines(RunLog);
	return FReply::Handled();
}
