#include "SOTS_BPGenDiscovery.h"

#include "BlueprintActionDatabase.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintVariableNodeSpawner.h"
#include "Commands/BlueprintReflection.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
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

	static FSOTS_BPGenNodeSpawnerDescriptor MakeDescriptor(UBlueprintNodeSpawner* Spawner)
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

		return Desc;
	}

	static bool ContainsKeywordIgnoreCase(const FString& Value, const TCHAR* Keyword)
	{
		return !Value.IsEmpty() && Value.Contains(Keyword, ESearchCase::IgnoreCase);
	}

	static bool HasContainerFlag(const FBlueprintReflection::FNodeSpawnerDescriptor& Source, const TCHAR* Keyword)
	{
		return ContainsKeywordIgnoreCase(Source.VariableTypePath, Keyword)
			|| ContainsKeywordIgnoreCase(Source.VariableType, Keyword);
	}

	static FString DeriveVariableContainerType(const FBlueprintReflection::FNodeSpawnerDescriptor& Source)
	{
		if (HasContainerFlag(Source, TEXT("ArrayProperty")) || HasContainerFlag(Source, TEXT("TArray")))
		{
			return TEXT("Array");
		}

		if (HasContainerFlag(Source, TEXT("SetProperty")) || HasContainerFlag(Source, TEXT("TSet")))
		{
			return TEXT("Set");
		}

		if (HasContainerFlag(Source, TEXT("MapProperty")) || HasContainerFlag(Source, TEXT("TMap")))
		{
			return TEXT("Map");
		}

		return TEXT("None");
	}

	static FSOTS_BPGenDiscoveredPinDescriptor ConvertReflectionPin(const FBlueprintReflection::FPinDescriptor& Source)
	{
		FSOTS_BPGenDiscoveredPinDescriptor Desc;

		if (!Source.Name.IsEmpty())
		{
			Desc.PinName = FName(*Source.Name);
		}

		Desc.Direction = Source.Direction.Equals(TEXT("output"), ESearchCase::IgnoreCase)
			? ESOTS_BPGenPinDirection::Output
			: ESOTS_BPGenPinDirection::Input;

		const FString CategoryName = Source.Category.IsEmpty() ? Source.Type : Source.Category;
		if (!CategoryName.IsEmpty())
		{
			Desc.PinCategory = FName(*CategoryName);
		}

		if (!Source.TypePath.IsEmpty())
		{
			Desc.PinSubCategory = FName(*Source.TypePath);
			Desc.SubObjectPath = Source.TypePath;
		}

		Desc.ContainerType = Source.bIsArray ? TEXT("Array") : TEXT("None");
		Desc.DefaultValue = Source.DefaultValue;
		Desc.bIsHidden = Source.bIsHidden;
		Desc.bIsAdvanced = Source.bIsAdvanced;
		Desc.Tooltip = Source.Tooltip;

		return Desc;
	}

	static FSOTS_BPGenNodeSpawnerDescriptor ConvertReflectionDescriptor(
		const FBlueprintReflection::FNodeSpawnerDescriptor& Source,
		bool bIncludePins)
	{
		FSOTS_BPGenNodeSpawnerDescriptor Desc;
		Desc.SpawnerKey = Source.SpawnerKey;
		Desc.DisplayName = Source.DisplayName;
		Desc.Category = Source.Category;
		Desc.Keywords = Source.Keywords;
		Desc.Tooltip = Source.Tooltip;
		Desc.NodeClassName = Source.NodeClassName;
		Desc.NodeClassPath = Source.NodeClassPath;
		Desc.NodeType = Source.NodeType;

		if (!Source.FunctionClassPath.IsEmpty() && !Source.FunctionName.IsEmpty())
		{
			Desc.FunctionPath = FString::Printf(TEXT("%s:%s"), *Source.FunctionClassPath, *Source.FunctionName);
		}
		else if (!Source.FunctionName.IsEmpty())
		{
			Desc.FunctionPath = Source.FunctionName;
		}

		if (!Source.VariableName.IsEmpty())
		{
			Desc.VariableName = FName(*Source.VariableName);
		}

		Desc.VariableOwnerClassPath = Source.OwnerClassPath;
		Desc.VariablePinCategory = Source.VariableType;
		Desc.VariablePinSubCategory = Source.VariableTypePath;
		Desc.VariablePinSubObjectPath = Source.VariableTypePath;
		Desc.VariablePinContainerType = DeriveVariableContainerType(Source);
		Desc.TargetClassPath = Source.TargetClassPath;
		Desc.bIsSynthetic = Source.bIsSynthetic;

		if (bIncludePins)
		{
			Desc.Pins.Reserve(Source.Pins.Num());
			for (const FBlueprintReflection::FPinDescriptor& Pin : Source.Pins)
			{
				Desc.Pins.Add(ConvertReflectionPin(Pin));
			}
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

	int32 AddedCount = 0;

	if (ContextBlueprint)
	{
		TArray<FBlueprintReflection::FNodeSpawnerDescriptor> ReflectionDescriptors =
			FBlueprintReflection::DiscoverNodesWithDescriptors(ContextBlueprint, SearchText, TEXT(""), TEXT(""), MaxResults);

		for (const FBlueprintReflection::FNodeSpawnerDescriptor& ReflectionDesc : ReflectionDescriptors)
		{
			if (AddedCount >= MaxResults)
			{
				break;
			}

			Result.Descriptors.Add(ConvertReflectionDescriptor(ReflectionDesc, bIncludePins));
			AddedCount++;
		}
	}
	else
	{
		FBlueprintActionDatabase& ActionDB = FBlueprintActionDatabase::Get();
		const FBlueprintActionDatabase::FActionRegistry& Registry = ActionDB.GetAllActions();

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

				FSOTS_BPGenNodeSpawnerDescriptor Desc = MakeDescriptor(Spawner);

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
	}

	Result.bSuccess = Result.Errors.Num() == 0;
	return Result;
}
