#include "SOTS_BPGenDiscovery.h"

#include "BlueprintActionDatabase.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintVariableNodeSpawner.h"
#include "BlueprintNodeBinder.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"

namespace
{
	static FString ContainerTypeToString(const FEdGraphPinType& PinType)
	{
		switch (PinType.ContainerType)
		{
		case EPinContainerType::Array: return TEXT("Array");
		case EPinContainerType::Set:   return TEXT("Set");
		case EPinContainerType::Map:   return TEXT("Map");
		default: return TEXT("None");
		}
	}

	static FSOTS_BPGenDiscoveredPinDescriptor MakePinDescriptor(const UEdGraphPin* Pin)
	{
		FSOTS_BPGenDiscoveredPinDescriptor Desc;
		if (!Pin)
		{
			return Desc;
		}

		Desc.PinName = Pin->PinName;
		Desc.Direction = (Pin->Direction == EGPD_Output) ? ESOTS_BPGenPinDirection::Output : ESOTS_BPGenPinDirection::Input;
		Desc.PinCategory = Pin->PinType.PinCategory;
		Desc.PinSubCategory = Pin->PinType.PinSubCategory;
		Desc.SubObjectPath = Pin->PinType.PinSubCategoryObject.IsValid() ? Pin->PinType.PinSubCategoryObject->GetPathName() : FString();
		Desc.ContainerType = ContainerTypeToString(Pin->PinType);
		Desc.DefaultValue = Pin->DefaultValue;
		Desc.bIsHidden = Pin->bHidden;
		Desc.bIsAdvanced = Pin->bAdvancedView;
		Desc.Tooltip = Pin->PinFriendlyName.IsEmpty() ? Pin->PinName.ToString() : Pin->PinFriendlyName.ToString();
		return Desc;
	}

	static void AddPinDescriptorsFromSpawner(UBlueprintNodeSpawner* Spawner, TArray<FSOTS_BPGenDiscoveredPinDescriptor>& OutPins)
	{
		if (!Spawner)
		{
			return;
		}

		UEdGraph* TempGraph = NewObject<UEdGraph>(GetTransientPackage(), TEXT("BPGenDiscoveryTempGraph"));
		TempGraph->Schema = UEdGraphSchema_K2::StaticClass();

		UEdGraphNode* NewNode = Spawner->Invoke(TempGraph, IBlueprintNodeBinder::FBindingSet(), FVector2D::ZeroVector);
		if (!NewNode)
		{
			return;
		}

		for (UEdGraphPin* Pin : NewNode->Pins)
		{
			OutPins.Add(MakePinDescriptor(Pin));
		}

		NewNode->MarkAsGarbage();
		TempGraph->MarkAsGarbage();
	}

	static void PopulateBaseDescriptor(UBlueprintNodeSpawner* Spawner, FSOTS_BPGenNodeSpawnerDescriptor& OutDesc)
	{
		if (!Spawner)
		{
			return;
		}

		const FBlueprintActionUiSpec UiSpec = Spawner->PrimeDefaultUiSpec();
		OutDesc.DisplayName = UiSpec.MenuName.ToString();
		OutDesc.Category = UiSpec.Category.ToString();
		OutDesc.Tooltip = UiSpec.Tooltip.ToString();
		const FString KeywordsString = UiSpec.Keywords.ToString();
		if (!KeywordsString.IsEmpty())
		{
			OutDesc.Keywords.Add(KeywordsString);
		}

		if (UClass* NodeClass = Spawner->NodeClass)
		{
			OutDesc.NodeClassName = NodeClass->GetName();
			OutDesc.NodeClassPath = NodeClass->GetPathName();
		}
	}

	static void FillFunctionDescriptor(UBlueprintFunctionNodeSpawner* FunctionSpawner, FSOTS_BPGenNodeSpawnerDescriptor& OutDesc)
	{
		if (!FunctionSpawner)
		{
			return;
		}

		if (const UFunction* Function = FunctionSpawner->GetFunction())
		{
			OutDesc.FunctionPath = Function->GetPathName();
			OutDesc.SpawnerKey = OutDesc.FunctionPath;
			OutDesc.NodeType = TEXT("function_call");
		}

		OutDesc.NodeClassName = TEXT("K2Node_CallFunction");
	}

