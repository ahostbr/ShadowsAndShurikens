#include "SOTS_BPGenBuilder.h"
#include "SOTS_BlueprintGen.h"
#include "SOTS_BPGenTypes.h"

#include "BlueprintGraphModule.h"
#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/UserDefinedEnum.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/StructureEditorUtils.h"
#include "UObject/SavePackage.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"

#include "K2Node.h"
#include "K2Node_AssignDelegate.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_EnumLiteral.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Knot.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_AddComponent.h"
#include "K2Node_AddComponentByClass.h"
#include "K2Node_MultiGate.h"
#include "K2Node_MacroInstance.h"
#include "GameplayTagsK2Node_SwitchGameplayTag.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_Event.h"
#include "K2Node_Select.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_SwitchInteger.h"
#include "K2Node_SwitchName.h"
#include "K2Node_SwitchString.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "UObject/UnrealType.h"
#include "Math/Transform.h"
#include "UObject/NoExportTypes.h"







namespace
{
	static void EnsureBlueprintNodeModulesLoaded()
	{
		FModuleManager::LoadModuleChecked<FBlueprintGraphModule>(TEXT("BlueprintGraph"));
	}

	static UBlueprint* LoadStandardMacrosBlueprint()
	{
		static TWeakObjectPtr<UBlueprint> CachedBlueprint;
		if (CachedBlueprint.IsValid())
		{
			return CachedBlueprint.Get();
		}

		const TCHAR* StandardMacrosPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros");
		if (UBlueprint* Loaded = LoadObject<UBlueprint>(nullptr, StandardMacrosPath))
		{
			CachedBlueprint = Loaded;
			return Loaded;
		}

		UE_LOG(LogSOTS_BlueprintGen, Warning, TEXT("LoadStandardMacrosBlueprint: Failed to load '%s'."), StandardMacrosPath);
		return nullptr;
	}

	static void ApplyExtraPinDefaults(UEdGraphNode* Node, const FSOTS_BPGenGraphNode& NodeSpec);
	static void ApplySelectOptionDefaults(UK2Node_Select* SelectNode, const FSOTS_BPGenGraphNode& NodeSpec);
	static FString GetPinDirectionText(EEdGraphPinDirection Direction);
	static bool ResolveStructPath(const FSOTS_BPGenGraphNode& NodeSpec, FString& OutStructPath);

	static UEdGraph* FindMacroGraphByName(UBlueprint* MacroBP, const FName MacroName)
	{
		if (!MacroBP)
		{
			return nullptr;
		}

		for (UEdGraph* Graph : MacroBP->MacroGraphs)
		{
			if (Graph && Graph->GetFName() == MacroName)
			{
				return Graph;
			}
		}

		return nullptr;
	}

