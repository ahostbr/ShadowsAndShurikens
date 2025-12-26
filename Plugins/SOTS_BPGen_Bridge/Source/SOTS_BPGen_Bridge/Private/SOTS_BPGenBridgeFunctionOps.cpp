#include "SOTS_BPGenBridgeFunctionOps.h"

#include "EditorAssetLibrary.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

#include "UObject/UnrealType.h"

#include "Math/Color.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "Math/Vector4.h"

#include "Misc/PackageName.h"

#include "SOTS_BPGenBridgePrivateHelpers.h"

namespace
{
	static FSOTS_BPGenBridgeFunctionOpResult MakeOpError(const FString& ErrorCode, const FString& ErrorMessage)
	{
		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = false;
		R.ErrorCode = ErrorCode;
		R.Errors.Add(ErrorMessage);
		R.Result = MakeShared<FJsonObject>();
		return R;
	}

	static bool TryUpdateFunctionEntryExtraFlags(UK2Node_FunctionEntry* Entry, uint32 FlagMask, bool bSetFlag)
	{
		if (!Entry)
		{
			return false;
		}

		if (const FUInt32Property* P = FindFProperty<FUInt32Property>(Entry->GetClass(), TEXT("ExtraFlags")))
		{
			uint32 Value = P->GetPropertyValue_InContainer(Entry);
			Value = bSetFlag ? (Value | FlagMask) : (Value & ~FlagMask);
			const_cast<FUInt32Property*>(P)->SetPropertyValue_InContainer(Entry, Value);
			return true;
		}

		if (const FIntProperty* P = FindFProperty<FIntProperty>(Entry->GetClass(), TEXT("ExtraFlags")))
		{
			uint32 Value = static_cast<uint32>(P->GetPropertyValue_InContainer(Entry));
			Value = bSetFlag ? (Value | FlagMask) : (Value & ~FlagMask);
			const_cast<FIntProperty*>(P)->SetPropertyValue_InContainer(Entry, static_cast<int32>(Value));
			return true;
		}

		return false;
	}

	static UBlueprint* LoadBlueprintByPath(const FString& BlueprintName, FString& OutNormalizedObjectPath, FSOTS_BPGenBridgeFunctionOpResult& OutError)
	{
		OutNormalizedObjectPath = SOTS_BPGenBridgePrivate::NormalizeAssetObjectPath(BlueprintName);
		if (OutNormalizedObjectPath.IsEmpty())
		{
			OutError = MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("Missing blueprint_name"));
			return nullptr;
		}

		UObject* Obj = UEditorAssetLibrary::LoadAsset(OutNormalizedObjectPath);
		UBlueprint* BP = Cast<UBlueprint>(Obj);
		if (!BP)
		{
			OutError = MakeOpError(TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Failed to load Blueprint: %s"), *OutNormalizedObjectPath));
			return nullptr;
		}
		return BP;
	}

