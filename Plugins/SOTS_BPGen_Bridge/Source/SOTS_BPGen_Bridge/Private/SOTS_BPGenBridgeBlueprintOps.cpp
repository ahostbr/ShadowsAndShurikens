#include "SOTS_BPGenBridgeBlueprintOps.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_CustomEvent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"

#include "Misc/PackageName.h"
#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace
{
	static FSOTS_BPGenBridgeBlueprintOpResult MakeError(const FString& ErrorCode, const FString& ErrorMessage)
	{
		FSOTS_BPGenBridgeBlueprintOpResult R;
		R.bOk = false;
		R.ErrorCode = ErrorCode;
		R.Errors.Add(ErrorMessage);
		R.Result = MakeShared<FJsonObject>();
		return R;
	}

	static FString NormalizeLongPackagePath(const FString& InPath)
	{
		FString Path = InPath;
		Path.TrimStartAndEndInline();
		if (Path.IsEmpty())
		{
			return FString();
		}

		if (Path.StartsWith(TEXT("/Game")) || Path.StartsWith(TEXT("/Engine")) || Path.StartsWith(TEXT("/Script")))
		{
			return Path;
		}

		if (Path.StartsWith(TEXT("/")))
		{
			return FString(TEXT("/Game")) + Path;
		}

		return FString(TEXT("/Game/")) + Path;
	}

	static FString NormalizeAssetObjectPath(const FString& InAssetPath)
	{
		FString Path = NormalizeLongPackagePath(InAssetPath);
		if (Path.IsEmpty())
		{
			return FString();
		}
		if (Path.Contains(TEXT(".")))
		{
			return Path;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(Path);
		if (AssetName.IsEmpty())
		{
			return Path;
		}

		return FString::Printf(TEXT("%s.%s"), *Path, *AssetName);
	}

	static UBlueprint* LoadBlueprintByPath(const FString& BlueprintName, FString& OutNormalizedObjectPath, FSOTS_BPGenBridgeBlueprintOpResult& OutError)
	{
		OutNormalizedObjectPath = NormalizeAssetObjectPath(BlueprintName);
		if (OutNormalizedObjectPath.IsEmpty())
		{
			OutError = MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("Missing blueprint_name"));
			return nullptr;
		}

		UObject* Obj = UEditorAssetLibrary::LoadAsset(OutNormalizedObjectPath);
		UBlueprint* BP = Cast<UBlueprint>(Obj);
		if (!BP)
		{
			OutError = MakeError(TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Failed to load Blueprint: %s"), *OutNormalizedObjectPath));
			return nullptr;
		}
		return BP;
	}

	static bool ResolveClassByName(const FString& ClassDescriptor, UClass*& OutClass)
	{
		OutClass = nullptr;
		FString Clean = ClassDescriptor;
		Clean.TrimStartAndEndInline();
		if (Clean.IsEmpty())
		{
			return false;
		}

		// If a full object path is provided, attempt load.
		if (Clean.StartsWith(TEXT("/")))
		{
			UClass* Loaded = LoadObject<UClass>(nullptr, *Clean);
			if (Loaded)
			{
				OutClass = Loaded;
				return true;
			}
		}

		// Strip any package qualifiers.
		int32 DotIndex = INDEX_NONE;
		if (Clean.FindLastChar('.', DotIndex))
		{
			Clean = Clean.Mid(DotIndex + 1);
		}

		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			if (!Class)
			{
				continue;
			}
			if (Class->GetName().Equals(Clean, ESearchCase::IgnoreCase))
			{
				OutClass = Class;
				return true;
			}
			const FString Display = Class->GetDisplayNameText().ToString();
			if (Display.Equals(Clean, ESearchCase::IgnoreCase))
			{
				OutClass = Class;
				return true;
			}
		}

		return false;
	}

	static FString GetBlueprintStatusString(const UBlueprint* BP)
	{
		if (!BP)
		{
			return TEXT("Unknown");
		}
		switch (BP->Status)
		{
		case BS_Unknown:
			return TEXT("Unknown");
		case BS_Dirty:
			return TEXT("Dirty");
		case BS_Error:
			return TEXT("Error");
		case BS_UpToDate:
			return TEXT("UpToDate");
		default:
			return TEXT("Other");
		}
	}

	static FString GetNodeTypeString(const UEdGraphNode* Node)
	{
		if (!Node)
		{
			return TEXT("Unknown");
		}
		if (Cast<UK2Node_CustomEvent>(Node))
		{
			return TEXT("CustomEvent");
		}
		return Node->GetClass()->GetName();
	}

	static bool ExportSupportedDefaultProperty(UObject* CDO, FProperty* Prop, FString& OutValue)
	{
		OutValue.Reset();
		if (!CDO || !Prop)
		{
			return false;
		}

		const bool bSupported =
			Prop->IsA<FBoolProperty>() ||
			Prop->IsA<FNumericProperty>() ||
			Prop->IsA<FStrProperty>() ||
			Prop->IsA<FNameProperty>() ||
			Prop->IsA<FTextProperty>();

		if (!bSupported)
		{
			return false;
		}

		void* PropData = Prop->ContainerPtrToValuePtr<void>(CDO);
		if (!PropData)
		{
			return false;
		}

		Prop->ExportTextItem_Direct(OutValue, PropData, nullptr, CDO, PPF_None);
		return true;
	}

	static bool SetObjectPropertyByImportText(UObject* Target, const FString& PropertyName, const FString& ImportText, FString& OutError)
	{
		OutError.Reset();
		if (!Target)
		{
			OutError = TEXT("Target is null");
			return false;
		}

		FString PName = PropertyName;
		PName.TrimStartAndEndInline();
		if (PName.IsEmpty())
		{
			OutError = TEXT("property_name is empty");
			return false;
		}

		FProperty* Prop = FindFProperty<FProperty>(Target->GetClass(), *PName);
		if (!Prop)
		{
			OutError = FString::Printf(TEXT("Property not found: %s"), *PName);
			return false;
		}

		void* PropData = Prop->ContainerPtrToValuePtr<void>(Target);
		if (!PropData)
		{
			OutError = FString::Printf(TEXT("Property data missing: %s"), *PName);
			return false;
		}

		Prop->ImportText_Direct(*ImportText, PropData, Target, PPF_None);
		return true;
	}
}