	static UEdGraphNode* SpawnStandardMacroNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec, const FName MacroName)
	{
		if (!Graph)
		{
			return nullptr;
		}

		UBlueprint* MacroBP = LoadStandardMacrosBlueprint();
		UEdGraph* MacroGraph = FindMacroGraphByName(MacroBP, MacroName);
		if (!MacroGraph)
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnStandardMacroNode: Could not find macro '%s' in StandardMacros blueprint for node '%s'."),
				*MacroName.ToString(), *NodeSpec.Id);
			return nullptr;
		}

		Graph->Modify();

		UK2Node_MacroInstance* Node = NewObject<UK2Node_MacroInstance>(Graph);
		Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		Node->SetFlags(RF_Transactional);
		Node->CreateNewGuid();
		Node->SetMacroGraph(MacroGraph);
		Node->PostPlacedNewNode();
		Node->AllocateDefaultPins();

		UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SpawnStandardMacroNode: Node '%s' (macro '%s') pins after creation:"), *NodeSpec.Id, *MacroName.ToString());
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin)
			{
				UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
					*Pin->PinName.ToString(),
					*GetPinDirectionText(Pin->Direction),
					*Pin->PinType.PinCategory.ToString(),
					*UEnum::GetValueAsString(Pin->PinType.ContainerType),
					*Pin->DefaultValue);
			}
		}

		Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(Node, NodeSpec);

		return Node;
	}

	static FString GetNormalizedPackageName(const FString& InAssetPath)
	{
		FString Result = InAssetPath;
		Result.TrimStartAndEndInline();
		return Result;
	}

	static FName GetSafeObjectName(const FName& InName, const FString& AssetPath)
	{
		if (!InName.IsNone())
		{
			return InName;
		}

		FString DummyLeft, Right;
		if (AssetPath.Split(TEXT("/"), &DummyLeft, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			return FName(*Right);
		}

		return FName(TEXT("SOTS_BPGenObject"));
	}

	static FString ResolvePinCategoryString(const FName& PinCategory)
	{
		return PinCategory.IsNone() ? FString() : PinCategory.ToString();
	}

	static FString GetPinDirectionText(EEdGraphPinDirection Direction)
	{
		if (const UEnum* Enum = StaticEnum<EEdGraphPinDirection>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(Direction));
		}

		return TEXT("Unknown");
	}

	static UScriptStruct* LoadStructFromPath(const FString& StructPath)
	{
		if (StructPath.IsEmpty())
		{
			return nullptr;
		}

		if (UScriptStruct* FoundStruct = FindObject<UScriptStruct>(nullptr, *StructPath))
		{
			return FoundStruct;
		}

		return LoadObject<UScriptStruct>(nullptr, *StructPath);
	}

	static bool FillPinTypeFromBPGen(const FSOTS_BPGenPin& InPin, FEdGraphPinType& OutType)
	{
		OutType.ResetToDefaults();

		OutType.PinCategory = InPin.Category;
		OutType.PinSubCategory = InPin.SubCategory;
		OutType.PinSubCategoryObject = nullptr;

		if (!InPin.SubObjectPath.IsEmpty())
		{
			if (UObject* LoadedObj = LoadObject<UObject>(nullptr, *InPin.SubObjectPath))
			{
				OutType.PinSubCategoryObject = LoadedObj;
			}
		}

		switch (InPin.ContainerType)
		{
		case ESOTS_BPGenContainerType::Array:
			OutType.ContainerType = EPinContainerType::Array;
			break;
		case ESOTS_BPGenContainerType::Set:
			OutType.ContainerType = EPinContainerType::Set;
			break;
		case ESOTS_BPGenContainerType::Map:
			OutType.ContainerType = EPinContainerType::Map;
			break;
		default:
			OutType.ContainerType = EPinContainerType::None;
			break;
		}

		return true;
	}

	static UEdGraph* FindFunctionGraph(UBlueprint* Blueprint, FName FunctionName)
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

		return nullptr;
	}

	static UK2Node_FunctionEntry* SpawnFunctionEntryNode(UEdGraph* Graph, const FVector2D& Position)
	{
		if (!Graph)
		{
			return nullptr;
		}

		Graph->Modify();

		UK2Node_FunctionEntry* EntryNode = NewObject<UK2Node_FunctionEntry>(Graph);
		Graph->AddNode(EntryNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		EntryNode->SetFlags(RF_Transactional);
		EntryNode->CreateNewGuid();
		EntryNode->PostPlacedNewNode();
		EntryNode->AllocateDefaultPins();

		EntryNode->NodePosX = static_cast<int32>(Position.X);
		EntryNode->NodePosY = static_cast<int32>(Position.Y);

		return EntryNode;
	}

	static UK2Node_FunctionResult* SpawnFunctionResultNode(UEdGraph* Graph, const FVector2D& Position)
	{
		if (!Graph)
		{
			return nullptr;
		}

		Graph->Modify();

		UK2Node_FunctionResult* ResultNode = NewObject<UK2Node_FunctionResult>(Graph);
		Graph->AddNode(ResultNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		ResultNode->SetFlags(RF_Transactional);
		ResultNode->CreateNewGuid();
		ResultNode->PostPlacedNewNode();
		ResultNode->AllocateDefaultPins();

		ResultNode->NodePosX = static_cast<int32>(Position.X);
		ResultNode->NodePosY = static_cast<int32>(Position.Y);

		return ResultNode;
	}

	static UEdGraphPin* FindPinByName(UEdGraphNode* Node, const FName& PinName)
	{
		if (!Node)
		{
			return nullptr;
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->PinName == PinName)
			{
				return Pin;
			}
		}
		return nullptr;
	}

	static void AddNodeToMap(TMap<FString, UEdGraphNode*>& NodeMap, const FString& Key, UEdGraphNode* Node)
	{
		if (Key.IsEmpty() || !Node)
		{
			return;
		}

		if (!NodeMap.Contains(Key))
		{
			NodeMap.Add(Key, Node);
		}
	}

	static ESOTS_BPGenContainerType ParseContainerType(const FString& Value)
	{
		if (Value.Equals(TEXT("Array"), ESearchCase::IgnoreCase))
		{
			return ESOTS_BPGenContainerType::Array;
		}
		if (Value.Equals(TEXT("Set"), ESearchCase::IgnoreCase))
		{
			return ESOTS_BPGenContainerType::Set;
		}
		if (Value.Equals(TEXT("Map"), ESearchCase::IgnoreCase))
		{
			return ESOTS_BPGenContainerType::Map;
		}

		return ESOTS_BPGenContainerType::None;
	}

	static void TryAssignDefaultObject(UEdGraphPin* Pin, const FString& DefaultValue)
	{
		if (!Pin || DefaultValue.IsEmpty())
		{
			return;
		}

		auto SanitizeObjectPath = [](const FString& InPath) -> FString
		{
			FString Result = InPath;
			if ((Result.StartsWith(TEXT("Class'")) || Result.StartsWith(TEXT("Object'")) || Result.StartsWith(TEXT("BlueprintGeneratedClass'")))
				&& Result.EndsWith(TEXT("'")))
			{
				Result = Result.Mid(Result.Find(TEXT("'")) + 1); // strip leading token and quote
				Result.RemoveFromEnd(TEXT("'"));
			}
			return Result;
		};

		const FString SanitizedPath = SanitizeObjectPath(DefaultValue);

		const FName Category = Pin->PinType.PinCategory;

		if (Category == UEdGraphSchema_K2::PC_Class)
		{
			if (UClass* LoadedClass = LoadObject<UClass>(nullptr, *SanitizedPath))
			{
				Pin->DefaultObject = LoadedClass;
				Pin->DefaultValue.Reset();
			}
			return;
		}

		if (Category == UEdGraphSchema_K2::PC_Object
			|| Category == UEdGraphSchema_K2::PC_Interface
			|| Category == UEdGraphSchema_K2::PC_SoftClass
			|| Category == UEdGraphSchema_K2::PC_SoftObject)
		{
			if (UObject* LoadedObject = LoadObject<UObject>(nullptr, *SanitizedPath))
			{
				Pin->DefaultObject = LoadedObject;
			}
		}
	}

	static FSOTS_BPGenPin BuildPinDefFromNodeSpec(const FSOTS_BPGenGraphNode& NodeSpec)
	{
		FSOTS_BPGenPin PinDef;
		PinDef.Name = NodeSpec.VariableName;

		if (const FString* Category = NodeSpec.ExtraData.Find(FName(TEXT("PinCategory"))))
		{
			PinDef.Category = FName(**Category);
		}
		if (const FString* SubCategory = NodeSpec.ExtraData.Find(FName(TEXT("PinSubCategory"))))
		{
			PinDef.SubCategory = FName(**SubCategory);
		}
		if (const FString* SubObjectPath = NodeSpec.ExtraData.Find(FName(TEXT("SubObjectPath"))))
		{
			PinDef.SubObjectPath = *SubObjectPath;
		}
		if (const FString* Container = NodeSpec.ExtraData.Find(FName(TEXT("ContainerType"))))
		{
			PinDef.ContainerType = ParseContainerType(*Container);
		}
		if (const FString* DefaultValue = NodeSpec.ExtraData.Find(FName(TEXT("DefaultValue"))))
		{
			PinDef.DefaultValue = *DefaultValue;
		}

		return PinDef;
	}

	static void EnsureBlueprintVariableFromNodeSpec(UBlueprint* Blueprint, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Blueprint || NodeSpec.VariableName.IsNone())
		{
			return;
		}

		const bool bVariableExists = Blueprint && Blueprint->NewVariables.ContainsByPredicate(
			[&](const FBPVariableDescription& Desc)
			{
				return Desc.VarName == NodeSpec.VariableName;
			});

		if (bVariableExists)
		{
			UE_LOG(LogSOTS_BlueprintGen, Display,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Variable '%s' already exists on blueprint '%s'."),
				*NodeSpec.VariableName.ToString(), *Blueprint->GetPathName());
			return;
		}

		const FSOTS_BPGenPin PinDef = BuildPinDefFromNodeSpec(NodeSpec);
		if (PinDef.Category.IsNone())
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Node '%s' does not specify a PinCategory."),
				*NodeSpec.Id);
			return;
		}

		FEdGraphPinType PinType;
		if (!FillPinTypeFromBPGen(PinDef, PinType))
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Failed to build pin type for variable '%s'."),
				*NodeSpec.VariableName.ToString());
			return;
		}

		Blueprint->Modify();
		UE_LOG(LogSOTS_BlueprintGen, Verbose,
			TEXT("EnsureBlueprintVariableFromNodeSpec: Creating variable '%s' (Category=%s, Default='%s')."),
			*NodeSpec.VariableName.ToString(),
			*PinDef.Category.ToString(),
			*PinDef.DefaultValue);

		if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, NodeSpec.VariableName, PinType, PinDef.DefaultValue))
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Could not add variable '%s' to blueprint '%s'."),
				*NodeSpec.VariableName.ToString(), *Blueprint->GetPathName());
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Display,
				TEXT("EnsureBlueprintVariableFromNodeSpec: Created variable '%s' (Category=%s, Default='%s') on blueprint '%s'."),
				*NodeSpec.VariableName.ToString(), *ResolvePinCategoryString(PinDef.Category), *PinDef.DefaultValue,
				*Blueprint->GetPathName());
		}
	}

	static bool SaveBlueprint(UBlueprint* Blueprint)
	{
		if (!Blueprint)
		{
			return false;
		}

		UPackage* Package = Blueprint->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString FileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.bWarnOfLongFilename = false;

		if (!UPackage::SavePackage(Package, Blueprint, *FileName, SaveArgs))
		{
			return false;
		}

		return true;
	}

	static UEdGraphNode* SpawnCallFunctionNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Graph)
		{
			return nullptr;
		}

		if (NodeSpec.FunctionPath.IsEmpty())
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnCallFunctionNode: Node '%s' missing FunctionPath."),
				*NodeSpec.Id);
			return nullptr;
		}

		UFunction* TargetFunction = FindObject<UFunction>(nullptr, *NodeSpec.FunctionPath);
		if (!TargetFunction)
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnCallFunctionNode: Could not find function '%s' for node '%s'."),
				*NodeSpec.FunctionPath, *NodeSpec.Id);
			return nullptr;
		}

		Graph->Modify();

		UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(Graph);
		Graph->AddNode(CallNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		CallNode->SetFlags(RF_Transactional);
		CallNode->CreateNewGuid();
		CallNode->PostPlacedNewNode();
		CallNode->SetFromFunction(TargetFunction);
		CallNode->AllocateDefaultPins();

		UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SpawnCallFunctionNode: Node '%s' function '%s' pins after creation:"), *NodeSpec.Id, *NodeSpec.FunctionPath);
		for (UEdGraphPin* Pin : CallNode->Pins)
		{
			if (Pin)
			{
				UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
					*Pin->PinName.ToString(),
					*GetPinDirectionText(Pin->Direction),
					*Pin->PinType.PinCategory.ToString(),
					*UEnum::GetValueAsString(Pin->PinType.ContainerType),
					*Pin->DefaultValue);
			}
		}

		CallNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		CallNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(CallNode, NodeSpec);

		return CallNode;
	}

	static UEdGraphNode* SpawnArrayFunctionNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Graph)
		{
			return nullptr;
		}

		if (NodeSpec.FunctionPath.IsEmpty())
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnArrayFunctionNode: Node '%s' missing FunctionPath."),
				*NodeSpec.Id);
			return nullptr;
		}

		UFunction* TargetFunction = FindObject<UFunction>(nullptr, *NodeSpec.FunctionPath);
		if (!TargetFunction)
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnArrayFunctionNode: Could not find function '%s' for node '%s'."),
				*NodeSpec.FunctionPath, *NodeSpec.Id);
			return nullptr;
		}

		Graph->Modify();

		UK2Node_CallArrayFunction* ArrayNode = NewObject<UK2Node_CallArrayFunction>(Graph);
		Graph->AddNode(ArrayNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		ArrayNode->SetFlags(RF_Transactional);
		ArrayNode->CreateNewGuid();
		ArrayNode->PostPlacedNewNode();
		ArrayNode->SetFromFunction(TargetFunction);
		ArrayNode->AllocateDefaultPins();

		ArrayNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		ArrayNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(ArrayNode, NodeSpec);

		UE_LOG(LogSOTS_BlueprintGen, Display,
			TEXT("SpawnArrayFunctionNode: Node '%s' pins after creation:"),
			*NodeSpec.Id);

		const UEnum* ContainerEnum = StaticEnum<EPinContainerType>();
		for (UEdGraphPin* Pin : ArrayNode->Pins)
		{
			if (!Pin)
			{
				continue;
			}

			const FString CategoryText = ResolvePinCategoryString(Pin->PinType.PinCategory);
			const FString DirectionText = GetPinDirectionText(Pin->Direction);
			const FString ContainerText = ContainerEnum
				? ContainerEnum->GetNameStringByValue(static_cast<int64>(Pin->PinType.ContainerType))
				: TEXT("Unknown");

			UE_LOG(LogSOTS_BlueprintGen, Display,
				TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
				*Pin->PinName.ToString(), *DirectionText, *CategoryText, *ContainerText, *Pin->DefaultValue);
		}

		return ArrayNode;
	}

	static UEdGraphNode* SpawnAddComponentByClassNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Graph)
		{
			return nullptr;
		}

		Graph->Modify();

		UK2Node_AddComponentByClass* AddCompNode = NewObject<UK2Node_AddComponentByClass>(Graph);
		Graph->AddNode(AddCompNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		AddCompNode->SetFlags(RF_Transactional);
		AddCompNode->CreateNewGuid();
		AddCompNode->PostPlacedNewNode();
		AddCompNode->AllocateDefaultPins();

		AddCompNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		AddCompNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(AddCompNode, NodeSpec);

		UE_LOG(LogSOTS_BlueprintGen, Display,
			TEXT("SpawnAddComponentByClassNode: Node '%s' pins after creation:"), *NodeSpec.Id);
		for (UEdGraphPin* Pin : AddCompNode->Pins)
		{
			if (Pin)
			{
				UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
					*Pin->PinName.ToString(),
					*GetPinDirectionText(Pin->Direction),
					*Pin->PinType.PinCategory.ToString(),
					*UEnum::GetValueAsString(Pin->PinType.ContainerType),
					*Pin->DefaultValue);
			}
		}

		return AddCompNode;
	}

	static UEdGraphNode* SpawnDynamicCastNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Graph)
		{
			return nullptr;
		}

		FString TargetTypePath;
		if (const FString* TargetTypeExtra = NodeSpec.ExtraData.Find(FName(TEXT("TargetType"))))
		{
			TargetTypePath = *TargetTypeExtra;
		}
		else if (!NodeSpec.StructPath.IsEmpty())
		{
			TargetTypePath = NodeSpec.StructPath;
		}

		if (TargetTypePath.IsEmpty())
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnDynamicCastNode: Node '%s' missing TargetType/StructPath."), *NodeSpec.Id);
			return nullptr;
		}

		UClass* TargetClass = FindObject<UClass>(nullptr, *TargetTypePath);
		if (!TargetClass)
		{
			TargetClass = LoadObject<UClass>(nullptr, *TargetTypePath);
		}

		if (!TargetClass)
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnDynamicCastNode: Could not load TargetClass '%s' for node '%s'."),
				*TargetTypePath, *NodeSpec.Id);
			return nullptr;
		}

		Graph->Modify();

		UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(Graph);
		Graph->AddNode(CastNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		CastNode->SetFlags(RF_Transactional);
		CastNode->TargetType = TargetClass;
		CastNode->SetPurity(false);
		CastNode->CreateNewGuid();
		CastNode->PostPlacedNewNode();
		CastNode->AllocateDefaultPins();

		CastNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		CastNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(CastNode, NodeSpec);

		UE_LOG(LogSOTS_BlueprintGen, Display,
			TEXT("SpawnDynamicCastNode: Node '%s' pins after creation (TargetType=%s):"),
			*NodeSpec.Id, *TargetTypePath);
		for (UEdGraphPin* Pin : CastNode->Pins)
		{
			if (Pin)
			{
				UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
					*Pin->PinName.ToString(),
					*GetPinDirectionText(Pin->Direction),
					*Pin->PinType.PinCategory.ToString(),
					*UEnum::GetValueAsString(Pin->PinType.ContainerType),
					*Pin->DefaultValue);
			}
		}

		return CastNode;
	}

	static UEdGraphNode* SpawnVariableGetNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec, UBlueprint* Blueprint)
	{
		if (!Graph)
		{
			return nullptr;
		}

		EnsureBlueprintVariableFromNodeSpec(Blueprint, NodeSpec);

		if (NodeSpec.VariableName.IsNone())
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnVariableGetNode: Node '%s' missing VariableName."),
				*NodeSpec.Id);
			return nullptr;
		}

		Graph->Modify();

		UK2Node_VariableGet* VarNode = NewObject<UK2Node_VariableGet>(Graph);
		Graph->AddNode(VarNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		VarNode->SetFlags(RF_Transactional);
		VarNode->CreateNewGuid();
		VarNode->VariableReference.SetSelfMember(NodeSpec.VariableName);
		VarNode->PostPlacedNewNode();
		VarNode->AllocateDefaultPins();

		VarNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		VarNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(VarNode, NodeSpec);

		UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SpawnVariableGetNode: Node '%s' pins after creation:"), *NodeSpec.Id);
		for (UEdGraphPin* Pin : VarNode->Pins)
		{
			if (Pin)
			{
				UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
					*Pin->PinName.ToString(),
					*GetPinDirectionText(Pin->Direction),
					*Pin->PinType.PinCategory.ToString(),
					*UEnum::GetValueAsString(Pin->PinType.ContainerType),
					*Pin->DefaultValue);
			}
		}

		return VarNode;
	}

	static UEdGraphNode* SpawnVariableSetNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec, UBlueprint* Blueprint)
	{
		if (!Graph)
		{
			return nullptr;
		}

		EnsureBlueprintVariableFromNodeSpec(Blueprint, NodeSpec);

		if (NodeSpec.VariableName.IsNone())
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnVariableSetNode: Node '%s' missing VariableName."),
				*NodeSpec.Id);
			return nullptr;
		}

		Graph->Modify();

		UK2Node_VariableSet* VarNode = NewObject<UK2Node_VariableSet>(Graph);
		Graph->AddNode(VarNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		VarNode->SetFlags(RF_Transactional);
		VarNode->CreateNewGuid();
		VarNode->VariableReference.SetSelfMember(NodeSpec.VariableName);
		VarNode->PostPlacedNewNode();
		VarNode->AllocateDefaultPins();

		VarNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		VarNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(VarNode, NodeSpec);

		return VarNode;
	}

	static UEdGraphNode* SpawnBranchNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Graph)
		{
			return nullptr;
		}

		Graph->Modify();

		UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(Graph);
		Graph->AddNode(BranchNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		BranchNode->SetFlags(RF_Transactional);
		BranchNode->CreateNewGuid();
		BranchNode->PostPlacedNewNode();
		BranchNode->AllocateDefaultPins();

		BranchNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		BranchNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(BranchNode, NodeSpec);

		return BranchNode;
	}

	static UEdGraphNode* SpawnKnotNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Graph)
		{
			return nullptr;
		}

		Graph->Modify();

		UK2Node_Knot* KnotNode = NewObject<UK2Node_Knot>(Graph);
		Graph->AddNode(KnotNode, /*bFromUI=*/false, /*bSelectNewNode=*/false);
		KnotNode->SetFlags(RF_Transactional);
		KnotNode->CreateNewGuid();
		KnotNode->PostPlacedNewNode();
		KnotNode->AllocateDefaultPins();

		KnotNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
		KnotNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

		ApplyExtraPinDefaults(KnotNode, NodeSpec);

		return KnotNode;
	}

	static void ApplyExtraPinDefaults(UEdGraphNode* Node, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!Node)
		{
			return;
		}

		TMap<FString, FSOTS_BPGenPin> PinTypeOverrides;

		for (const TPair<FName, FString>& ExtraPair : NodeSpec.ExtraData)
		{
			const FString KeyString = ExtraPair.Key.ToString();
			FString PinName;
			FString Attribute;
			if (KeyString.Split(TEXT("."), &PinName, &Attribute))
			{
				FSOTS_BPGenPin& Override = PinTypeOverrides.FindOrAdd(PinName);
				if (Attribute.Equals(TEXT("PinCategory"), ESearchCase::IgnoreCase))
				{
					Override.Category = FName(*ExtraPair.Value);
				}
				else if (Attribute.Equals(TEXT("PinSubCategory"), ESearchCase::IgnoreCase))
				{
					Override.SubCategory = FName(*ExtraPair.Value);
				}
				else if (Attribute.Equals(TEXT("SubObjectPath"), ESearchCase::IgnoreCase))
				{
					Override.SubObjectPath = ExtraPair.Value;
				}
				else if (Attribute.Equals(TEXT("ContainerType"), ESearchCase::IgnoreCase))
				{
					Override.ContainerType = ParseContainerType(ExtraPair.Value);
				}
				else if (Attribute.Equals(TEXT("DefaultValue"), ESearchCase::IgnoreCase))
				{
					Override.DefaultValue = ExtraPair.Value;
				}
				continue;
			}

			if (UEdGraphPin* Pin = FindPinByName(Node, ExtraPair.Key))
			{
				const bool bIsClassPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class;
				if (!bIsClassPin)
				{
					Pin->DefaultValue = ExtraPair.Value;
				}

				TryAssignDefaultObject(Pin, ExtraPair.Value);
			}
		}

		for (const TPair<FString, FSOTS_BPGenPin>& OverridePair : PinTypeOverrides)
		{
			if (UEdGraphPin* Pin = FindPinByName(Node, FName(*OverridePair.Key)))
			{
				FEdGraphPinType NewType;
				if (FillPinTypeFromBPGen(OverridePair.Value, NewType))
				{
					Pin->PinType = NewType;
				}

				if (!OverridePair.Value.DefaultValue.IsEmpty())
				{
					const bool bIsClassPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class;
					if (!bIsClassPin)
					{
						Pin->DefaultValue = OverridePair.Value.DefaultValue;
					}

					TryAssignDefaultObject(Pin, OverridePair.Value.DefaultValue);
				}
			}
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin)
			{
				continue;
			}

			const bool bIsClassOrObjectPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class
				|| Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object;
			if (bIsClassOrObjectPin && !Pin->DefaultValue.IsEmpty())
			{
				TryAssignDefaultObject(Pin, Pin->DefaultValue);
			}

			const bool bIsTransformPin = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct
				&& Pin->PinType.PinSubCategoryObject == TBaseStructure<FTransform>::Get();
			if (bIsTransformPin && !Pin->DefaultValue.IsEmpty())
			{
				FTransform ParsedTransform = FTransform::Identity;
				if (ParsedTransform.InitFromString(Pin->DefaultValue))
				{
					Pin->DefaultValue = ParsedTransform.ToString();
				}
				else
				{
					Pin->DefaultValue = FTransform::Identity.ToString();
				}
			}
		}
	}

	static void ApplySelectOptionDefaults(UK2Node_Select* SelectNode, const FSOTS_BPGenGraphNode& NodeSpec)
	{
		if (!SelectNode)
		{
			return;
		}

		TArray<UEdGraphPin*> OptionPins;
		SelectNode->GetOptionPins(OptionPins);

		for (int32 Index = 0; Index < OptionPins.Num(); ++Index)
		{
			UEdGraphPin* OptionPin = OptionPins[Index];
			if (!OptionPin)
			{
				continue;
			}

			const FString Key = FString::Printf(TEXT("Option %d.DefaultValue"), Index);
			if (const FString* DefaultPtr = NodeSpec.ExtraData.Find(*Key))
			{
				OptionPin->DefaultValue = *DefaultPtr;
			}
		}

		SelectNode->NodeConnectionListChanged();
	}