	static bool FindUserFunctionGraph(UBlueprint* Blueprint, const FString& FunctionName, UEdGraph*& OutGraph)
	{
		OutGraph = nullptr;
		if (!Blueprint)
		{
			return false;
		}
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
			{
				OutGraph = Graph;
				return true;
			}
		}
		return false;
	}

	static UK2Node_FunctionEntry* FindFunctionEntry(UEdGraph* Graph)
	{
		if (!Graph)
		{
			return nullptr;
		}
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
			{
				return Entry;
			}
		}
		return nullptr;
	}

	static UK2Node_FunctionResult* FindOrCreateResultNode(UBlueprint* Blueprint, UEdGraph* Graph)
	{
		if (!Graph)
		{
			return nullptr;
		}
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UK2Node_FunctionResult* Result = Cast<UK2Node_FunctionResult>(Node))
			{
				return Result;
			}
		}

		FGraphNodeCreator<UK2Node_FunctionResult> Creator(*Graph);
		UK2Node_FunctionResult* NewNode = Creator.CreateNode();
		Creator.Finalize();

		if (Blueprint)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		}
		return NewNode;
	}

	static const UStruct* ResolveFunctionScopeStruct(UBlueprint* Blueprint, UEdGraph* FunctionGraph)
	{
		if (!Blueprint || !FunctionGraph)
		{
			return nullptr;
		}

		auto FindScope = [FunctionGraph](UClass* InClass) -> const UStruct*
		{
			if (!InClass)
			{
				return nullptr;
			}
			return InClass->FindFunctionByName(FunctionGraph->GetFName());
		};

		if (const UStruct* Scope = FindScope(Blueprint->SkeletonGeneratedClass))
		{
			return Scope;
		}
		if (const UStruct* Scope = FindScope(Blueprint->GeneratedClass))
		{
			return Scope;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (const UStruct* Scope = FindScope(Blueprint->SkeletonGeneratedClass))
		{
			return Scope;
		}
		return FindScope(Blueprint->GeneratedClass);
	}

	static FString DescribePinType(const FEdGraphPinType& PinType)
	{
		auto DescribeCategory = [](const FName& Category, const FName& SubCategory, UObject* SubObject) -> FString
		{
			if (Category == UEdGraphSchema_K2::PC_Exec) return TEXT("exec");
			if (Category == UEdGraphSchema_K2::PC_Boolean) return TEXT("bool");
			if (Category == UEdGraphSchema_K2::PC_Byte)
			{
				if (SubObject)
				{
					return FString::Printf(TEXT("enum:%s"), *SubObject->GetName());
				}
				return TEXT("byte");
			}
			if (Category == UEdGraphSchema_K2::PC_Int) return TEXT("int");
			if (Category == UEdGraphSchema_K2::PC_Int64) return TEXT("int64");
			if (Category == UEdGraphSchema_K2::PC_Float) return TEXT("float");
			if (Category == UEdGraphSchema_K2::PC_Double) return TEXT("double");
			if (Category == UEdGraphSchema_K2::PC_String) return TEXT("string");
			if (Category == UEdGraphSchema_K2::PC_Name) return TEXT("name");
			if (Category == UEdGraphSchema_K2::PC_Text) return TEXT("text");
			if (Category == UEdGraphSchema_K2::PC_Struct && SubObject)
			{
				return FString::Printf(TEXT("struct:%s"), *SubObject->GetName());
			}
			if (Category == UEdGraphSchema_K2::PC_Object && SubObject)
			{
				return FString::Printf(TEXT("object:%s"), *SubObject->GetName());
			}
			if (Category == UEdGraphSchema_K2::PC_Class && SubObject)
			{
				return FString::Printf(TEXT("class:%s"), *SubObject->GetName());
			}
			return Category.ToString();
		};

		FString Base = DescribeCategory(PinType.PinCategory, PinType.PinSubCategory, PinType.PinSubCategoryObject.Get());
		if (PinType.ContainerType == EPinContainerType::Array)
		{
			return FString::Printf(TEXT("array<%s>"), *Base);
		}
		if (PinType.ContainerType == EPinContainerType::Set)
		{
			return FString::Printf(TEXT("set<%s>"), *Base);
		}
		if (PinType.ContainerType == EPinContainerType::Map)
		{
			FString ValueDesc = DescribeCategory(PinType.PinValueType.TerminalCategory, PinType.PinValueType.TerminalSubCategory, PinType.PinValueType.TerminalSubCategoryObject.Get());
			return FString::Printf(TEXT("map<%s,%s>"), *Base, *ValueDesc);
		}
		return Base;
	}

	static bool TryResolveClassDescriptor(const FString& Descriptor, UClass*& OutClass)
	{
		OutClass = nullptr;
		FString Clean = Descriptor;
		Clean.TrimStartAndEndInline();
		if (Clean.IsEmpty())
		{
			return false;
		}

		if (Clean.Contains(TEXT("/")))
		{
			OutClass = LoadObject<UClass>(nullptr, *Clean);
			return OutClass != nullptr;
		}

		OutClass = FindFirstObject<UClass>(*Clean);
		if (OutClass)
		{
			return true;
		}

		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* C = *It;
			if (!C)
			{
				continue;
			}
			if (C->GetName().Equals(Clean, ESearchCase::IgnoreCase))
			{
				OutClass = C;
				return true;
			}
		}
		return false;
	}

	static bool TryResolveStructDescriptor(const FString& Descriptor, UScriptStruct*& OutStruct)
	{
		OutStruct = nullptr;
		FString Clean = Descriptor;
		Clean.TrimStartAndEndInline();
		if (Clean.IsEmpty())
		{
			return false;
		}
		if (Clean.Contains(TEXT("/")))
		{
			OutStruct = LoadObject<UScriptStruct>(nullptr, *Clean);
			return OutStruct != nullptr;
		}
		OutStruct = FindFirstObject<UScriptStruct>(*Clean);
		if (OutStruct)
		{
			return true;
		}
		for (TObjectIterator<UScriptStruct> It; It; ++It)
		{
			UScriptStruct* S = *It;
			if (!S)
			{
				continue;
			}
			if (S->GetName().Equals(Clean, ESearchCase::IgnoreCase))
			{
				OutStruct = S;
				return true;
			}
		}
		return false;
	}

	static bool ParseTypeDescriptor(const FString& TypeDesc, FEdGraphPinType& OutType, FString& OutError)
	{
		OutError.Reset();
		OutType.ResetToDefaults();

		FString Trimmed = TypeDesc;
		Trimmed.TrimStartAndEndInline();
		const FString Lower = Trimmed.ToLower();

		if (Lower.StartsWith(TEXT("array<")) && Lower.EndsWith(TEXT(">")))
		{
			FString Inner = Trimmed.Mid(6, Trimmed.Len() - 7);
			Inner.TrimStartAndEndInline();
			FEdGraphPinType InnerType;
			FString Err;
			if (!ParseTypeDescriptor(Inner, InnerType, Err))
			{
				OutError = Err;
				return false;
			}
			OutType = InnerType;
			OutType.ContainerType = EPinContainerType::Array;
			return true;
		}

		if (Lower == TEXT("exec")) { OutType.PinCategory = UEdGraphSchema_K2::PC_Exec; return true; }
		if (Lower == TEXT("bool")) { OutType.PinCategory = UEdGraphSchema_K2::PC_Boolean; return true; }
		if (Lower == TEXT("byte")) { OutType.PinCategory = UEdGraphSchema_K2::PC_Byte; return true; }
		if (Lower == TEXT("int") || Lower == TEXT("int32")) { OutType.PinCategory = UEdGraphSchema_K2::PC_Int; return true; }
		if (Lower == TEXT("int64")) { OutType.PinCategory = UEdGraphSchema_K2::PC_Int64; return true; }
		if (Lower == TEXT("float") || Lower == TEXT("real")) { OutType.PinCategory = UEdGraphSchema_K2::PC_Float; return true; }
		if (Lower == TEXT("double")) { OutType.PinCategory = UEdGraphSchema_K2::PC_Double; return true; }
		if (Lower == TEXT("string")) { OutType.PinCategory = UEdGraphSchema_K2::PC_String; return true; }
		if (Lower == TEXT("name")) { OutType.PinCategory = UEdGraphSchema_K2::PC_Name; return true; }
		if (Lower == TEXT("text")) { OutType.PinCategory = UEdGraphSchema_K2::PC_Text; return true; }

		if (Lower == TEXT("vector"))
		{
			OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
			return true;
		}
		if (Lower == TEXT("vector2d"))
		{
			OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get();
			return true;
		}
		if (Lower == TEXT("vector4"))
		{
			OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutType.PinSubCategoryObject = TBaseStructure<FVector4>::Get();
			return true;
		}
		if (Lower == TEXT("rotator"))
		{
			OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
			return true;
		}
		if (Lower == TEXT("transform"))
		{
			OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
			return true;
		}
		if (Lower == TEXT("color"))
		{
			OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutType.PinSubCategoryObject = TBaseStructure<FColor>::Get();
			return true;
		}

		if (Lower.StartsWith(TEXT("object:")))
		{
			const FString ClassDesc = Trimmed.Mid(7);
			UClass* C = nullptr;
			if (!TryResolveClassDescriptor(ClassDesc, C) || !C)
			{
				OutError = FString::Printf(TEXT("Class not found: %s"), *ClassDesc);
				return false;
			}
			OutType.PinCategory = UEdGraphSchema_K2::PC_Object;
			OutType.PinSubCategoryObject = C;
			return true;
		}

		if (Lower.StartsWith(TEXT("class:")))
		{
			const FString ClassDesc = Trimmed.Mid(6);
			UClass* C = nullptr;
			if (!TryResolveClassDescriptor(ClassDesc, C) || !C)
			{
				OutError = FString::Printf(TEXT("Class not found: %s"), *ClassDesc);
				return false;
			}
			OutType.PinCategory = UEdGraphSchema_K2::PC_Class;
			OutType.PinSubCategoryObject = C;
			return true;
		}

		if (Lower.StartsWith(TEXT("struct:")))
		{
			const FString StructDesc = Trimmed.Mid(7);
			UScriptStruct* S = nullptr;
			if (!TryResolveStructDescriptor(StructDesc, S) || !S)
			{
				OutError = FString::Printf(TEXT("Struct not found: %s"), *StructDesc);
				return false;
			}
			OutType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			OutType.PinSubCategoryObject = S;
			return true;
		}

		OutError = FString::Printf(TEXT("Unknown type descriptor: %s"), *Trimmed);
		return false;
	}
}