namespace SOTS_BPGenBridgeBlueprintOps
{
	FSOTS_BPGenBridgeBlueprintOpResult GetInfo(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		bool bIncludeDefaults = true;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetBoolField(TEXT("include_class_defaults"), bIncludeDefaults);
		}

		FSOTS_BPGenBridgeBlueprintOpResult Err;
		FString NormalizedObjectPath;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, NormalizedObjectPath, Err);
		if (!BP)
		{
			return Err;
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("name"), BP->GetName());
		ResultObj->SetStringField(TEXT("object_path"), NormalizedObjectPath);
		ResultObj->SetStringField(TEXT("parent_class"), BP->ParentClass ? BP->ParentClass->GetName() : TEXT(""));
		ResultObj->SetStringField(TEXT("parent_class_path"), BP->ParentClass ? BP->ParentClass->GetPathName() : TEXT(""));
		ResultObj->SetStringField(TEXT("status"), GetBlueprintStatusString(BP));
		ResultObj->SetBoolField(TEXT("is_compiled"), BP->Status != BS_Unknown && BP->Status != BS_Dirty);

		// Variables
		TArray<TSharedPtr<FJsonValue>> Vars;
		for (const FBPVariableDescription& Var : BP->NewVariables)
		{
			TSharedPtr<FJsonObject> V = MakeShared<FJsonObject>();
			V->SetStringField(TEXT("name"), Var.VarName.ToString());
			V->SetStringField(TEXT("pin_category"), Var.VarType.PinCategory.ToString());
			V->SetStringField(TEXT("pin_sub_category"), Var.VarType.PinSubCategory.ToString());
			V->SetBoolField(TEXT("is_array"), Var.VarType.IsArray());
			Vars.Add(MakeShared<FJsonValueObject>(V));
		}
		ResultObj->SetArrayField(TEXT("variables"), Vars);

		// Components (simple list)
		TArray<TSharedPtr<FJsonValue>> Components;
		if (BP->SimpleConstructionScript)
		{
			for (USCS_Node* Node : BP->SimpleConstructionScript->GetAllNodes())
			{
				if (!Node)
				{
					continue;
				}
				TSharedPtr<FJsonObject> C = MakeShared<FJsonObject>();
				C->SetStringField(TEXT("component_name"), Node->GetVariableName().ToString());
				C->SetStringField(TEXT("component_type"), Node->ComponentClass ? Node->ComponentClass->GetName() : TEXT("Unknown"));
				C->SetStringField(TEXT("parent_name"), Node->ParentComponentOrVariableName.ToString());
				Components.Add(MakeShared<FJsonValueObject>(C));
			}
		}
		ResultObj->SetArrayField(TEXT("components"), Components);

		// Function graphs
		TArray<TSharedPtr<FJsonValue>> Functions;
		for (UEdGraph* Graph : BP->FunctionGraphs)
		{
			if (!Graph)
			{
				continue;
			}
			TSharedPtr<FJsonObject> F = MakeShared<FJsonObject>();
			F->SetStringField(TEXT("name"), Graph->GetName());
			F->SetStringField(TEXT("guid"), Graph->GraphGuid.ToString(EGuidFormats::DigitsWithHyphensInBraces));
			F->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
			Functions.Add(MakeShared<FJsonValueObject>(F));
		}
		ResultObj->SetArrayField(TEXT("functions"), Functions);

		// Class defaults (supported-only; capped)
		ResultObj->SetBoolField(TEXT("class_defaults_included"), bIncludeDefaults);
		if (bIncludeDefaults && BP->GeneratedClass)
		{
			UObject* CDO = BP->GeneratedClass->GetDefaultObject();
			TArray<TSharedPtr<FJsonValue>> Defaults;
			int32 Added = 0;
			const int32 MaxDefaults = 256;
			if (CDO)
			{
				for (TFieldIterator<FProperty> It(CDO->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
				{
					if (Added >= MaxDefaults)
					{
						break;
					}
					FProperty* Prop = *It;
					FString Value;
					if (!ExportSupportedDefaultProperty(CDO, Prop, Value))
					{
						continue;
					}
					TSharedPtr<FJsonObject> P = MakeShared<FJsonObject>();
					P->SetStringField(TEXT("name"), Prop->GetName());
					P->SetStringField(TEXT("type"), Prop->GetClass()->GetName());
					P->SetStringField(TEXT("value"), Value);
					Defaults.Add(MakeShared<FJsonValueObject>(P));
					Added++;
				}
			}
			ResultObj->SetArrayField(TEXT("class_defaults"), Defaults);
			ResultObj->SetBoolField(TEXT("class_defaults_truncated"), Added >= MaxDefaults);
			ResultObj->SetBoolField(TEXT("class_defaults_supported_only"), true);
		}

		FSOTS_BPGenBridgeBlueprintOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeBlueprintOpResult GetProperty(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString PropertyName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("property_name"), PropertyName);
		}

		FSOTS_BPGenBridgeBlueprintOpResult Err;
		FString NormalizedObjectPath;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, NormalizedObjectPath, Err);
		if (!BP)
		{
			return Err;
		}
		if (PropertyName.TrimStartAndEnd().IsEmpty())
		{
			return MakeError(TEXT("ERR_INVALID_PARAMS"), TEXT("Missing property_name"));
		}

		if (!BP->GeneratedClass)
		{
			return MakeError(TEXT("ERR_BLUEPRINT_NO_GENERATED_CLASS"), TEXT("Blueprint has no GeneratedClass"));
		}

		UObject* CDO = BP->GeneratedClass->GetDefaultObject();
		if (!CDO)
		{
			return MakeError(TEXT("ERR_BLUEPRINT_NO_CDO"), TEXT("Failed to get class default object"));
		}

		FString PName = PropertyName;
		PName.TrimStartAndEndInline();
		FProperty* Prop = FindFProperty<FProperty>(CDO->GetClass(), *PName);
		if (!Prop)
		{
			return MakeError(TEXT("PROP_NOT_FOUND"), FString::Printf(TEXT("Property not found: %s"), *PName));
		}

		void* PropData = Prop->ContainerPtrToValuePtr<void>(CDO);
		if (!PropData)
		{
			return MakeError(TEXT("PROP_DATA_MISSING"), FString::Printf(TEXT("Property data missing: %s"), *PName));
		}

		FString Value;
		Prop->ExportTextItem_Direct(Value, PropData, nullptr, CDO, PPF_None);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetStringField(TEXT("property_name"), PName);
		ResultObj->SetStringField(TEXT("property_type"), Prop->GetClass()->GetName());
		ResultObj->SetStringField(TEXT("value"), Value);

		FSOTS_BPGenBridgeBlueprintOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeBlueprintOpResult SetProperty(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString PropertyName;
		FString PropertyValue;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("property_name"), PropertyName);
			Params->TryGetStringField(TEXT("property_value"), PropertyValue);
		}

		FSOTS_BPGenBridgeBlueprintOpResult Err;
		FString NormalizedObjectPath;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, NormalizedObjectPath, Err);
		if (!BP)
		{
			return Err;
		}

		if (!BP->GeneratedClass)
		{
			return MakeError(TEXT("ERR_BLUEPRINT_NO_GENERATED_CLASS"), TEXT("Blueprint has no GeneratedClass"));
		}

		UObject* CDO = BP->GeneratedClass->GetDefaultObject();
		if (!CDO)
		{
			return MakeError(TEXT("ERR_BLUEPRINT_NO_CDO"), TEXT("Failed to get class default object"));
		}

		FString Error;
		if (!SetObjectPropertyByImportText(CDO, PropertyName, PropertyValue, Error))
		{
			return MakeError(TEXT("PROP_SET_FAILED"), Error);
		}

		FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetStringField(TEXT("property_name"), PropertyName);
		ResultObj->SetStringField(TEXT("property_value"), PropertyValue);

		FSOTS_BPGenBridgeBlueprintOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeBlueprintOpResult Reparent(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString NewParentClass;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("new_parent_class"), NewParentClass);
		}

		FSOTS_BPGenBridgeBlueprintOpResult Err;
		FString NormalizedObjectPath;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, NormalizedObjectPath, Err);
		if (!BP)
		{
			return Err;
		}

		UClass* Resolved = nullptr;
		if (!ResolveClassByName(NewParentClass, Resolved) || !Resolved)
		{
			return MakeError(TEXT("BLUEPRINT_INVALID_PARENT"), FString::Printf(TEXT("Parent class not found: %s"), *NewParentClass));
		}

		const FString OldParent = BP->ParentClass ? BP->ParentClass->GetName() : TEXT("");
		const bool bReparented = FKismetEditorUtilities::ReparentBlueprint(BP, Resolved);
		if (!bReparented)
		{
			return MakeError(TEXT("BLUEPRINT_REPARENT_FAILED"), FString::Printf(TEXT("Failed to reparent Blueprint to: %s"), *Resolved->GetName()));
		}

		FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetStringField(TEXT("old_parent_class"), OldParent);
		ResultObj->SetStringField(TEXT("new_parent_class"), BP->ParentClass ? BP->ParentClass->GetName() : TEXT(""));

		FSOTS_BPGenBridgeBlueprintOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeBlueprintOpResult ListCustomEvents(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
		}

		FSOTS_BPGenBridgeBlueprintOpResult Err;
		FString NormalizedObjectPath;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, NormalizedObjectPath, Err);
		if (!BP)
		{
			return Err;
		}

		TArray<TSharedPtr<FJsonValue>> Events;
		TArray<UEdGraph*> AllGraphs;
		BP->GetAllGraphs(AllGraphs);

		for (UEdGraph* Graph : AllGraphs)
		{
			if (!Graph)
			{
				continue;
			}

			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node))
				{
					TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>();
					E->SetStringField(TEXT("event_name"), CustomEvent->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
					E->SetStringField(TEXT("graph"), Graph->GetName());
					E->SetStringField(TEXT("node_guid"), Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphensInBraces));
					Events.Add(MakeShared<FJsonValueObject>(E));
				}
			}
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetArrayField(TEXT("events"), Events);
		ResultObj->SetNumberField(TEXT("count"), Events.Num());

		FSOTS_BPGenBridgeBlueprintOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeBlueprintOpResult SummarizeEventGraph(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		int32 MaxNodes = 200;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			double MaxNodesValue = static_cast<double>(MaxNodes);
			if (Params->TryGetNumberField(TEXT("max_nodes"), MaxNodesValue))
			{
				MaxNodes = static_cast<int32>(MaxNodesValue);
			}
		}

		FSOTS_BPGenBridgeBlueprintOpResult Err;
		FString NormalizedObjectPath;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, NormalizedObjectPath, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* EventGraph = nullptr;
		if (BP->UbergraphPages.Num() > 0)
		{
			EventGraph = BP->UbergraphPages[0];
		}
		if (!EventGraph)
		{
			return MakeError(TEXT("ERR_NO_EVENT_GRAPH"), TEXT("Blueprint has no Event Graph"));
		}

		TMap<FString, int32> TypeCounts;
		TArray<TSharedPtr<FJsonValue>> NodeList;
		int32 Processed = 0;
		MaxNodes = FMath::Max(0, MaxNodes);

		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			const FString TypeStr = GetNodeTypeString(Node);
			TypeCounts.FindOrAdd(TypeStr) += 1;

			if (MaxNodes > 0 && Processed < MaxNodes)
			{
				TSharedPtr<FJsonObject> N = MakeShared<FJsonObject>();
				N->SetStringField(TEXT("type"), TypeStr);
				N->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
				N->SetStringField(TEXT("node_guid"), Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphensInBraces));
				NodeList.Add(MakeShared<FJsonValueObject>(N));
				Processed++;
			}
		}

		FString Summary;
		Summary += FString::Printf(TEXT("Event Graph Summary (%s)\n"), *BP->GetName());
		Summary += FString::Printf(TEXT("Total Nodes: %d\n"), EventGraph->Nodes.Num());
		Summary += TEXT("Node Types:\n");
		for (const TPair<FString, int32>& Pair : TypeCounts)
		{
			Summary += FString::Printf(TEXT("  %s: %d\n"), *Pair.Key, Pair.Value);
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetStringField(TEXT("summary"), Summary);
		ResultObj->SetArrayField(TEXT("nodes"), NodeList);
		ResultObj->SetBoolField(TEXT("nodes_truncated"), MaxNodes > 0 && EventGraph->Nodes.Num() > MaxNodes);
		ResultObj->SetNumberField(TEXT("total_nodes"), EventGraph->Nodes.Num());
		ResultObj->SetNumberField(TEXT("max_nodes"), MaxNodes);

		FSOTS_BPGenBridgeBlueprintOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}
}