static bool ResolveStructPath(const FSOTS_BPGenGraphNode& NodeSpec, FString& OutStructPath)
{
	if (const FString* ExtraStructPath = NodeSpec.ExtraData.Find(FName(TEXT("StructPath"))))
	{
		OutStructPath = *ExtraStructPath;
	}
	else
	{
		OutStructPath = NodeSpec.StructPath;
	}

	return !OutStructPath.IsEmpty();
}

	static void ClearNonExecPins(UEdGraphNode* Node)
	{
		if (!Node)
		{
			return;
		}

		Node->Modify();

		TArray<UEdGraphPin*> PinsToRemove;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin)
			{
				continue;
			}

			if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				continue;
			}

			PinsToRemove.Add(Pin);
		}

		for (UEdGraphPin* Pin : PinsToRemove)
		{
			Pin->BreakAllPinLinks();
			Node->RemovePin(Pin);
		}
	}

	static bool ValidateLinkPins(const FSOTS_BPGenGraphLink& Link, UEdGraphPin* FromPin, UEdGraphPin* ToPin, FSOTS_BPGenApplyResult& Result)
	{
		if (!FromPin || !ToPin)
		{
			return false;
		}

		bool bIsValid = true;
		if (FromPin->Direction != EGPD_Output)
		{
			const FString Message = FString::Printf(
				TEXT("Link '%s.%s' expects an output pin but '%s' has direction '%s'."),
				*Link.FromNodeId,
				*Link.FromPinName.ToString(),
				*Link.FromNodeId,
				*GetPinDirectionText(FromPin->Direction));
			UE_LOG(LogSOTS_BlueprintGen, Warning, TEXT("%s"), *Message);
			Result.Warnings.Add(Message);
			bIsValid = false;
		}

		if (ToPin->Direction != EGPD_Input)
		{
			const FString Message = FString::Printf(
				TEXT("Link '%s.%s' expects an input pin but '%s' has direction '%s'."),
				*Link.ToNodeId,
				*Link.ToPinName.ToString(),
				*Link.ToNodeId,
				*GetPinDirectionText(ToPin->Direction));
			UE_LOG(LogSOTS_BlueprintGen, Warning, TEXT("%s"), *Message);
			Result.Warnings.Add(Message);
			bIsValid = false;
		}

		return bIsValid;
	}

	static int32 AddPinsFromSpec(UEdGraphNode* Node, EEdGraphPinDirection Direction, const TArray<FSOTS_BPGenPin>& PinDefs, TArray<FString>& OutWarnings)
	{
		if (!Node || PinDefs.Num() == 0)
		{
			return 0;
		}

		int32 AddedCount = 0;
		int32 FallbackIndex = 0;
		for (const FSOTS_BPGenPin& PinDef : PinDefs)
		{
			FString PinNameString;
			if (PinDef.Name.IsNone())
			{
				PinNameString = FString::Printf(TEXT("BPGenPin_%d"), ++FallbackIndex);
			}
			else
			{
				PinNameString = PinDef.Name.ToString();
			}

			FEdGraphPinType PinType;
			if (!FillPinTypeFromBPGen(PinDef, PinType))
			{
				OutWarnings.Add(FString::Printf(
					TEXT("ApplyFunctionSkeleton: Pin '%s' has invalid type information."),
					*PinNameString));
				continue;
			}

			UEdGraphPin* NewPin = Node->CreatePin(Direction, PinType, FName(*PinNameString));

			if (!NewPin)
			{
				OutWarnings.Add(FString::Printf(
					TEXT("ApplyFunctionSkeleton: Failed to create pin '%s'."),
					*PinNameString));
				continue;
			}

			if (!PinDef.DefaultValue.IsEmpty())
			{
				NewPin->DefaultValue = PinDef.DefaultValue;
			}

			++AddedCount;
		}

		if (UEdGraph* Graph = Node->GetGraph())
		{
			Graph->NotifyGraphChanged();
		}
		return AddedCount;
	}
}