namespace SOTS_BPGenBridgeFunctionOps
{
	FSOTS_BPGenBridgeFunctionOpResult List(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		TArray<TSharedPtr<FJsonValue>> Functions;
		for (UEdGraph* Graph : BP->FunctionGraphs)
		{
			if (!Graph)
			{
				continue;
			}
			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetStringField(TEXT("name"), Graph->GetName());
			Obj->SetStringField(TEXT("graph_guid"), Graph->GraphGuid.ToString());
			Obj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
			Functions.Add(MakeShared<FJsonValueObject>(Obj));
		}

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetArrayField(TEXT("functions"), Functions);
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult Get(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
		}

		FunctionName.TrimStartAndEndInline();
		if (FunctionName.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("get requires function_name"));
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("name"), Graph->GetName());
		R.Result->SetStringField(TEXT("graph_guid"), Graph->GraphGuid.ToString());
		R.Result->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult Delete(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
		}

		FunctionName.TrimStartAndEndInline();
		if (FunctionName.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("delete requires function_name"));
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		FBlueprintEditorUtils::RemoveGraph(BP, Graph, EGraphRemoveFlags::Recompile);

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		R.Result->SetBoolField(TEXT("success"), true);
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult ListParams(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
		}

		FunctionName.TrimStartAndEndInline();
		if (FunctionName.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("list_params requires function_name"));
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		TArray<TSharedPtr<FJsonValue>> ParamsOut;