	static void FillVariableDescriptor(UBlueprintVariableNodeSpawner* VarSpawner, FSOTS_BPGenNodeSpawnerDescriptor& OutDesc)
	{
		if (!VarSpawner)
		{
			return;
		}

		OutDesc.NodeType = VarSpawner->NodeClass && VarSpawner->NodeClass->IsChildOf<UK2Node_VariableSet>()
			? TEXT("variable_set")
			: TEXT("variable_get");

		if (const FProperty* VarProperty = VarSpawner->GetVarProperty())
		{
			OutDesc.VariableName = VarProperty->GetFName();
		}

		if (const FFieldVariant OwnerVariant = VarSpawner->GetVarOuter())
		{
			if (const UStruct* OwnerStruct = Cast<UStruct>(OwnerVariant.ToUObject()))
			{
				OutDesc.VariableOwnerClassPath = OwnerStruct->GetPathName();
			}
		}

		if (!OutDesc.VariableOwnerClassPath.IsEmpty())
		{
			OutDesc.SpawnerKey = FString::Printf(TEXT("%s:%s"), *OutDesc.VariableOwnerClassPath, *OutDesc.VariableName.ToString());
		}

		if (const FProperty* VarProperty = VarSpawner->GetVarProperty())
		{
			FEdGraphPinType PinType;
			if (const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>())
			{
				if (Schema->ConvertPropertyToPinType(VarProperty, PinType))
				{
					OutDesc.VariablePinCategory = PinType.PinCategory.ToString();
					OutDesc.VariablePinSubCategory = PinType.PinSubCategory.ToString();
					OutDesc.VariablePinSubObjectPath = PinType.PinSubCategoryObject.IsValid() ? PinType.PinSubCategoryObject->GetPathName() : FString();
					OutDesc.VariablePinContainerType = ContainerTypeToString(PinType);
				}
			}
		}
	}

	static FSOTS_BPGenNodeSpawnerDescriptor MakeDescriptor(UBlueprintNodeSpawner* Spawner, UBlueprint* ContextBP, bool bIncludePins)
	{
		FSOTS_BPGenNodeSpawnerDescriptor Desc;
		PopulateBaseDescriptor(Spawner, Desc);

		if (UBlueprintFunctionNodeSpawner* FuncSpawner = Cast<UBlueprintFunctionNodeSpawner>(Spawner))
		{
			FillFunctionDescriptor(FuncSpawner, Desc);
		}
		else if (UBlueprintVariableNodeSpawner* VarSpawner = Cast<UBlueprintVariableNodeSpawner>(Spawner))
		{
			FillVariableDescriptor(VarSpawner, Desc);
		}
		else if (Desc.NodeClassName.Equals(TEXT("K2Node_DynamicCast")))
		{
			Desc.NodeType = TEXT("cast");
			Desc.SpawnerKey = FString::Printf(TEXT("K2Node_DynamicCast|%s"), *Desc.NodeClassPath);
		}
		else
		{
			Desc.NodeType = TEXT("generic");
			Desc.SpawnerKey = Desc.NodeClassPath;
		}

		if (bIncludePins)
		{
			AddPinDescriptorsFromSpawner(Spawner, Desc.Pins);
		}

		return Desc;
	}
}

FSOTS_BPGenNodeDiscoveryResult USOTS_BPGenDiscovery::DiscoverNodesWithDescriptors(
	const UObject* WorldContextObject,
	const FString& BlueprintAssetPath,
	const FString& SearchText,
	int32 MaxResults,
	bool bIncludePins)
{
	FSOTS_BPGenNodeDiscoveryResult Result;
	Result.MaxResults = MaxResults;
	Result.SearchText = SearchText;
	Result.BlueprintAssetPath = BlueprintAssetPath;

	UBlueprint* ContextBlueprint = nullptr;
	if (!BlueprintAssetPath.IsEmpty())
	{
		ContextBlueprint = LoadObject<UBlueprint>(nullptr, *BlueprintAssetPath);
		if (!ContextBlueprint)
		{
			Result.Errors.Add(FString::Printf(TEXT("Failed to load Blueprint at '%s'."), *BlueprintAssetPath));
			return Result;
		}
	}

	FBlueprintActionDatabase& ActionDB = FBlueprintActionDatabase::Get();
	const FBlueprintActionDatabase::FActionRegistry& Registry = ActionDB.GetAllActions();

	int32 AddedCount = 0;
	for (const TPair<FObjectKey, FBlueprintActionDatabase::FActionList>& Entry : Registry)
	{
		if (AddedCount >= MaxResults)
		{
			break;
		}

		for (UBlueprintNodeSpawner* Spawner : Entry.Value)
		{
			if (!Spawner)
			{
				continue;
			}

			FSOTS_BPGenNodeSpawnerDescriptor Desc = MakeDescriptor(Spawner, ContextBlueprint, bIncludePins);

			bool bPassesSearch = SearchText.IsEmpty()
				|| Desc.DisplayName.Contains(SearchText, ESearchCase::IgnoreCase)
				|| Desc.SpawnerKey.Contains(SearchText, ESearchCase::IgnoreCase)
				|| Desc.Category.Contains(SearchText, ESearchCase::IgnoreCase)
				|| Desc.Tooltip.Contains(SearchText, ESearchCase::IgnoreCase)
				|| Desc.FunctionPath.Contains(SearchText, ESearchCase::IgnoreCase);

			if (!bPassesSearch)
			{
				continue;
			}

			Result.Descriptors.Add(Desc);
			AddedCount++;

			if (AddedCount >= MaxResults)
			{
				break;
			}
		}
	}

	Result.bSuccess = Result.Errors.Num() == 0;
	return Result;
}