template <typename TNode>
static UEdGraphNode* SpawnBasicNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	TNode* Node = NewObject<TNode>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static UEdGraphNode* SpawnExecutionSequenceNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	UK2Node_ExecutionSequence* Node = NewObject<UK2Node_ExecutionSequence>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	int32 DesiredOutputs = 2;
	if (const FString* NumOutputsStr = NodeSpec.ExtraData.Find(FName(TEXT("NumOutputs"))))
	{
		DesiredOutputs = FMath::Max(2, FCString::Atoi(**NumOutputsStr));
	}

	auto CountExecOutputs = [](UK2Node_ExecutionSequence* SeqNode)
	{
		int32 Count = 0;
		for (UEdGraphPin* Pin : SeqNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				++Count;
			}
		}
		return Count;
	};

	int32 CurrentOutputs = CountExecOutputs(Node);
	for (int32 Index = CurrentOutputs; Index < DesiredOutputs; ++Index)
	{
		const FName NewPinName(*FString::Printf(TEXT("Then_%d"), Index));
		Node->CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, NewPinName);
	}

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static UEdGraphNode* SpawnMakeArrayNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	int32 DesiredInputs = 0;
	if (const FString* NumInputsStr = NodeSpec.ExtraData.Find(FName(TEXT("NumInputs"))))
	{
		DesiredInputs = FMath::Max(0, FCString::Atoi(**NumInputsStr));
	}

	DesiredInputs = FMath::Max(1, DesiredInputs);

	UK2Node_MakeArray* Node = NewObject<UK2Node_MakeArray>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();
	Node->NumInputs = DesiredInputs;
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	FSOTS_BPGenPin ElementPinSpec;
	if (const FString* PinCategory = NodeSpec.ExtraData.Find(FName(TEXT("PinCategory"))))
	{
		ElementPinSpec.Category = FName(**PinCategory);
	}
	if (const FString* PinSubCategory = NodeSpec.ExtraData.Find(FName(TEXT("PinSubCategory"))))
	{
		ElementPinSpec.SubCategory = FName(**PinSubCategory);
	}
	if (const FString* SubObjectPath = NodeSpec.ExtraData.Find(FName(TEXT("SubObjectPath"))))
	{
		ElementPinSpec.SubObjectPath = *SubObjectPath;
	}
	if (const FString* ContainerType = NodeSpec.ExtraData.Find(FName(TEXT("ContainerType"))))
	{
		ElementPinSpec.ContainerType = ParseContainerType(*ContainerType);
	}

	FEdGraphPinType ElementType;
	if (FillPinTypeFromBPGen(ElementPinSpec, ElementType))
	{
		FEdGraphPinType ArrayType = ElementType;
		ArrayType.ContainerType = EPinContainerType::Array;

		TMap<FString, FString> InputDefaults;
		for (const TPair<FName, FString>& ExtraPair : NodeSpec.ExtraData)
		{
			const FString KeyString = ExtraPair.Key.ToString();
			if (KeyString.StartsWith(TEXT("Input")))
			{
				InputDefaults.Add(KeyString.RightChop(5), ExtraPair.Value); // strip "Input"
			}
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin)
			{
				continue;
			}

			if (Pin->Direction == EGPD_Output)
			{
				Pin->PinType = ArrayType;
				continue;
			}

			Pin->PinType = ElementType;

			const FString PinKey = Pin->PinName.ToString();
			if (const FString* DefaultValue = InputDefaults.Find(PinKey))
			{
				Pin->DefaultValue = *DefaultValue;
			}
		}
	}

	// Apply defaults after all pins exist so inputs can be initialized from the spec.
	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static UEdGraphNode* SpawnMakeStructNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	FString StructPath;
	if (!ResolveStructPath(NodeSpec, StructPath))
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SpawnMakeStructNode: Node '%s' missing StructPath."),
			*NodeSpec.Id);
		return nullptr;
	}

	UScriptStruct* StructType = LoadStructFromPath(StructPath);
	if (!StructType)
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SpawnMakeStructNode: Failed to resolve StructPath '%s' for node '%s'."),
			*StructPath,
			*NodeSpec.Id);
		return nullptr;
	}

	Graph->Modify();

	UK2Node_MakeStruct* Node = NewObject<UK2Node_MakeStruct>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->StructType = StructType;
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();
	Node->ReconstructNode();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static UEdGraphNode* SpawnBreakStructNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	FString StructPath;
	if (!ResolveStructPath(NodeSpec, StructPath))
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SpawnBreakStructNode: Node '%s' missing StructPath."),
			*NodeSpec.Id);
		return nullptr;
	}

	UScriptStruct* StructType = LoadStructFromPath(StructPath);
	if (!StructType)
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SpawnBreakStructNode: Failed to resolve StructPath '%s' for node '%s'."),
			*StructPath,
			*NodeSpec.Id);
		return nullptr;
	}

	Graph->Modify();

	UK2Node_BreakStruct* Node = NewObject<UK2Node_BreakStruct>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->StructType = StructType;
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();
	Node->ReconstructNode();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static UEdGraphNode* SpawnEventNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec, UBlueprint* Blueprint)
{
	if (!Graph || !Blueprint)
	{
		return nullptr;
	}

	const FString* EventNameStr = NodeSpec.ExtraData.Find(FName(TEXT("EventName")));
	if (!EventNameStr || EventNameStr->IsEmpty())
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SpawnEventNode: Node '%s' missing EventName in ExtraData."),
			*NodeSpec.Id);
		return nullptr;
	}

	const FName EventName(**EventNameStr);

	UBlueprintGeneratedClass* SkeletonClass = Cast<UBlueprintGeneratedClass>(Blueprint->SkeletonGeneratedClass.Get());
	if (!SkeletonClass)
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SpawnEventNode: Blueprint '%s' has no valid SkeletonGeneratedClass."),
			*Blueprint->GetPathName());
		return nullptr;
	}

	UFunction* EventFunction = SkeletonClass->FindFunctionByName(EventName);
	if (!EventFunction)
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SpawnEventNode: Could not find event function '%s' on skeleton class '%s'."),
			*EventName.ToString(),
			*SkeletonClass->GetName());
		return nullptr;
	}

	Graph->Modify();

	UK2Node_Event* Node = NewObject<UK2Node_Event>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();
	Node->EventReference.SetFromField<UFunction>(EventFunction, /*bSelfContext=*/false);
	Node->bOverrideFunction = true;

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static UEdGraphNode* SpawnCustomEventNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	const FString* EventNameStr = NodeSpec.ExtraData.Find(FName(TEXT("EventName")));
	if (!EventNameStr || EventNameStr->IsEmpty())
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SpawnCustomEventNode: Node '%s' missing EventName in ExtraData."),
			*NodeSpec.Id);
		return nullptr;
	}

	Graph->Modify();

	UK2Node_CustomEvent* Node = NewObject<UK2Node_CustomEvent>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();
	Node->CustomFunctionName = FName(**EventNameStr);

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static UEdGraphNode* SpawnSelectNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	UK2Node_Select* Node = NewObject<UK2Node_Select>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();

	bool bApplyStructType = false;
	FEdGraphPinType DesiredStructPinType;

	// If a TypePath is provided, try to set the select type before pin allocation so pins get the right schema.
	if (const FString* TypePath = NodeSpec.ExtraData.Find(FName(TEXT("TypePath"))))
	{
		// Prefer structs first (common case), then enums. This avoids noisy log warnings when the path is a struct.
		if (UScriptStruct* StructType = LoadStructFromPath(*TypePath))
		{
			DesiredStructPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			DesiredStructPinType.PinSubCategoryObject = StructType;
			DesiredStructPinType.ContainerType = EPinContainerType::None;
			bApplyStructType = true;
		}
		else if (UEnum* EnumType = LoadObject<UEnum>(nullptr, **TypePath))
		{
			Node->SetEnum(EnumType);
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnSelectNode: Failed to resolve TypePath '%s' for node '%s'."),
				**TypePath, *NodeSpec.Id);
		}
		// If neither enum nor struct, fall back to wildcard and let ExtraData pin overrides shape the type.
	}

	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	int32 DesiredOptionPins = 0;
	if (const FString* NumOptionsStr = NodeSpec.ExtraData.Find(FName(TEXT("NumOptionPins"))))
	{
		DesiredOptionPins = FMath::Max(0, FCString::Atoi(**NumOptionsStr));
	}

	if (DesiredOptionPins > 0)
	{
		TArray<UEdGraphPin*> OptionPins;
		Node->GetOptionPins(OptionPins);

		while (OptionPins.Num() < DesiredOptionPins)
		{
			Node->AddInputPin();
			OptionPins.Reset();
			Node->GetOptionPins(OptionPins);
		}
	}

	// Apply index type + default so it does not remain wildcard.
	if (UEdGraphPin* IndexPin = Node->GetIndexPin())
	{
		IndexPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
		IndexPin->PinType.PinSubCategory = NAME_None;
		IndexPin->PinType.PinSubCategoryObject = nullptr;
		const FString IndexDefault = NodeSpec.ExtraData.FindRef(FName(TEXT("IndexDefault")));
		IndexPin->DefaultValue = IndexDefault.IsEmpty() ? TEXT("0") : IndexDefault;
		Node->PinTypeChanged(IndexPin);
	}

	if (bApplyStructType)
	{
		TArray<UEdGraphPin*> OptionPins;
		Node->GetOptionPins(OptionPins);
		for (UEdGraphPin* OptionPin : OptionPins)
		{
			if (OptionPin)
			{
				OptionPin->PinType = DesiredStructPinType;
				Node->PinTypeChanged(OptionPin);
			}
		}

		if (UEdGraphPin* ResultPin = Node->GetReturnValuePin())
		{
			ResultPin->PinType = DesiredStructPinType;
			Node->PinTypeChanged(ResultPin);
		}

		if (UEdGraphPin* DefaultPin = Node->FindPin(FName(TEXT("Default"))))
		{
			DefaultPin->PinType = DesiredStructPinType;
			Node->PinTypeChanged(DefaultPin);
		}
	}

	ApplySelectOptionDefaults(Node, NodeSpec);

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static UK2Node* SpawnNodeByClassPath(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec, const TCHAR* ClassPath)
{
	if (!Graph)
	{
		return nullptr;
	}

	UClass* NodeClass = LoadObject<UClass>(nullptr, ClassPath);
	if (!NodeClass)
	{
		UE_LOG(LogSOTS_BlueprintGen, Warning,
			TEXT("SpawnNodeByClassPath: Could not load class '%s' for node '%s'."),
			ClassPath, *NodeSpec.Id);
		return nullptr;
	}

	Graph->Modify();

	UK2Node* Node = NewObject<UK2Node>(Graph, NodeClass);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static UEdGraphNode* SpawnForLoopWithBreakNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	return SpawnStandardMacroNode(Graph, NodeSpec, FName(TEXT("ForLoopWithBreak")));
}

static UEdGraphNode* SpawnForLoopNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	return nullptr; // ForLoop node intentionally disabled
}

static UEdGraphNode* SpawnForEachLoopWithBreakNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	return SpawnStandardMacroNode(Graph, NodeSpec, FName(TEXT("ForEachLoopWithBreak")));
}

static UEdGraphNode* SpawnForEachLoopNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	return SpawnStandardMacroNode(Graph, NodeSpec, FName(TEXT("ForEachLoop")));
}

static UEdGraphNode* SpawnForEachElementInArrayNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	return nullptr; // ForEachElementInArray node disabled due to missing header
}

static UEdGraphNode* SpawnDoOnceNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	return SpawnStandardMacroNode(Graph, NodeSpec, FName(TEXT("DoOnce")));
}

static UEdGraphNode* SpawnGateNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	return SpawnStandardMacroNode(Graph, NodeSpec, FName(TEXT("Gate")));
}

static UEdGraphNode* SpawnFlipFlopNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	return SpawnStandardMacroNode(Graph, NodeSpec, FName(TEXT("FlipFlop")));
}