		if (UK2Node_FunctionEntry* Entry = FindFunctionEntry(Graph))
		{
			for (UEdGraphPin* Pin : Entry->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Output && Pin->PinName != UEdGraphSchema_K2::PN_Then)
				{
					TSharedPtr<FJsonObject> P = MakeShared<FJsonObject>();
					P->SetStringField(TEXT("name"), Pin->GetFName().ToString());
					P->SetStringField(TEXT("direction"), TEXT("input"));
					P->SetStringField(TEXT("type"), DescribePinType(Pin->PinType));
					ParamsOut.Add(MakeShared<FJsonValueObject>(P));
				}
			}
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
			{
				for (UEdGraphPin* Pin : ResultNode->Pins)
				{
					if (Pin && Pin->Direction == EGPD_Input && Pin->PinName != UEdGraphSchema_K2::PN_Then)
					{
						const bool bIsReturn = (Pin->PinName == UEdGraphSchema_K2::PN_ReturnValue);
						TSharedPtr<FJsonObject> P = MakeShared<FJsonObject>();
						P->SetStringField(TEXT("name"), Pin->GetFName().ToString());
						P->SetStringField(TEXT("direction"), bIsReturn ? TEXT("return") : TEXT("out"));
						P->SetStringField(TEXT("type"), DescribePinType(Pin->PinType));
						ParamsOut.Add(MakeShared<FJsonValueObject>(P));
					}
				}
			}
		}

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetBoolField(TEXT("success"), true);
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		R.Result->SetArrayField(TEXT("parameters"), ParamsOut);
		R.Result->SetNumberField(TEXT("count"), ParamsOut.Num());
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult AddParam(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		FString ParamName;
		FString ParamType;
		FString Direction;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
			Params->TryGetStringField(TEXT("param_name"), ParamName);
			Params->TryGetStringField(TEXT("type"), ParamType);
			Params->TryGetStringField(TEXT("direction"), Direction);
		}

		FunctionName.TrimStartAndEndInline();
		ParamName.TrimStartAndEndInline();
		ParamType.TrimStartAndEndInline();
		Direction.TrimStartAndEndInline();
		if (FunctionName.IsEmpty() || ParamName.IsEmpty() || ParamType.IsEmpty() || Direction.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("add_param requires function_name, param_name, type, direction"));
		}

		const FString DirLower = Direction.ToLower();
		if (!(DirLower == TEXT("input") || DirLower == TEXT("out") || DirLower == TEXT("return")))
		{
			return MakeOpError(TEXT("PARAMETER_INVALID_DIRECTION"), TEXT("Invalid direction (expected input|out|return)"));
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		FEdGraphPinType PinType;
		FString TypeError;
		if (!ParseTypeDescriptor(ParamType, PinType, TypeError))
		{
			return MakeOpError(TEXT("PARAMETER_TYPE_INVALID"), TypeError);
		}

		UK2Node_FunctionEntry* Entry = FindFunctionEntry(Graph);
		if (!Entry)
		{
			return MakeOpError(TEXT("FUNCTION_ENTRY_NOT_FOUND"), TEXT("Function entry node not found"));
		}

		if (DirLower == TEXT("input"))
		{
			UEdGraphPin* NewPin = Entry->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output, false);
			if (!NewPin)
			{
				return MakeOpError(TEXT("PARAMETER_CREATE_FAILED"), TEXT("Failed to create input pin"));
			}
		}
		else
		{
			UK2Node_FunctionResult* ResultNode = FindOrCreateResultNode(BP, Graph);
			if (!ResultNode)
			{
				return MakeOpError(TEXT("FUNCTION_RESULT_CREATE_FAILED"), TEXT("Failed to resolve/create result node"));
			}
			const FName NewPinName = (DirLower == TEXT("return")) ? UEdGraphSchema_K2::PN_ReturnValue : FName(*ParamName);
			if (DirLower == TEXT("return"))
			{
				for (UEdGraphPin* P : ResultNode->Pins)
				{
					if (P && P->PinName == UEdGraphSchema_K2::PN_ReturnValue)
					{
						return MakeOpError(TEXT("PARAMETER_ALREADY_EXISTS"), TEXT("Return value already exists"));
					}
				}
			}
			UEdGraphPin* NewPin = ResultNode->CreateUserDefinedPin(NewPinName, PinType, EGPD_Input, false);
			if (!NewPin)
			{
				return MakeOpError(TEXT("PARAMETER_CREATE_FAILED"), TEXT("Failed to create result pin"));
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetBoolField(TEXT("success"), true);
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		R.Result->SetStringField(TEXT("param_name"), ParamName);
		R.Result->SetStringField(TEXT("direction"), DirLower);
		R.Result->SetStringField(TEXT("type"), ParamType);
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult RemoveParam(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		FString ParamName;
		FString Direction;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
			Params->TryGetStringField(TEXT("param_name"), ParamName);
			Params->TryGetStringField(TEXT("direction"), Direction);
		}

		FunctionName.TrimStartAndEndInline();
		ParamName.TrimStartAndEndInline();
		Direction.TrimStartAndEndInline();
		if (FunctionName.IsEmpty() || ParamName.IsEmpty() || Direction.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("remove_param requires function_name, param_name, direction"));
		}

		const FString DirLower = Direction.ToLower();
		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		bool bFound = false;
		if (DirLower == TEXT("input"))
		{
			if (UK2Node_FunctionEntry* Entry = FindFunctionEntry(Graph))
			{
				for (int32 i = Entry->Pins.Num() - 1; i >= 0; --i)
				{
					UEdGraphPin* P = Entry->Pins[i];
					if (P && P->Direction == EGPD_Output && P->PinName.ToString().Equals(ParamName, ESearchCase::IgnoreCase))
					{
						P->BreakAllPinLinks();
						Entry->Pins.RemoveAt(i);
						bFound = true;
						break;
					}
				}
			}
		}
		else
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
				{
					for (int32 i = ResultNode->Pins.Num() - 1; i >= 0; --i)
					{
						UEdGraphPin* P = ResultNode->Pins[i];
						if (!P || P->Direction != EGPD_Input)
						{
							continue;
						}
						bool bNameMatch = false;
						if (DirLower == TEXT("return"))
						{
							bNameMatch = (P->PinName == UEdGraphSchema_K2::PN_ReturnValue);
						}
						else
						{
							bNameMatch = P->PinName.ToString().Equals(ParamName, ESearchCase::IgnoreCase);
						}
						if (bNameMatch)
						{
							P->BreakAllPinLinks();
							ResultNode->Pins.RemoveAt(i);
							bFound = true;
							break;
						}
					}
					if (bFound)
					{
						break;
					}
				}
			}
		}

		if (!bFound)
		{
			return MakeOpError(TEXT("PARAMETER_NOT_FOUND"), FString::Printf(TEXT("Parameter not found: %s"), *ParamName));
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetBoolField(TEXT("success"), true);
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		R.Result->SetStringField(TEXT("param_name"), ParamName);
		R.Result->SetStringField(TEXT("direction"), DirLower);
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult UpdateParam(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		FString ParamName;
		FString Direction;
		FString NewType;
		FString NewName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
			Params->TryGetStringField(TEXT("param_name"), ParamName);
			Params->TryGetStringField(TEXT("direction"), Direction);
			Params->TryGetStringField(TEXT("new_type"), NewType);
			Params->TryGetStringField(TEXT("new_name"), NewName);
		}

		FunctionName.TrimStartAndEndInline();
		ParamName.TrimStartAndEndInline();
		Direction.TrimStartAndEndInline();
		NewType.TrimStartAndEndInline();
		NewName.TrimStartAndEndInline();
		if (FunctionName.IsEmpty() || ParamName.IsEmpty() || Direction.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("update_param requires function_name, param_name, direction"));
		}

		FEdGraphPinType NewPinType;
		bool bTypeChange = false;
		if (!NewType.IsEmpty())
		{
			FString ErrMsg;
			if (!ParseTypeDescriptor(NewType, NewPinType, ErrMsg))
			{
				return MakeOpError(TEXT("PARAMETER_TYPE_INVALID"), ErrMsg);
			}
			bTypeChange = true;
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		bool bModified = false;
		auto ApplyChanges = [&](UEdGraphPin* P)
		{
			if (!P)
			{
				return;
			}
			if (bTypeChange)
			{
				P->PinType = NewPinType;
			}
			if (!NewName.IsEmpty() && P->PinName.ToString() != NewName && P->PinName != UEdGraphSchema_K2::PN_ReturnValue)
			{
				P->PinName = FName(*NewName);
			}
			bModified = true;
		};

		const FString DirLower = Direction.ToLower();
		if (DirLower == TEXT("input"))
		{
			if (UK2Node_FunctionEntry* Entry = FindFunctionEntry(Graph))
			{
				for (UEdGraphPin* P : Entry->Pins)
				{
					if (P && P->Direction == EGPD_Output && P->PinName.ToString().Equals(ParamName, ESearchCase::IgnoreCase))
					{
						ApplyChanges(P);
						break;
					}
				}
			}
		}
		else
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
				{
					for (UEdGraphPin* P : ResultNode->Pins)
					{
						if (!P || P->Direction != EGPD_Input)
						{
							continue;
						}
						bool bMatch = false;
						if (DirLower == TEXT("return"))
						{
							bMatch = (P->PinName == UEdGraphSchema_K2::PN_ReturnValue);
						}
						else
						{
							bMatch = P->PinName.ToString().Equals(ParamName, ESearchCase::IgnoreCase);
						}
						if (bMatch)
						{
							ApplyChanges(P);
							break;
						}
					}
					if (bModified)
					{
						break;
					}
				}
			}
		}

		if (!bModified)
		{
			return MakeOpError(TEXT("PARAMETER_NOT_FOUND"), FString::Printf(TEXT("Parameter not found: %s"), *ParamName));
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetBoolField(TEXT("success"), true);
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		R.Result->SetStringField(TEXT("param_name"), ParamName);
		R.Result->SetStringField(TEXT("direction"), DirLower);
		if (!NewType.IsEmpty())	R.Result->SetStringField(TEXT("new_type"), NewType);
		if (!NewName.IsEmpty())	R.Result->SetStringField(TEXT("new_name"), NewName);
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult ListLocals(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
		}

		FunctionName.TrimStartAndEndInline();
		if (FunctionName.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("list_locals requires function_name"));
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		TArray<TSharedPtr<FJsonValue>> Locals;
		if (UK2Node_FunctionEntry* Entry = FindFunctionEntry(Graph))
		{
			for (const FBPVariableDescription& VarDesc : Entry->LocalVariables)
			{
				TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
				Obj->SetStringField(TEXT("name"), VarDesc.VarName.ToString());
				Obj->SetStringField(TEXT("type"), DescribePinType(VarDesc.VarType));
				Obj->SetStringField(TEXT("default_value"), VarDesc.DefaultValue);
				Obj->SetBoolField(TEXT("is_const"), VarDesc.VarType.bIsConst || ((VarDesc.PropertyFlags & CPF_BlueprintReadOnly) != 0));
				Obj->SetBoolField(TEXT("is_reference"), VarDesc.VarType.bIsReference);
				Locals.Add(MakeShared<FJsonValueObject>(Obj));
			}
		}

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetBoolField(TEXT("success"), true);
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		R.Result->SetArrayField(TEXT("locals"), Locals);
		R.Result->SetNumberField(TEXT("count"), Locals.Num());
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult AddLocal(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		FString VarName;
		FString VarType;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
			Params->TryGetStringField(TEXT("param_name"), VarName);
			Params->TryGetStringField(TEXT("type"), VarType);
		}

		FunctionName.TrimStartAndEndInline();
		VarName.TrimStartAndEndInline();
		VarType.TrimStartAndEndInline();
		if (FunctionName.IsEmpty() || VarName.IsEmpty() || VarType.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("add_local requires function_name, param_name, type"));
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		UK2Node_FunctionEntry* Entry = FindFunctionEntry(Graph);
		if (!Entry)
		{
			return MakeOpError(TEXT("FUNCTION_ENTRY_NOT_FOUND"), TEXT("Function entry node not found"));
		}

		for (const FBPVariableDescription& Local : Entry->LocalVariables)
		{
			if (Local.VarName.ToString().Equals(VarName, ESearchCase::IgnoreCase))
			{
				return MakeOpError(TEXT("VARIABLE_ALREADY_EXISTS"), FString::Printf(TEXT("Local variable already exists: %s"), *VarName));
			}
		}

		FEdGraphPinType PinType;
		FString TypeError;
		if (!ParseTypeDescriptor(VarType, PinType, TypeError))
		{
			return MakeOpError(TEXT("VARIABLE_TYPE_INVALID"), TypeError);
		}

		if (!FBlueprintEditorUtils::AddLocalVariable(BP, Graph, FName(*VarName), PinType, FString()))
		{
			return MakeOpError(TEXT("VARIABLE_CREATE_FAILED"), TEXT("Failed to add local variable"));
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetBoolField(TEXT("success"), true);
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		R.Result->SetStringField(TEXT("local_name"), VarName);
		R.Result->SetStringField(TEXT("type"), VarType);
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult RemoveLocal(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		FString VarName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
			Params->TryGetStringField(TEXT("param_name"), VarName);
		}

		FunctionName.TrimStartAndEndInline();
		VarName.TrimStartAndEndInline();
		if (FunctionName.IsEmpty() || VarName.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("remove_local requires function_name, param_name"));
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		FName VarFName(*VarName);
		UK2Node_FunctionEntry* Entry = nullptr;
		FBPVariableDescription* Existing = FBlueprintEditorUtils::FindLocalVariable(BP, Graph, VarFName, &Entry);
		if (!Existing || !Entry)
		{
			return MakeOpError(TEXT("VARIABLE_NOT_FOUND"), FString::Printf(TEXT("Local variable not found: %s"), *VarName));
		}

		if (const UStruct* Scope = ResolveFunctionScopeStruct(BP, Graph))
		{
			FBlueprintEditorUtils::RemoveLocalVariable(BP, Scope, VarFName);
		}
		else
		{
			Entry->Modify();
			for (int32 Index = 0; Index < Entry->LocalVariables.Num(); ++Index)
			{
				if (Entry->LocalVariables[Index].VarName == VarFName)
				{
					Entry->LocalVariables.RemoveAt(Index);
					break;
				}
			}
			FBlueprintEditorUtils::RemoveVariableNodes(BP, VarFName, true, Graph);
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		}

		FKismetEditorUtilities::CompileBlueprint(BP);

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetBoolField(TEXT("success"), true);
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		R.Result->SetStringField(TEXT("local_name"), VarName);
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult UpdateLocal(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		FString VarName;
		FString NewType;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
			Params->TryGetStringField(TEXT("param_name"), VarName);
			Params->TryGetStringField(TEXT("new_type"), NewType);
		}

		FunctionName.TrimStartAndEndInline();
		VarName.TrimStartAndEndInline();
		NewType.TrimStartAndEndInline();
		if (FunctionName.IsEmpty() || VarName.IsEmpty() || NewType.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("update_local requires function_name, param_name, new_type"));
		}

		FEdGraphPinType NewPinType;
		FString TypeError;
		if (!ParseTypeDescriptor(NewType, NewPinType, TypeError))
		{
			return MakeOpError(TEXT("VARIABLE_TYPE_INVALID"), TypeError);
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		FName VarFName(*VarName);
		UK2Node_FunctionEntry* Entry = nullptr;
		FBPVariableDescription* Existing = FBlueprintEditorUtils::FindLocalVariable(BP, Graph, VarFName, &Entry);
		if (!Existing || !Entry)
		{
			return MakeOpError(TEXT("VARIABLE_NOT_FOUND"), FString::Printf(TEXT("Local variable not found: %s"), *VarName));
		}

		Entry->Modify();
		Existing->VarType = NewPinType;
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetBoolField(TEXT("success"), true);
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		R.Result->SetStringField(TEXT("local_name"), VarName);
		R.Result->SetStringField(TEXT("new_type"), NewType);
		return R;
	}

	FSOTS_BPGenBridgeFunctionOpResult UpdateProperties(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString FunctionName;
		TSharedPtr<FJsonObject> Props;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
			if (Params->HasTypedField<EJson::Object>(TEXT("properties")))
			{
				Props = Params->GetObjectField(TEXT("properties"));
			}
		}

		FunctionName.TrimStartAndEndInline();
		if (FunctionName.IsEmpty())
		{
			return MakeOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("update_properties requires function_name"));
		}

		FSOTS_BPGenBridgeFunctionOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		UEdGraph* Graph = nullptr;
		if (!FindUserFunctionGraph(BP, FunctionName, Graph) || !Graph)
		{
			return MakeOpError(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: %s"), *FunctionName));
		}

		UK2Node_FunctionEntry* Entry = FindFunctionEntry(Graph);
		if (!Entry)
		{
			return MakeOpError(TEXT("FUNCTION_ENTRY_NOT_FOUND"), TEXT("Function entry node not found"));
		}

		bool bModified = false;
		if (Props.IsValid())
		{
			bool bCallInEditor = false;
			if (Props->TryGetBoolField(TEXT("CallInEditor"), bCallInEditor))
			{
				Entry->Modify();
				Entry->MetaData.bCallInEditor = bCallInEditor;
				bModified = true;
			}

			bool bPure = false;
			if (Props->TryGetBoolField(TEXT("BlueprintPure"), bPure))
			{
				Entry->Modify();
				if (TryUpdateFunctionEntryExtraFlags(Entry, FUNC_BlueprintPure, bPure))
				{
					bModified = true;
				}
			}

			FString Category;
			if (Props->TryGetStringField(TEXT("Category"), Category))
			{
				Category.TrimStartAndEndInline();
				if (!Category.IsEmpty())
				{
					Entry->Modify();
					Entry->MetaData.Category = FText::FromString(Category);
					bModified = true;
				}
			}
		}

		if (!bModified)
		{
			FSOTS_BPGenBridgeFunctionOpResult R;
			R.bOk = true;
			R.Result = MakeShared<FJsonObject>();
			R.Result->SetBoolField(TEXT("success"), true);
			R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
			R.Result->SetStringField(TEXT("function_name"), FunctionName);
			R.Warnings.Add(TEXT("No supported properties applied (supported: CallInEditor, BlueprintPure, Category)"));
			return R;
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);

		FSOTS_BPGenBridgeFunctionOpResult R;
		R.bOk = true;
		R.Result = MakeShared<FJsonObject>();
		R.Result->SetBoolField(TEXT("success"), true);
		R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
		R.Result->SetStringField(TEXT("function_name"), FunctionName);
		return R;
	}
}