static UEdGraphNode* SpawnAssignDelegateNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec, UBlueprint* Blueprint)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	UK2Node_AssignDelegate* Node = NewObject<UK2Node_AssignDelegate>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();

	// Attempt to bind to a delegate property on the owning Blueprint (self context).
	if (Blueprint)
	{
		const FString* DelegatePropStr = NodeSpec.ExtraData.Find(FName(TEXT("DelegatePropertyName")));
		if (DelegatePropStr && !DelegatePropStr->IsEmpty())
		{
			const FName DelegatePropName(**DelegatePropStr);
			UClass* OwnerClass = Blueprint->SkeletonGeneratedClass ? Blueprint->SkeletonGeneratedClass : Blueprint->GeneratedClass;
			if (OwnerClass)
			{
				if (FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(OwnerClass, DelegatePropName))
				{
					Node->SetFromProperty(DelegateProp, /*bSelfContext=*/true, OwnerClass);
				}
				else
				{
					UE_LOG(LogSOTS_BlueprintGen, Warning,
						TEXT("SpawnAssignDelegateNode: Could not find delegate property '%s' on class '%s' for node '%s'."),
						*DelegatePropName.ToString(), *OwnerClass->GetName(), *NodeSpec.Id);
				}
			}
		}
	}

	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	ApplyExtraPinDefaults(Node, NodeSpec);

	return Node;
}

static int32 DetermineMultiGateOutputs(const FSOTS_BPGenGraphNode& NodeSpec, const TArray<FSOTS_BPGenGraphLink>& Links)
{
	int32 DesiredOutputs = 2; // default similar to ExecutionSequence minimum
	for (const FSOTS_BPGenGraphLink& Link : Links)
	{
		if (Link.FromNodeId != NodeSpec.Id)
		{
			continue;
		}

		const FString PinName = Link.FromPinName.ToString();
		if (PinName.StartsWith(TEXT("Out_")))
		{
			const FString IndexStr = PinName.RightChop(4);
			int32 IndexValue = 0;
			if (IndexStr.IsNumeric())
			{
				IndexValue = FCString::Atoi(*IndexStr);
				DesiredOutputs = FMath::Max(DesiredOutputs, IndexValue + 1);
			}
		}
	}
	return DesiredOutputs;
}

static UEdGraphNode* SpawnMultiGateNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec, const TArray<FSOTS_BPGenGraphLink>& Links)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	UK2Node_MultiGate* Node = NewObject<UK2Node_MultiGate>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	const int32 DesiredOutputs = DetermineMultiGateOutputs(NodeSpec, Links);
	int32 CurrentOutputs = 0;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
		{
			++CurrentOutputs;
		}
	}

	for (int32 Index = CurrentOutputs; Index < DesiredOutputs; ++Index)
	{
		const FName NewPinName(*FString::Printf(TEXT("Out_%d"), Index));
		Node->CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, NewPinName);
	}

	ApplyExtraPinDefaults(Node, NodeSpec);
	return Node;
}

static UEdGraphNode* SpawnEnumLiteralNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	UK2Node_EnumLiteral* Node = NewObject<UK2Node_EnumLiteral>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();

	if (const FString* EnumPath = NodeSpec.ExtraData.Find(FName(TEXT("EnumPath"))))
	{
		if (UEnum* EnumAsset = LoadObject<UEnum>(nullptr, **EnumPath))
		{
			Node->Enum = EnumAsset;
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnEnumLiteralNode: Could not load enum '%s' for node '%s'."),
				**EnumPath, *NodeSpec.Id);
		}
	}

	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();
	Node->ReconstructNode();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	ApplyExtraPinDefaults(Node, NodeSpec);

	if (const FString* EnumValue = NodeSpec.ExtraData.Find(FName(TEXT("EnumValue"))))
	{
		if (UEdGraphPin* ReturnPin = FindPinByName(Node, FName(TEXT("ReturnValue"))))
		{
			ReturnPin->DefaultValue = *EnumValue;
		}
	}

	return Node;
}

static UEdGraphNode* SpawnSwitchIntegerNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	UK2Node_SwitchInteger* Node = Cast<UK2Node_SwitchInteger>(SpawnBasicNode<UK2Node_SwitchInteger>(Graph, NodeSpec));
	if (!Node)
	{
		return nullptr;
	}

	int32 CaseCount = 0;
	if (const FString* CaseCountStr = NodeSpec.ExtraData.Find(FName(TEXT("CaseCount"))))
	{
		CaseCount = FCString::Atoi(**CaseCountStr);
	}

	for (int32 Index = 0; Index < CaseCount; ++Index)
	{
		Node->AddPinToSwitchNode();
	}

	UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SpawnSwitchIntegerNode: Node '%s' pins after creation:"), *NodeSpec.Id);
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
				*Pin->PinName.ToString(),
				*GetPinDirectionText(Pin->Direction),
				*Pin->PinType.PinCategory.ToString(),
				*UEnum::GetValueAsString(Pin->PinType.ContainerType),
				*Pin->DefaultValue);
		}
	}

	return Node;
}

static UEdGraphNode* SpawnSwitchStringNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	UK2Node_SwitchString* Node = Cast<UK2Node_SwitchString>(SpawnBasicNode<UK2Node_SwitchString>(Graph, NodeSpec));
	if (!Node)
	{
		return nullptr;
	}

	int32 CaseCount = 0;
	if (const FString* CaseCountStr = NodeSpec.ExtraData.Find(FName(TEXT("CaseCount"))))
	{
		CaseCount = FCString::Atoi(**CaseCountStr);
	}

	for (int32 Index = 0; Index < CaseCount; ++Index)
	{
		Node->AddPinToSwitchNode();
	}

	UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SpawnSwitchStringNode: Node '%s' pins after creation:"), *NodeSpec.Id);
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
				*Pin->PinName.ToString(),
				*GetPinDirectionText(Pin->Direction),
				*Pin->PinType.PinCategory.ToString(),
				*UEnum::GetValueAsString(Pin->PinType.ContainerType),
				*Pin->DefaultValue);
		}
	}

	return Node;
}

static UEdGraphNode* SpawnNameLiteralNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	// Map name literal to the existing KismetSystemLibrary helper to avoid a dedicated node class.
	FSOTS_BPGenGraphNode CallSpec = NodeSpec;
	CallSpec.FunctionPath = TEXT("/Script/Engine.KismetSystemLibrary:MakeLiteralName");

	UEdGraphNode* Node = SpawnCallFunctionNode(Graph, CallSpec);
	if (!Node)
	{
		return nullptr;
	}

	if (const FString* LiteralValue = NodeSpec.ExtraData.Find(FName(TEXT("Value"))))
	{
		if (UEdGraphPin* ValuePin = FindPinByName(Node, FName(TEXT("Value"))))
		{
			ValuePin->DefaultValue = *LiteralValue;
		}
	}

	return Node;
}

static UEdGraphNode* SpawnSwitchNameNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	UK2Node_SwitchName* Node = Cast<UK2Node_SwitchName>(SpawnBasicNode<UK2Node_SwitchName>(Graph, NodeSpec));
	if (!Node)
	{
		return nullptr;
	}

	int32 CaseCount = 0;
	if (const FString* CaseCountStr = NodeSpec.ExtraData.Find(FName(TEXT("CaseCount"))))
	{
		CaseCount = FCString::Atoi(**CaseCountStr);
	}

	for (int32 Index = 0; Index < CaseCount; ++Index)
	{
		Node->AddPinToSwitchNode();
	}

	UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SpawnSwitchNameNode: Node '%s' pins after creation:"), *NodeSpec.Id);
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
				*Pin->PinName.ToString(),
				*GetPinDirectionText(Pin->Direction),
				*Pin->PinType.PinCategory.ToString(),
				*UEnum::GetValueAsString(Pin->PinType.ContainerType),
				*Pin->DefaultValue);
		}
	}

	return Node;
}

static UEdGraphNode* SpawnMakeLiteralGameplayTagNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	// Avoid deprecated gameplay tag literal node; call the kismet helper function instead.
	FSOTS_BPGenGraphNode CallSpec = NodeSpec;
	// BlueprintGameplayTagLibrary hosts the literal helpers in UE5.7; use that path so FindObject succeeds.
	CallSpec.FunctionPath = TEXT("/Script/GameplayTags.BlueprintGameplayTagLibrary:MakeLiteralGameplayTag");

	UEdGraphNode* Node = SpawnCallFunctionNode(Graph, CallSpec);
	if (!Node)
	{
		return nullptr;
	}

	if (const FString* TagValue = NodeSpec.ExtraData.Find(FName(TEXT("Tag"))))
	{
		if (UEdGraphPin* TagPin = FindPinByName(Node, FName(TEXT("Value"))))
		{
			// FGameplayTag ImportText expects a struct-style literal like (TagName="GameplayCue.Default").
			if (!TagValue->IsEmpty())
			{
				TagPin->DefaultValue = FString::Printf(TEXT("(TagName=\"%s\")"), **TagValue);
			}
			else
			{
				TagPin->DefaultValue.Reset();
			}
		}
	}

	UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SpawnMakeLiteralGameplayTagNode: Node '%s' pins after creation:"), *NodeSpec.Id);
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
				*Pin->PinName.ToString(),
				*GetPinDirectionText(Pin->Direction),
				*Pin->PinType.PinCategory.ToString(),
				*UEnum::GetValueAsString(Pin->PinType.ContainerType),
				*Pin->DefaultValue);
		}
	}

	return Node;
}

static UEdGraphNode* SpawnSwitchGameplayTagNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	UGameplayTagsK2Node_SwitchGameplayTag* Node = Cast<UGameplayTagsK2Node_SwitchGameplayTag>(SpawnBasicNode<UGameplayTagsK2Node_SwitchGameplayTag>(Graph, NodeSpec));
	if (!Node)
	{
		return nullptr;
	}

	int32 CaseCount = 0;
	if (const FString* CaseCountStr = NodeSpec.ExtraData.Find(FName(TEXT("CaseCount"))))
	{
		CaseCount = FCString::Atoi(**CaseCountStr);
	}

	for (int32 Index = 0; Index < CaseCount; ++Index)
	{
		Node->AddPinToSwitchNode();
	}

	UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SpawnSwitchGameplayTagNode: Node '%s' pins after creation:"), *NodeSpec.Id);
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
				*Pin->PinName.ToString(),
				*GetPinDirectionText(Pin->Direction),
				*Pin->PinType.PinCategory.ToString(),
				*UEnum::GetValueAsString(Pin->PinType.ContainerType),
				*Pin->DefaultValue);
		}
	}

	return Node;
}

static UEdGraphNode* SpawnSwitchEnumNode(UEdGraph* Graph, const FSOTS_BPGenGraphNode& NodeSpec)
{
	if (!Graph)
	{
		return nullptr;
	}

	Graph->Modify();

	UK2Node_SwitchEnum* Node = NewObject<UK2Node_SwitchEnum>(Graph);
	Graph->AddNode(Node, /*bFromUI=*/false, /*bSelectNewNode=*/false);
	Node->SetFlags(RF_Transactional);
	Node->CreateNewGuid();

	if (const FString* EnumPath = NodeSpec.ExtraData.Find(FName(TEXT("EnumPath"))))
	{
		if (UEnum* EnumAsset = LoadObject<UEnum>(nullptr, **EnumPath))
		{
			if (FObjectProperty* EnumProp = FindFProperty<FObjectProperty>(UK2Node_SwitchEnum::StaticClass(), TEXT("Enum")))
			{
				EnumProp->SetObjectPropertyValue_InContainer(Node, EnumAsset);
			}
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("SpawnSwitchEnumNode: Could not load enum '%s' for node '%s'."),
				**EnumPath, *NodeSpec.Id);
		}
	}

	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();
	Node->ReconstructNode();

	Node->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
	Node->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);

	ApplyExtraPinDefaults(Node, NodeSpec);

	UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("SpawnSwitchEnumNode: Node '%s' pins after creation:"), *NodeSpec.Id);
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			UE_LOG(LogSOTS_BlueprintGen, Display, TEXT("  Pin '%s' Dir=%s Cat=%s Container=%s Default='%s'"),
				*Pin->PinName.ToString(),
				*GetPinDirectionText(Pin->Direction),
				*Pin->PinType.PinCategory.ToString(),
				*UEnum::GetValueAsString(Pin->PinType.ContainerType),
				*Pin->DefaultValue);
		}
	}

	return Node;
}

FSOTS_BPGenAssetResult USOTS_BPGenBuilder::CreateStructAssetFromDef(
	const UObject* WorldContextObject,
	const FSOTS_BPGenStructDef& StructDef)
{
	FSOTS_BPGenAssetResult Result;
	Result.AssetPath = StructDef.AssetPath;

	if (StructDef.AssetPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.Message = TEXT("StructDef.AssetPath is empty.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("CreateStructAssetFromDef failed: AssetPath is empty."));
		return Result;
	}

	const FString PackageName = GetNormalizedPackageName(StructDef.AssetPath);
	const FName StructName = GetSafeObjectName(StructDef.StructName, PackageName);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		Result.bSuccess = false;
		Result.Message = FString::Printf(TEXT("Failed to create or load package '%s'."), *PackageName);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
		return Result;
	}

	// Ensure the package is fully loaded before we start mutating it so SavePackage won't fail with partial load errors.
	Package->FullyLoad();

	UUserDefinedStruct* TargetStruct = FindObject<UUserDefinedStruct>(Package, *StructName.ToString());
	if (!TargetStruct)
	{
		TargetStruct = FStructureEditorUtils::CreateUserDefinedStruct(
			Package,
			StructName,
			RF_Public | RF_Standalone | RF_Transactional);

		if (!TargetStruct)
		{
			Result.bSuccess = false;
			Result.Message = FString::Printf(TEXT("Failed to create user-defined struct '%s'."), *StructName.ToString());
			UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
			return Result;
		}
	}
	else
	{
		UE_LOG(LogSOTS_BlueprintGen, Log,
			TEXT("CreateStructAssetFromDef: Updating existing struct '%s' in package '%s'."),
			*StructName.ToString(), *PackageName);
	}

	TArray<FStructVariableDescription>& VarDescs = FStructureEditorUtils::GetVarDesc(TargetStruct);
	VarDescs.Reset();

	for (const FSOTS_BPGenPin& MemberPin : StructDef.Members)
	{
		FStructVariableDescription NewVar;
		NewVar.VarGuid = FGuid::NewGuid();
		NewVar.VarName = MemberPin.Name.IsNone() ? FName(TEXT("Member")) : MemberPin.Name;
		NewVar.FriendlyName = NewVar.VarName.ToString();

		FEdGraphPinType PinType;
		if (FillPinTypeFromBPGen(MemberPin, PinType))
		{
			NewVar.SetPinType(PinType);
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning, TEXT("CreateStructAssetFromDef: Invalid pin definition for member '%s'."), *NewVar.VarName.ToString());
		}

		VarDescs.Add(MoveTemp(NewVar));
	}

	FStructureEditorUtils::OnStructureChanged(TargetStruct);

	FAssetRegistryModule::AssetCreated(TargetStruct);
	Package->MarkPackageDirty();

	const FString FileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bWarnOfLongFilename = false;

	if (!UPackage::SavePackage(Package, TargetStruct, *FileName, SaveArgs))
	{
		Result.bSuccess = false;
		Result.Message = FString::Printf(TEXT("Failed to save struct package '%s'."), *FileName);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
		return Result;
	}

	Result.bSuccess = true;
	Result.Message = FString::Printf(TEXT("Struct '%s' created/updated at '%s'."), *StructName.ToString(), *PackageName);
	return Result;
}

FSOTS_BPGenAssetResult USOTS_BPGenBuilder::CreateEnumAssetFromDef(
	const UObject* WorldContextObject,
	const FSOTS_BPGenEnumDef& EnumDef)
{
	FSOTS_BPGenAssetResult Result;
	Result.AssetPath = EnumDef.AssetPath;

	if (EnumDef.AssetPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.Message = TEXT("EnumDef.AssetPath is empty.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("CreateEnumAssetFromDef failed: AssetPath is empty."));
		return Result;
	}

	const FString PackageName = GetNormalizedPackageName(EnumDef.AssetPath);
	const FName EnumName = GetSafeObjectName(EnumDef.EnumName, PackageName);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		Result.bSuccess = false;
		Result.Message = FString::Printf(TEXT("Failed to create or load package '%s'."), *PackageName);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
		return Result;
	}

	// Ensure the package is fully loaded so updates to existing enums operate on a complete asset state.
	Package->FullyLoad();

	UUserDefinedEnum* TargetEnum = FindObject<UUserDefinedEnum>(Package, *EnumName.ToString());
	if (!TargetEnum)
	{
		TargetEnum = NewObject<UUserDefinedEnum>(
			Package,
			EnumName,
			RF_Public | RF_Standalone | RF_Transactional);

		if (!TargetEnum)
		{
			Result.bSuccess = false;
			Result.Message = FString::Printf(TEXT("Failed to create user-defined enum '%s'."), *EnumName.ToString());
			UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
			return Result;
		}
	}
	else
	{
		UE_LOG(LogSOTS_BlueprintGen, Log,
			TEXT("CreateEnumAssetFromDef: Updating existing enum '%s' in package '%s'."),
			*EnumName.ToString(), *PackageName);
	}

	TArray<TPair<FName, int64>> EnumNames;
	for (int32 Index = 0; Index < EnumDef.Values.Num(); ++Index)
	{
		const FString& EntryString = EnumDef.Values[Index];
		const FName EntryName = FName(*EntryString);

		EnumNames.Add(TPair<FName, int64>(EntryName, Index));
	}

	TargetEnum->UEnum::SetEnums(EnumNames, UUserDefinedEnum::ECppForm::Namespaced);
	TargetEnum->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(TargetEnum);
	Package->MarkPackageDirty();

	const FString FileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.bWarnOfLongFilename = false;

	if (!UPackage::SavePackage(Package, TargetEnum, *FileName, SaveArgs))
	{
		Result.bSuccess = false;
		Result.Message = FString::Printf(TEXT("Failed to save enum package '%s'."), *FileName);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.Message);
		return Result;
	}

	Result.bSuccess = true;
	Result.Message = FString::Printf(TEXT("Enum '%s' created/updated at '%s'."), *EnumName.ToString(), *PackageName);
	return Result;
}

FSOTS_BPGenApplyResult USOTS_BPGenBuilder::ApplyFunctionSkeleton(
	const UObject* WorldContextObject,
	const FSOTS_BPGenFunctionDef& FunctionDef)
{
	FSOTS_BPGenApplyResult Result;
	Result.TargetBlueprintPath = FunctionDef.TargetBlueprintPath;
	Result.FunctionName = FunctionDef.FunctionName;

	if (FunctionDef.TargetBlueprintPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyFunctionSkeleton: TargetBlueprintPath is empty.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	if (FunctionDef.FunctionName.IsNone())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyFunctionSkeleton: FunctionName is None.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(
		StaticLoadObject(UBlueprint::StaticClass(), nullptr, *FunctionDef.TargetBlueprintPath));

	if (!Blueprint)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(
			TEXT("ApplyFunctionSkeleton: Failed to load Blueprint at '%s'."),
			*FunctionDef.TargetBlueprintPath);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	UEdGraph* FunctionGraph = FindFunctionGraph(Blueprint, FunctionDef.FunctionName);

	if (!FunctionGraph)
	{
		// Create a new function graph using the K2 schema.
		FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint,
			FunctionDef.FunctionName,
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass());

		if (!FunctionGraph)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(
				TEXT("ApplyFunctionSkeleton: Failed to create function graph '%s'."),
				*FunctionDef.FunctionName.ToString());
			UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
			return Result;
		}

		FBlueprintEditorUtils::AddFunctionGraph<UFunction>(
			Blueprint,
			FunctionGraph,
			/*bIsUserCreated=*/true,
			nullptr);

		UE_LOG(LogSOTS_BlueprintGen, Log,
			TEXT("ApplyFunctionSkeleton: Created new function graph '%s' in '%s'."),
			*FunctionDef.FunctionName.ToString(),
			*FunctionDef.TargetBlueprintPath);
	}
	else
	{
		UE_LOG(LogSOTS_BlueprintGen, Log,
			TEXT("ApplyFunctionSkeleton: Reusing existing function graph '%s' in '%s'."),
			*FunctionDef.FunctionName.ToString(),
			*FunctionDef.TargetBlueprintPath);
	}

	UK2Node_FunctionEntry* EntryNode = nullptr;
	TArray<UK2Node_FunctionResult*> ResultNodes;
	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (UK2Node_FunctionEntry* AsEntry = Cast<UK2Node_FunctionEntry>(Node))
		{
			EntryNode = AsEntry;
		}
		else if (UK2Node_FunctionResult* AsResult = Cast<UK2Node_FunctionResult>(Node))
		{
			ResultNodes.Add(AsResult);
		}
	}

	if (!EntryNode)
	{
		EntryNode = SpawnFunctionEntryNode(FunctionGraph, FVector2D::ZeroVector);
		if (!EntryNode)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = TEXT("ApplyFunctionSkeleton: Failed to create function entry node.");
			UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
			return Result;
		}
	}

	if (ResultNodes.Num() == 0)
	{
		if (UK2Node_FunctionResult* NewResult = SpawnFunctionResultNode(FunctionGraph, FVector2D::ZeroVector))
		{
			ResultNodes.Add(NewResult);
		}
	}

	// Remove any remaining nodes before shaping the entry/result pins so the
	// compiler triggered later doesn't try to recreate stale variable pins.
	{
		TArray<UEdGraphNode*> NodesToRemove;
		for (UEdGraphNode* Node : FunctionGraph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			if (Node == EntryNode)
			{
				continue;
			}

			if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
			{
				if (ResultNodes.Contains(ResultNode))
				{
					continue;
				}
			}

			NodesToRemove.Add(Node);
		}

		for (UEdGraphNode* NodeToRemove : NodesToRemove)
		{
			FunctionGraph->RemoveNode(NodeToRemove);
		}
	}

	if (ResultNodes.Num() == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyFunctionSkeleton: Failed to create function result node.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	TArray<FString> PinWarnings;
	ClearNonExecPins(EntryNode);
	const int32 InputPinsAdded = AddPinsFromSpec(EntryNode, EGPD_Input, FunctionDef.Inputs, PinWarnings);
	int32 OutputPinsAdded = 0;
	for (UK2Node_FunctionResult* ResultNode : ResultNodes)
	{
		ClearNonExecPins(ResultNode);
		OutputPinsAdded += AddPinsFromSpec(ResultNode, EGPD_Output, FunctionDef.Outputs, PinWarnings);
	}

	for (const FString& Warning : PinWarnings)
	{
		Result.Warnings.Add(Warning);
	}

	if (FunctionDef.Inputs.Num() > 0 || FunctionDef.Outputs.Num() > 0)
	{
		UE_LOG(LogSOTS_BlueprintGen, Display,
			TEXT("ApplyFunctionSkeleton: Added %d input pin(s) and %d output pin(s) to function '%s'."),
			InputPinsAdded,
			OutputPinsAdded,
			*FunctionDef.FunctionName.ToString());
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (UPackage* Package = Blueprint->GetOutermost())
	{
		Package->MarkPackageDirty();
	}
	if (!SaveBlueprint(Blueprint))
	{
		Result.Warnings.Add(TEXT("ApplyFunctionSkeleton: Failed to save Blueprint after creating function."));
	}

	Result.bSuccess = true;

	return Result;
}

FSOTS_BPGenApplyResult USOTS_BPGenBuilder::ApplyGraphSpecToFunction(
	const UObject* WorldContextObject,
	const FSOTS_BPGenFunctionDef& FunctionDef,
	const FSOTS_BPGenGraphSpec& GraphSpec)
{
	FSOTS_BPGenApplyResult Result;
	Result.TargetBlueprintPath = FunctionDef.TargetBlueprintPath;
	Result.FunctionName = FunctionDef.FunctionName;

	if (FunctionDef.TargetBlueprintPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyGraphSpecToFunction: TargetBlueprintPath is empty.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	if (FunctionDef.FunctionName.IsNone())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("ApplyGraphSpecToFunction: FunctionName is None.");
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(
		StaticLoadObject(UBlueprint::StaticClass(), nullptr, *FunctionDef.TargetBlueprintPath));

	if (!Blueprint)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(
			TEXT("ApplyGraphSpecToFunction: Failed to load Blueprint at '%s'."),
			*FunctionDef.TargetBlueprintPath);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	UEdGraph* FunctionGraph = FindFunctionGraph(Blueprint, FunctionDef.FunctionName);
	if (!FunctionGraph)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(
			TEXT("ApplyGraphSpecToFunction: Function graph '%s' not found in '%s'. Did you call ApplyFunctionSkeleton first?"),
			*FunctionDef.FunctionName.ToString(),
			*FunctionDef.TargetBlueprintPath);
		UE_LOG(LogSOTS_BlueprintGen, Error, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}

	// Ensure editor-only node modules are loaded so LoadObject lookups succeed when spawning nodes by class path.
	EnsureBlueprintNodeModulesLoaded();

	// Identify existing entry/result nodes before clearing anything.
	UK2Node_FunctionEntry* EntryNode = nullptr;
	TArray<UK2Node_FunctionResult*> ResultNodes;

	for (UEdGraphNode* Node : FunctionGraph->Nodes)
	{
		if (UK2Node_FunctionEntry* AsEntry = Cast<UK2Node_FunctionEntry>(Node))
		{
			if (!EntryNode)
			{
				EntryNode = AsEntry;
			}
		}
		else if (UK2Node_FunctionResult* AsResult = Cast<UK2Node_FunctionResult>(Node))
		{
			ResultNodes.Add(AsResult);
		}
	}

	// Remove all non-entry/result nodes to get a clean slate.
	{
		TArray<UEdGraphNode*> NodesToRemove;
		for (UEdGraphNode* Node : FunctionGraph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			if (Node == EntryNode || ResultNodes.Contains(Cast<UK2Node_FunctionResult>(Node)))
			{
				continue;
			}

			NodesToRemove.Add(Node);
		}

		for (UEdGraphNode* Node : NodesToRemove)
		{
			FunctionGraph->RemoveNode(Node);
		}
	}

	// Ensure we have entry/result nodes to work with.
	if (!EntryNode)
	{
		EntryNode = SpawnFunctionEntryNode(FunctionGraph, FVector2D::ZeroVector);
	}

	if (ResultNodes.Num() == 0)
	{
		if (UK2Node_FunctionResult* NewResult = SpawnFunctionResultNode(FunctionGraph, FVector2D::ZeroVector))
		{
			ResultNodes.Add(NewResult);
		}
	}

	// Map node ids to spawned/located nodes.
	TMap<FString, UEdGraphNode*> NodeMap;

	AddNodeToMap(NodeMap, TEXT("Entry"), EntryNode);
	AddNodeToMap(NodeMap, TEXT("FunctionEntry"), EntryNode);
	if (ResultNodes.Num() > 0)
	{
		AddNodeToMap(NodeMap, TEXT("Result"), ResultNodes[0]);
		AddNodeToMap(NodeMap, TEXT("FunctionResult"), ResultNodes[0]);
	}
	for (int32 Index = 0; Index < ResultNodes.Num(); ++Index)
	{
		AddNodeToMap(NodeMap, FString::Printf(TEXT("Result%d"), Index), ResultNodes[Index]);
	}

	// First, map any spec nodes that reference the existing entry/result nodes.
	for (const FSOTS_BPGenGraphNode& NodeSpec : GraphSpec.Nodes)
	{
		if (NodeSpec.NodeType == FName(TEXT("K2Node_FunctionEntry")))
		{
			if (EntryNode)
			{
				AddNodeToMap(NodeMap, NodeSpec.Id, EntryNode);
				EntryNode->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
				EntryNode->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);
				ApplyExtraPinDefaults(EntryNode, NodeSpec);
			}
			else
			{
				UE_LOG(LogSOTS_BlueprintGen, Warning,
					TEXT("ApplyGraphSpecToFunction: Node '%s' requests K2Node_FunctionEntry but none found in graph."),
					*NodeSpec.Id);
			}
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_FunctionResult")))
		{
			if (ResultNodes.Num() > 0)
			{
				// For now, bind to the first result node. More complex mappings
				// can be added later if needed.
				AddNodeToMap(NodeMap, NodeSpec.Id, ResultNodes[0]);
				ResultNodes[0]->NodePosX = static_cast<int32>(NodeSpec.NodePosition.X);
				ResultNodes[0]->NodePosY = static_cast<int32>(NodeSpec.NodePosition.Y);
				ApplyExtraPinDefaults(ResultNodes[0], NodeSpec);
			}
			else
			{
				UE_LOG(LogSOTS_BlueprintGen, Warning,
					TEXT("ApplyGraphSpecToFunction: Node '%s' requests K2Node_FunctionResult but none found in graph."),
					*NodeSpec.Id);
			}
		}
	}

	// Spawn new nodes for all remaining specs.
	for (const FSOTS_BPGenGraphNode& NodeSpec : GraphSpec.Nodes)
	{
		if (NodeMap.Contains(NodeSpec.Id))
		{
			continue; // Already mapped (entry/result).
		}

		UEdGraphNode* NewNode = nullptr;

		if (NodeSpec.NodeType == FName(TEXT("K2Node_CallFunction")))
		{
			NewNode = SpawnCallFunctionNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_AddComponentByClass")))
		{
			NewNode = SpawnAddComponentByClassNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_CallArrayFunction")))
		{
			NewNode = SpawnArrayFunctionNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_MakeArray")))
		{
			NewNode = SpawnMakeArrayNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_MakeStruct")))
		{
			NewNode = SpawnMakeStructNode(FunctionGraph, NodeSpec);
			if (!NewNode)
			{
				Result.Warnings.Add(FString::Printf(
					TEXT("Failed to spawn K2Node_MakeStruct for node '%s'."),
					*NodeSpec.Id));
			}
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_BreakStruct")))
		{
			NewNode = SpawnBreakStructNode(FunctionGraph, NodeSpec);
			if (!NewNode)
			{
				Result.Warnings.Add(FString::Printf(
					TEXT("Failed to spawn K2Node_BreakStruct for node '%s'."),
					*NodeSpec.Id));
			}
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_VariableGet")))
		{
			NewNode = SpawnVariableGetNode(FunctionGraph, NodeSpec, Blueprint);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_VariableSet")))
		{
			NewNode = SpawnVariableSetNode(FunctionGraph, NodeSpec, Blueprint);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_Branch"))
			|| NodeSpec.NodeType == FName(TEXT("K2Node_IfThenElse")))
		{
			NewNode = SpawnBranchNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_Knot")))
		{
			NewNode = SpawnKnotNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_ExecutionSequence")))
		{
			NewNode = SpawnExecutionSequenceNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_ForLoopWithBreak")))
		{
			NewNode = SpawnForLoopWithBreakNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_ForLoop")))
		{
			NewNode = nullptr; // ForLoop node intentionally disabled
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_ForEachLoopWithBreak")))
		{
			NewNode = SpawnForEachLoopWithBreakNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_ForEachElementInArray")))
		{
			NewNode = nullptr; // ForEachElementInArray node disabled
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_ForEachLoop")))
		{
			NewNode = SpawnForEachLoopNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_DoOnce")))
		{
			NewNode = SpawnDoOnceNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_Gate")))
		{
			NewNode = SpawnGateNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_AssignDelegate")))
		{
			NewNode = SpawnAssignDelegateNode(FunctionGraph, NodeSpec, Blueprint);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_MultiGate")))
		{
			NewNode = SpawnMultiGateNode(FunctionGraph, NodeSpec, GraphSpec.Links);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_FlipFlop")))
		{
			NewNode = SpawnFlipFlopNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_EnumLiteral")))
		{
			NewNode = SpawnEnumLiteralNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_SwitchEnum")))
		{
			NewNode = SpawnSwitchEnumNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_SwitchInteger")))
		{
			NewNode = SpawnSwitchIntegerNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_SwitchString")))
		{
			NewNode = SpawnSwitchStringNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_NameLiteral")))
		{
			NewNode = SpawnNameLiteralNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_SwitchName")))
		{
			NewNode = SpawnSwitchNameNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_MakeLiteralGameplayTag"))
			|| NodeSpec.NodeType == FName(TEXT("GameplayTagsK2Node_LiteralGameplayTag")))
		{
			NewNode = SpawnMakeLiteralGameplayTagNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_SwitchGameplayTag"))
			|| NodeSpec.NodeType == FName(TEXT("GameplayTagsK2Node_SwitchGameplayTag")))
		{
			NewNode = SpawnSwitchGameplayTagNode(FunctionGraph, NodeSpec);
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_Select")))
		{
			NewNode = SpawnSelectNode(FunctionGraph, NodeSpec);
			if (!NewNode)
			{
				Result.Warnings.Add(FString::Printf(
					TEXT("Failed to spawn K2Node_Select for node '%s'."),
					*NodeSpec.Id));
			}
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_Event")))
		{
			NewNode = SpawnEventNode(FunctionGraph, NodeSpec, Blueprint);
			if (!NewNode)
			{
				Result.Warnings.Add(FString::Printf(
					TEXT("Failed to spawn K2Node_Event for node '%s'."),
					*NodeSpec.Id));
			}
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_CustomEvent")))
		{
			NewNode = SpawnCustomEventNode(FunctionGraph, NodeSpec);
			if (!NewNode)
			{
				Result.Warnings.Add(FString::Printf(
					TEXT("Failed to spawn K2Node_CustomEvent for node '%s'."),
					*NodeSpec.Id));
			}
		}
		else if (NodeSpec.NodeType == FName(TEXT("K2Node_DynamicCast")))
		{
			NewNode = SpawnDynamicCastNode(FunctionGraph, NodeSpec);
			if (!NewNode)
			{
				Result.Warnings.Add(FString::Printf(
					TEXT("Failed to spawn K2Node_DynamicCast for node '%s'."),
					*NodeSpec.Id));
			}
		}
		else
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("ApplyGraphSpecToFunction: Unsupported NodeType '%s' for node '%s'."),
				*NodeSpec.NodeType.ToString(), *NodeSpec.Id);
			Result.Warnings.Add(FString::Printf(
				TEXT("Unsupported NodeType '%s' for node '%s'."),
				*NodeSpec.NodeType.ToString(), *NodeSpec.Id));
		}

		if (NewNode)
		{
			AddNodeToMap(NodeMap, NodeSpec.Id, NewNode);
		}
	}

	// Connect pins according to Links.
	for (const FSOTS_BPGenGraphLink& Link : GraphSpec.Links)
	{
		UEdGraphNode** FromNodePtr = NodeMap.Find(Link.FromNodeId);
		UEdGraphNode** ToNodePtr = NodeMap.Find(Link.ToNodeId);

		if (!FromNodePtr || !ToNodePtr || !(*FromNodePtr) || !(*ToNodePtr))
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("ApplyGraphSpecToFunction: Link from '%s' to '%s' could not find nodes."),
				*Link.FromNodeId, *Link.ToNodeId);
			Result.Warnings.Add(FString::Printf(
				TEXT("Link from '%s' to '%s' could not find nodes."),
				*Link.FromNodeId, *Link.ToNodeId));
			continue;
		}

		UEdGraphPin* FromPin = FindPinByName(*FromNodePtr, Link.FromPinName);
		UEdGraphPin* ToPin = FindPinByName(*ToNodePtr, Link.ToPinName);

		if (!ToPin && ToNodePtr && *ToNodePtr)
		{
			if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(*ToNodePtr))
			{
				ToPin = FindPinByName(*ToNodePtr, UEdGraphSchema_K2::PN_Execute);
				if (ToPin)
				{
					UE_LOG(LogSOTS_BlueprintGen, Verbose,
						TEXT("ApplyGraphSpecToFunction: Link '%s' fell back to Execute pin on result node."),
						*Link.ToNodeId);
				}
			}
		}

		if (!FromPin || !ToPin)
		{
			UE_LOG(LogSOTS_BlueprintGen, Warning,
				TEXT("ApplyGraphSpecToFunction: Link from '%s.%s' to '%s.%s' could not find pins."),
				*Link.FromNodeId,
				*Link.FromPinName.ToString(),
				*Link.ToNodeId,
				*Link.ToPinName.ToString());
			Result.Warnings.Add(FString::Printf(
				TEXT("Link from '%s.%s' to '%s.%s' could not find pins."),
				*Link.FromNodeId,
				*Link.FromPinName.ToString(),
				*Link.ToNodeId,
				*Link.ToPinName.ToString()));
			continue;
		}

		if (!ValidateLinkPins(Link, FromPin, ToPin, Result))
		{
			continue;
		}

		FromPin->MakeLinkTo(ToPin);

		if (UEdGraphNode* OwningNode = FromPin->GetOwningNode())
		{
			OwningNode->NodeConnectionListChanged();
		}
		if (UEdGraphNode* OwningNode = ToPin->GetOwningNode())
		{
			OwningNode->NodeConnectionListChanged();
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (UPackage* Package = Blueprint->GetOutermost())
	{
		Package->MarkPackageDirty();
	}
	if (!SaveBlueprint(Blueprint))
	{
		Result.Warnings.Add(TEXT("ApplyGraphSpecToFunction: Failed to save Blueprint after applying graph."));
	}

	Result.bSuccess = true;
	return Result;
}

FSOTS_BPGenApplyResult USOTS_BPGenBuilder::BuildTestAllNodesGraphForBPPrintHello()
{
	static const TCHAR* TestBlueprintPath = TEXT("/Game/DevTools/BP_PrintHello.BP_PrintHello");
	static const FName TestFunctionName(TEXT("Test_AllNodesGraph"));

	FSOTS_BPGenFunctionDef FunctionDef;
	FunctionDef.TargetBlueprintPath = TestBlueprintPath;
	FunctionDef.FunctionName = TestFunctionName;

	FSOTS_BPGenGraphSpec GraphSpec;
	GraphSpec.Nodes.Reserve(16);
	GraphSpec.Links.Reserve(12);

	auto AddNode = [&GraphSpec](
		const FString& Id,
		const TCHAR* NodeType,
		const FVector2D& Position) -> FSOTS_BPGenGraphNode&
	{
		FSOTS_BPGenGraphNode& Node = GraphSpec.Nodes.AddDefaulted_GetRef();
		Node.Id = Id;
		Node.NodeType = FName(NodeType);
		Node.NodePosition = Position;
		return Node;
	};

	AddNode(TEXT("Entry"), TEXT("K2Node_FunctionEntry"), FVector2D(-500.f, -100.f));
	AddNode(TEXT("Result"), TEXT("K2Node_FunctionResult"), FVector2D(800.f, 0.f));

	AddNode(TEXT("Branch"), TEXT("K2Node_IfThenElse"), FVector2D(-150.f, -50.f));

	FSOTS_BPGenGraphNode& PrintStringNode =
		AddNode(TEXT("PrintString"), TEXT("K2Node_CallFunction"), FVector2D(150.f, -50.f));
	PrintStringNode.FunctionPath = TEXT("/Script/Engine.KismetSystemLibrary:PrintString");
	PrintStringNode.ExtraData.Add(TEXT("InString"), TEXT("BPGen AllNodes Test"));

	AddNode(TEXT("TestInt_Get"), TEXT("K2Node_VariableGet"), FVector2D(-500.f, 50.f)).VariableName = TEXT("TestInt");

	FSOTS_BPGenGraphNode& TestIntSetNode =
		AddNode(TEXT("TestInt_Set"), TEXT("K2Node_VariableSet"), FVector2D(350.f, 150.f));
	TestIntSetNode.VariableName = TEXT("TestInt");

	FSOTS_BPGenGraphNode& TestBoolGetNode =
		AddNode(TEXT("TestBool_Get"), TEXT("K2Node_VariableGet"), FVector2D(-500.f, -200.f));
	TestBoolGetNode.VariableName = TEXT("bTestBool");

	FSOTS_BPGenGraphNode& TestBoolSetNode =
		AddNode(TEXT("TestBool_Set"), TEXT("K2Node_VariableSet"), FVector2D(450.f, -50.f));
	TestBoolSetNode.VariableName = TEXT("bTestBool");
	TestBoolSetNode.ExtraData.Add(TEXT("bTestBool"), TEXT("true"));

	FSOTS_BPGenGraphNode& TestStringGetNode =
		AddNode(TEXT("TestString_Get"), TEXT("K2Node_VariableGet"), FVector2D(-500.f, 125.f));
	TestStringGetNode.VariableName = TEXT("TestString");

	FSOTS_BPGenGraphNode& StringArrayGetNode =
		AddNode(TEXT("TestStringArray_Get"), TEXT("K2Node_VariableGet"), FVector2D(-200.f, 200.f));
	StringArrayGetNode.VariableName = TEXT("TestStringArray");
	StringArrayGetNode.ExtraData.Add(TEXT("PinCategory"), TEXT("string"));
	StringArrayGetNode.ExtraData.Add(TEXT("ContainerType"), TEXT("Array"));

	FSOTS_BPGenGraphNode& StringArraySetNode =
		AddNode(TEXT("TestStringArray_Set"), TEXT("K2Node_VariableSet"), FVector2D(150.f, 325.f));
	StringArraySetNode.VariableName = TEXT("TestStringArray");
	StringArraySetNode.ExtraData.Add(TEXT("PinCategory"), TEXT("string"));
	StringArraySetNode.ExtraData.Add(TEXT("ContainerType"), TEXT("Array"));

	FSOTS_BPGenGraphNode& ArrayLengthNode =
		AddNode(TEXT("ArrayLength"), TEXT("K2Node_CallArrayFunction"), FVector2D(100.f, 200.f));
	ArrayLengthNode.FunctionPath = TEXT("/Script/Engine.KismetArrayLibrary:Array_Length");

	AddNode(TEXT("KnotNode"), TEXT("K2Node_Knot"), FVector2D(-300.f, -200.f));

	auto AddLink = [&GraphSpec](
		const FString& FromNode,
		const FName& FromPin,
		const FString& ToNode,
		const FName& ToPin)
	{
		FSOTS_BPGenGraphLink& Link = GraphSpec.Links.AddDefaulted_GetRef();
		Link.FromNodeId = FromNode;
		Link.FromPinName = FromPin;
		Link.ToNodeId = ToNode;
		Link.ToPinName = ToPin;
	};

	AddLink(TEXT("Entry"), UEdGraphSchema_K2::PN_Then, TEXT("Branch"), UEdGraphSchema_K2::PN_Execute);
	AddLink(TEXT("Branch"), UEdGraphSchema_K2::PN_Then, TEXT("PrintString"), UEdGraphSchema_K2::PN_Execute);
	AddLink(TEXT("PrintString"), UEdGraphSchema_K2::PN_Then, TEXT("TestStringArray_Set"), UEdGraphSchema_K2::PN_Execute);
	AddLink(TEXT("TestStringArray_Set"), UEdGraphSchema_K2::PN_Then, TEXT("TestBool_Set"), UEdGraphSchema_K2::PN_Execute);
	AddLink(TEXT("TestBool_Set"), UEdGraphSchema_K2::PN_Then, TEXT("Result"), UEdGraphSchema_K2::PN_Execute);
	AddLink(TEXT("Branch"), UEdGraphSchema_K2::PN_Else, TEXT("TestInt_Set"), UEdGraphSchema_K2::PN_Execute);
	AddLink(TEXT("TestInt_Set"), UEdGraphSchema_K2::PN_Then, TEXT("Result"), UEdGraphSchema_K2::PN_Execute);

	AddLink(TEXT("TestBool_Get"), TEXT("bTestBool"), TEXT("KnotNode"), TEXT("InputPin"));
	AddLink(TEXT("KnotNode"), TEXT("OutputPin"), TEXT("Branch"), TEXT("Condition"));
	AddLink(TEXT("TestString_Get"), TEXT("TestString"), TEXT("PrintString"), TEXT("InString"));
	AddLink(TEXT("TestStringArray_Get"), TEXT("TestStringArray"), TEXT("TestStringArray_Set"), TEXT("TestStringArray"));
	AddLink(TEXT("TestStringArray_Set"), TEXT("TestStringArray"), TEXT("ArrayLength"), TEXT("TargetArray"));
	AddLink(TEXT("ArrayLength"), TEXT("ReturnValue"), TEXT("TestInt_Set"), TEXT("TestInt"));

	FSOTS_BPGenApplyResult SkeletonResult = ApplyFunctionSkeleton(nullptr, FunctionDef);
	if (!SkeletonResult.bSuccess)
	{
		return SkeletonResult;
	}

	FSOTS_BPGenApplyResult GraphResult = ApplyGraphSpecToFunction(nullptr, FunctionDef, GraphSpec);
	GraphResult.Warnings.Append(SkeletonResult.Warnings);
	return GraphResult;
}
