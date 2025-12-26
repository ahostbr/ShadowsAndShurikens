#include "SOTS_BPGenBridgeComponentOps.h"

#include "JsonObjectConverter.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "EditorAssetLibrary.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include "Misc/PackageName.h"

#include "SOTS_BPGenBridgePrivateHelpers.h"

namespace
{
	static FSOTS_BPGenBridgeComponentOpResult MakeComponentOpError(const FString& ErrorCode, const FString& ErrorMessage)
	{
		FSOTS_BPGenBridgeComponentOpResult R;
		R.bOk = false;
		R.ErrorCode = ErrorCode;
		R.Errors.Add(ErrorMessage);
		R.Result = MakeShared<FJsonObject>();
		return R;
	}

	static FSOTS_BPGenBridgeComponentOpResult MakeComponentOpError(const TCHAR* ErrorCode, const TCHAR* ErrorMessage)
	{
		return MakeComponentOpError(FString(ErrorCode), FString(ErrorMessage));
	}

	static FSOTS_BPGenBridgeComponentOpResult MakeComponentOpError(const TCHAR* ErrorCode, const FString& ErrorMessage)
	{
		return MakeComponentOpError(FString(ErrorCode), ErrorMessage);
	}

	static FSOTS_BPGenBridgeComponentOpResult MakeComponentOpError(const FString& ErrorCode, const TCHAR* ErrorMessage)
	{
		return MakeComponentOpError(ErrorCode, FString(ErrorMessage));
	}

	static UBlueprint* LoadBlueprintByPath(const FString& BlueprintName, FString& OutNormalizedObjectPath, FSOTS_BPGenBridgeComponentOpResult& OutError)
	{
		OutNormalizedObjectPath = SOTS_BPGenBridgePrivate::NormalizeAssetObjectPath(BlueprintName);
		if (OutNormalizedObjectPath.IsEmpty())
		{
			OutError = MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("Missing blueprint_name"));
			return nullptr;
		}

		UObject* Obj = UEditorAssetLibrary::LoadAsset(OutNormalizedObjectPath);
		UBlueprint* BP = Cast<UBlueprint>(Obj);
		if (!BP)
		{
			OutError = MakeComponentOpError(TEXT("BLUEPRINT_NOT_FOUND"), FString::Printf(TEXT("Failed to load Blueprint: %s"), *OutNormalizedObjectPath));
			return nullptr;
		}
		return BP;
	}

	static bool ResolveComponentClassByName(const FString& ComponentTypeName, bool bIncludeAbstract, bool bIncludeDeprecated, UClass*& OutClass)
	{
		OutClass = nullptr;

		FString CleanName = ComponentTypeName;
		CleanName.TrimStartAndEndInline();
		if (CleanName.IsEmpty())
		{
			return false;
		}

		int32 DotIndex = INDEX_NONE;
		if (CleanName.FindLastChar('.', DotIndex))
		{
			CleanName = CleanName.Mid(DotIndex + 1);
		}

		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			if (!Class || !Class->IsChildOf(UActorComponent::StaticClass()))
			{
				continue;
			}

			const bool bIsAbstract = Class->HasAnyClassFlags(CLASS_Abstract);
			if (bIsAbstract && !bIncludeAbstract)
			{
				continue;
			}

			const bool bIsDeprecated = Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists);
			if (bIsDeprecated && !bIncludeDeprecated)
			{
				continue;
			}

			const FString Name = Class->GetName();
			const FString Display = Class->GetDisplayNameText().ToString();
			if (Name.Equals(CleanName, ESearchCase::IgnoreCase) || Display.Equals(CleanName, ESearchCase::IgnoreCase) || Name.Equals(ComponentTypeName, ESearchCase::IgnoreCase))
			{
				OutClass = Class;
				return true;
			}
		}

		return false;
	}

	static USCS_Node* FindNodeByComponentName(USimpleConstructionScript* SCS, const FString& ComponentName)
	{
		if (!SCS)
		{
			return nullptr;
		}

		FString Name = ComponentName;
		Name.TrimStartAndEndInline();
		if (Name.IsEmpty())
		{
			return nullptr;
		}

		if (USCS_Node* Found = SCS->FindSCSNode(*Name))
		{
			return Found;
		}

		// Fallback: some callers may provide display/variable names with odd casing.
		for (USCS_Node* Node : SCS->GetAllNodes())
		{
			if (Node && Node->GetVariableName().ToString().Equals(Name, ESearchCase::IgnoreCase))
			{
				return Node;
			}
		}

		return nullptr;
	}

	static void RemoveNodeRecursive(USimpleConstructionScript* SCS, USCS_Node* Node)
	{
		if (!SCS || !Node)
		{
			return;
		}

		const TArray<USCS_Node*>& ChildRefs = Node->GetChildNodes();
		TArray<USCS_Node*> ChildNodes(ChildRefs);

		for (USCS_Node* ChildNode : ChildNodes)
		{
			RemoveNodeRecursive(SCS, ChildNode);
		}

		SCS->RemoveNode(Node);
	}

	static void BuildComponentInfo(USCS_Node* Node, TArray<TSharedPtr<FJsonValue>>& OutComponents)
	{
		if (!Node)
		{
			return;
		}

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("component_name"), Node->GetVariableName().ToString());
		Obj->SetStringField(TEXT("component_type"), Node->ComponentClass ? Node->ComponentClass->GetName() : TEXT("Unknown"));
		Obj->SetStringField(TEXT("parent_name"), Node->ParentComponentOrVariableName.ToString());
		Obj->SetBoolField(TEXT("is_scene_component"), Node->ComponentClass && Node->ComponentClass->IsChildOf<USceneComponent>());

		if (USceneComponent* SceneComp = Cast<USceneComponent>(Node->ComponentTemplate))
		{
			const FTransform T = SceneComp->GetRelativeTransform();
			Obj->SetStringField(TEXT("relative_transform"), T.ToString());
		}

		TArray<TSharedPtr<FJsonValue>> Children;
		for (USCS_Node* Child : Node->GetChildNodes())
		{
			if (Child)
			{
				Children.Add(MakeShared<FJsonValueString>(Child->GetVariableName().ToString()));
			}
		}
		Obj->SetArrayField(TEXT("children"), Children);

		OutComponents.Add(MakeShared<FJsonValueObject>(Obj));

		for (USCS_Node* Child : Node->GetChildNodes())
		{
			BuildComponentInfo(Child, OutComponents);
		}
	}



	static void AddPropertyMetadataEntry(TArray<TSharedPtr<FJsonValue>>& Out, FProperty* Prop)
	{
		if (!Prop)
		{
			return;
		}

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), Prop->GetName());
		Obj->SetStringField(TEXT("type"), Prop->GetCPPType());

		const FName CategoryFName(TEXT("Category"));
		if (Prop->HasMetaData(CategoryFName))
		{
			Obj->SetStringField(TEXT("category"), Prop->GetMetaData(CategoryFName));
		}

		const FName ToolTipFName(TEXT("ToolTip"));
		if (Prop->HasMetaData(ToolTipFName))
		{
			Obj->SetStringField(TEXT("tooltip"), Prop->GetMetaData(ToolTipFName));
		}

		Obj->SetBoolField(TEXT("editable"), Prop->HasAnyPropertyFlags(CPF_Edit));
		Obj->SetBoolField(TEXT("blueprint_visible"), Prop->HasAnyPropertyFlags(CPF_BlueprintVisible));

		Out.Add(MakeShared<FJsonValueObject>(Obj));
	}
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::SearchTypes(const TSharedPtr<FJsonObject>& Params)
{
	FString Category;
	FString SearchText;
	FString BaseClass;
	bool bIncludeAbstract = false;
	bool bIncludeDeprecated = false;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("category"), Category);
		Params->TryGetStringField(TEXT("search_text"), SearchText);
		Params->TryGetStringField(TEXT("base_class"), BaseClass);
		Params->TryGetBoolField(TEXT("include_abstract"), bIncludeAbstract);
		Params->TryGetBoolField(TEXT("include_deprecated"), bIncludeDeprecated);
	}

	Category.TrimStartAndEndInline();
	SearchText.TrimStartAndEndInline();
	BaseClass.TrimStartAndEndInline();

	UClass* BaseFilter = nullptr;
	if (!BaseClass.IsEmpty())
	{
		ResolveComponentClassByName(BaseClass, true, true, BaseFilter);
	}

	TArray<TSharedPtr<FJsonValue>> Types;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class || !Class->IsChildOf(UActorComponent::StaticClass()))
		{
			continue;
		}

		if (!bIncludeAbstract && Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		if (!bIncludeDeprecated && Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}

		if (BaseFilter && !Class->IsChildOf(BaseFilter))
		{
			continue;
		}

		const FString Name = Class->GetName();
		const FString Display = Class->GetDisplayNameText().ToString();

		if (!SearchText.IsEmpty())
		{
			if (!Name.Contains(SearchText, ESearchCase::IgnoreCase) && !Display.Contains(SearchText, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		if (!Category.IsEmpty())
		{
			// Best-effort: category filter is approximate; upstream uses a more structured palette taxonomy.
			if (!Name.Contains(Category, ESearchCase::IgnoreCase) && !Display.Contains(Category, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("name"), Name);
		Obj->SetStringField(TEXT("display_name"), Display);
		Obj->SetStringField(TEXT("class_path"), Class->GetPathName());
		Obj->SetBoolField(TEXT("is_scene_component"), Class->IsChildOf<USceneComponent>());
		Types.Add(MakeShared<FJsonValueObject>(Obj));
	}

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetArrayField(TEXT("types"), Types);
	R.Result->SetNumberField(TEXT("count"), Types.Num());
	if (!Category.IsEmpty())
	{
		R.Warnings.Add(TEXT("category filter is best-effort (substring match on class names)."));
	}
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::List(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
	}

	FSOTS_BPGenBridgeComponentOpResult Err;
	FString Normalized;
	UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
	if (!BP)
	{
		return Err;
	}

	USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
	if (!SCS)
	{
		return MakeComponentOpError(TEXT("SCS_NOT_AVAILABLE"), TEXT("Blueprint does not have a SimpleConstructionScript"));
	}

	TArray<TSharedPtr<FJsonValue>> Components;
	for (USCS_Node* Root : SCS->GetRootNodes())
	{
		BuildComponentInfo(Root, Components);
	}

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
	R.Result->SetArrayField(TEXT("components"), Components);
	R.Result->SetNumberField(TEXT("count"), Components.Num());
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::GetProperty(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	FString ComponentName;
	FString PropertyName;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
		Params->TryGetStringField(TEXT("component_name"), ComponentName);
		Params->TryGetStringField(TEXT("property_name"), PropertyName);
	}

	if (BlueprintName.TrimStartAndEnd().IsEmpty() || ComponentName.TrimStartAndEnd().IsEmpty() || PropertyName.TrimStartAndEnd().IsEmpty())
	{
		return MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("get_property requires blueprint_name, component_name, property_name"));
	}

	FSOTS_BPGenBridgeComponentOpResult Err;
	FString Normalized;
	UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
	if (!BP)
	{
		return Err;
	}

	USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
	if (!SCS)
	{
		return MakeComponentOpError(TEXT("SCS_NOT_AVAILABLE"), TEXT("Blueprint does not have a SimpleConstructionScript"));
	}

	USCS_Node* Node = FindNodeByComponentName(SCS, ComponentName);
	if (!Node || !Node->ComponentTemplate)
	{
		return MakeComponentOpError(TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	FProperty* Prop = FindFProperty<FProperty>(Node->ComponentTemplate->GetClass(), *PropertyName);
	if (!Prop)
	{
		return MakeComponentOpError(TEXT("PROPERTY_NOT_FOUND"), FString::Printf(TEXT("Property not found: %s"), *PropertyName));
	}

	const uint8* PropData = Prop->ContainerPtrToValuePtr<uint8>(Node->ComponentTemplate);
	TSharedPtr<FJsonValue> JsonValue = FJsonObjectConverter::UPropertyToJsonValue(Prop, PropData);

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
	R.Result->SetStringField(TEXT("component_name"), Node->GetVariableName().ToString());
	R.Result->SetStringField(TEXT("property"), Prop->GetName());
	R.Result->SetStringField(TEXT("type"), Prop->GetCPPType());
	if (JsonValue.IsValid())
	{
		R.Result->SetField(TEXT("value"), JsonValue);
	}
	else
	{
		FString Export;
		Prop->ExportTextItem_Direct(Export, PropData, nullptr, Node->ComponentTemplate, PPF_None);
		R.Result->SetStringField(TEXT("value"), Export);
		R.Warnings.Add(TEXT("Property serialized via ExportText fallback"));
	}
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::SetProperty(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	FString ComponentName;
	FString PropertyName;
	FString PropertyValue;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
		Params->TryGetStringField(TEXT("component_name"), ComponentName);
		Params->TryGetStringField(TEXT("property_name"), PropertyName);
		Params->TryGetStringField(TEXT("property_value"), PropertyValue);
	}

	if (BlueprintName.TrimStartAndEnd().IsEmpty() || ComponentName.TrimStartAndEnd().IsEmpty() || PropertyName.TrimStartAndEnd().IsEmpty())
	{
		return MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("set_property requires blueprint_name, component_name, property_name"));
	}

	FSOTS_BPGenBridgeComponentOpResult Err;
	FString Normalized;
	UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
	if (!BP)
	{
		return Err;
	}

	USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
	if (!SCS)
	{
		return MakeComponentOpError(TEXT("SCS_NOT_AVAILABLE"), TEXT("Blueprint does not have a SimpleConstructionScript"));
	}

	USCS_Node* Node = FindNodeByComponentName(SCS, ComponentName);
	if (!Node || !Node->ComponentTemplate)
	{
		return MakeComponentOpError(TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	FString ErrMsg;
		if (!SOTS_BPGenBridgePrivate::SetObjectPropertyByImportText(Node->ComponentTemplate, PropertyName, PropertyValue, ErrMsg))
	{
		return MakeComponentOpError(TEXT("PROPERTY_SET_FAILED"), ErrMsg);
	}

	Node->ComponentTemplate->PostEditChange();
	Node->ComponentTemplate->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
	FBlueprintEditorUtils::RefreshAllNodes(BP);

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
	R.Result->SetStringField(TEXT("component_name"), Node->GetVariableName().ToString());
	R.Result->SetStringField(TEXT("property"), PropertyName);
	R.Result->SetBoolField(TEXT("success"), true);
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::GetAllProperties(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	FString ComponentName;
	bool bIncludeInherited = true;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
		Params->TryGetStringField(TEXT("component_name"), ComponentName);
		Params->TryGetBoolField(TEXT("include_inherited"), bIncludeInherited);
	}

	if (BlueprintName.TrimStartAndEnd().IsEmpty() || ComponentName.TrimStartAndEnd().IsEmpty())
	{
		return MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("get_all_properties requires blueprint_name, component_name"));
	}

	FSOTS_BPGenBridgeComponentOpResult Err;
	FString Normalized;
	UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
	if (!BP)
	{
		return Err;
	}

	USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
	if (!SCS)
	{
		return MakeComponentOpError(TEXT("SCS_NOT_AVAILABLE"), TEXT("Blueprint does not have a SimpleConstructionScript"));
	}

	USCS_Node* Node = FindNodeByComponentName(SCS, ComponentName);
	if (!Node || !Node->ComponentTemplate)
	{
		return MakeComponentOpError(TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	TSharedPtr<FJsonObject> Props = MakeShared<FJsonObject>();
	int32 Count = 0;

	for (TFieldIterator<FProperty> It(Node->ComponentTemplate->GetClass(), bIncludeInherited ? EFieldIteratorFlags::IncludeSuper : EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop)
		{
			continue;
		}

		// Keep this manageable: prioritize editable/blueprint-visible properties.
		if (!Prop->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible | CPF_BlueprintReadOnly))
		{
			continue;
		}

		const uint8* PropData = Prop->ContainerPtrToValuePtr<uint8>(Node->ComponentTemplate);
		TSharedPtr<FJsonValue> JsonValue = FJsonObjectConverter::UPropertyToJsonValue(Prop, PropData);
		if (JsonValue.IsValid())
		{
			Props->SetField(Prop->GetName(), JsonValue);
		}
		else
		{
			FString Export;
			Prop->ExportTextItem_Direct(Export, PropData, nullptr, Node->ComponentTemplate, PPF_None);
			Props->SetStringField(Prop->GetName(), Export);
		}
		Count++;
	}

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
	R.Result->SetStringField(TEXT("component_name"), Node->GetVariableName().ToString());
	R.Result->SetObjectField(TEXT("properties"), Props);
	R.Result->SetNumberField(TEXT("property_count"), Count);
	if (!bIncludeInherited)
	{
		R.Warnings.Add(TEXT("include_inherited=false may omit many useful properties"));
	}
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::Create(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	FString ComponentName;
	FString ComponentType;
	FString ParentName;
	
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);
	bool bHasLocation = false;
	bool bHasRotation = false;
	bool bHasScale = false;

	TSharedPtr<FJsonObject> InitialProps;

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
		Params->TryGetStringField(TEXT("component_name"), ComponentName);
		Params->TryGetStringField(TEXT("component_type"), ComponentType);
		Params->TryGetStringField(TEXT("parent_name"), ParentName);

		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Params->TryGetArrayField(TEXT("location"), Arr) && Arr && Arr->Num() >= 3)
		{
			Location = FVector((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
			bHasLocation = true;
		}
		if (Params->TryGetArrayField(TEXT("rotation"), Arr) && Arr && Arr->Num() >= 3)
		{
			Rotation = FRotator((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
			bHasRotation = true;
		}
		if (Params->TryGetArrayField(TEXT("scale"), Arr) && Arr && Arr->Num() >= 3)
		{
			Scale = FVector((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
			bHasScale = true;
		}

		if (Params->HasTypedField<EJson::Object>(TEXT("properties")))
		{
			InitialProps = Params->GetObjectField(TEXT("properties"));
		}
	}

	if (BlueprintName.TrimStartAndEnd().IsEmpty() || ComponentName.TrimStartAndEnd().IsEmpty() || ComponentType.TrimStartAndEnd().IsEmpty())
	{
		return MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("create requires blueprint_name, component_name, component_type"));
	}

	FSOTS_BPGenBridgeComponentOpResult Err;
	FString Normalized;
	UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
	if (!BP)
	{
		return Err;
	}

	USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
	if (!SCS)
	{
		return MakeComponentOpError(TEXT("SCS_NOT_AVAILABLE"), TEXT("Blueprint does not have a SimpleConstructionScript"));
	}

	if (FindNodeByComponentName(SCS, ComponentName))
	{
		return MakeComponentOpError(TEXT("COMPONENT_NAME_EXISTS"), FString::Printf(TEXT("Component name already exists: %s"), *ComponentName));
	}

	UClass* ComponentClass = nullptr;
	if (!ResolveComponentClassByName(ComponentType, false, false, ComponentClass) || !ComponentClass)
	{
		return MakeComponentOpError(TEXT("COMPONENT_TYPE_INVALID"), FString::Printf(TEXT("Invalid component_type: %s"), *ComponentType));
	}

	USCS_Node* NewNode = SCS->CreateNode(ComponentClass, *ComponentName);
	if (!NewNode)
	{
		return MakeComponentOpError(TEXT("COMPONENT_CREATE_FAILED"), FString::Printf(TEXT("Failed to create component: %s"), *ComponentName));
	}

	ParentName.TrimStartAndEndInline();
	if (ParentName.Equals(TEXT("root"), ESearchCase::IgnoreCase))
	{
		ParentName.Reset();
	}

	if (!ParentName.IsEmpty())
	{
		if (USCS_Node* ParentNode = FindNodeByComponentName(SCS, ParentName))
		{
			ParentNode->AddChildNode(NewNode);
		}
		else
		{
			SCS->AddNode(NewNode);
		}
	}
	else
	{
		SCS->AddNode(NewNode);
	}

	if (ComponentClass->IsChildOf<USceneComponent>())
	{
		if (USceneComponent* SceneComp = Cast<USceneComponent>(NewNode->ComponentTemplate))
		{
			FTransform T = SceneComp->GetRelativeTransform();
			if (bHasLocation)
			{
				T.SetLocation(Location);
			}
			if (bHasRotation)
			{
				T.SetRotation(Rotation.Quaternion());
			}
			if (bHasScale)
			{
				T.SetScale3D(Scale);
			}
			SceneComp->SetRelativeTransform(T);
		}
	}

	TArray<FString> PropertyWarnings;
	if (InitialProps.IsValid() && NewNode->ComponentTemplate)
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : InitialProps->Values)
		{
			FString ValueStr;
			if (Pair.Value.IsValid())
			{
				if (Pair.Value->Type == EJson::String)
				{
					ValueStr = Pair.Value->AsString();
				}
				else if (Pair.Value->Type == EJson::Boolean)
				{
					ValueStr = Pair.Value->AsBool() ? TEXT("True") : TEXT("False");
				}
				else if (Pair.Value->Type == EJson::Number)
				{
					ValueStr = FString::SanitizeFloat(Pair.Value->AsNumber());
				}
				else
				{
					// Best-effort: caller should prefer ImportText strings.
					ValueStr = TEXT("");
				}
			}

			FString PropErr;
				if (!SOTS_BPGenBridgePrivate::SetObjectPropertyByImportText(NewNode->ComponentTemplate, Pair.Key, ValueStr, PropErr))
			{
				PropertyWarnings.Add(FString::Printf(TEXT("Failed to set property '%s': %s"), *Pair.Key, *PropErr));
			}
		}
		NewNode->ComponentTemplate->PostEditChange();
		NewNode->ComponentTemplate->MarkPackageDirty();
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FBlueprintEditorUtils::RefreshAllNodes(BP);

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
	R.Result->SetStringField(TEXT("component_name"), NewNode->GetVariableName().ToString());
	R.Result->SetStringField(TEXT("component_type"), ComponentClass->GetName());
	R.Result->SetBoolField(TEXT("success"), true);
	if (InitialProps.IsValid())
	{
		R.Warnings.Add(TEXT("Initial properties should be provided as ImportText strings for best results"));
	}
	R.Warnings.Append(PropertyWarnings);
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::Delete(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	FString ComponentName;
	bool bRemoveChildren = true;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
		Params->TryGetStringField(TEXT("component_name"), ComponentName);
		Params->TryGetBoolField(TEXT("remove_children"), bRemoveChildren);
	}

	if (BlueprintName.TrimStartAndEnd().IsEmpty() || ComponentName.TrimStartAndEnd().IsEmpty())
	{
		return MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("delete requires blueprint_name, component_name"));
	}

	FSOTS_BPGenBridgeComponentOpResult Err;
	FString Normalized;
	UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
	if (!BP)
	{
		return Err;
	}

	USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
	if (!SCS)
	{
		return MakeComponentOpError(TEXT("SCS_NOT_AVAILABLE"), TEXT("Blueprint does not have a SimpleConstructionScript"));
	}

	USCS_Node* Node = FindNodeByComponentName(SCS, ComponentName);
	if (!Node)
	{
		return MakeComponentOpError(TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	const int32 ChildrenCount = Node->GetChildNodes().Num();
	if (bRemoveChildren)
	{
		RemoveNodeRecursive(SCS, Node);
	}
	else
	{
		TArray<USCS_Node*> Children(Node->GetChildNodes());
		for (USCS_Node* Child : Children)
		{
			Node->RemoveChildNode(Child);
			SCS->AddNode(Child);
		}
		SCS->RemoveNode(Node);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FBlueprintEditorUtils::RefreshAllNodes(BP);

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
	R.Result->SetStringField(TEXT("component_name"), ComponentName);
	R.Result->SetBoolField(TEXT("success"), true);
	R.Result->SetBoolField(TEXT("removed_children"), bRemoveChildren);
	R.Result->SetNumberField(TEXT("children_count"), ChildrenCount);
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::Reparent(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	FString ComponentName;
	FString ParentName;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
		Params->TryGetStringField(TEXT("component_name"), ComponentName);
		Params->TryGetStringField(TEXT("parent_name"), ParentName);
	}

	if (BlueprintName.TrimStartAndEnd().IsEmpty() || ComponentName.TrimStartAndEnd().IsEmpty() || ParentName.TrimStartAndEnd().IsEmpty())
	{
		return MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("reparent requires blueprint_name, component_name, parent_name"));
	}

	FSOTS_BPGenBridgeComponentOpResult Err;
	FString Normalized;
	UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
	if (!BP)
	{
		return Err;
	}

	USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
	if (!SCS)
	{
		return MakeComponentOpError(TEXT("SCS_NOT_AVAILABLE"), TEXT("Blueprint does not have a SimpleConstructionScript"));
	}

	USCS_Node* ChildNode = FindNodeByComponentName(SCS, ComponentName);
	if (!ChildNode)
	{
		return MakeComponentOpError(TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	USCS_Node* NewParentNode = FindNodeByComponentName(SCS, ParentName);
	if (!NewParentNode)
	{
		return MakeComponentOpError(TEXT("PARENT_COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Parent component not found: %s"), *ParentName));
	}

	if (!NewParentNode->ComponentTemplate || !NewParentNode->ComponentTemplate->IsA<USceneComponent>())
	{
		return MakeComponentOpError(TEXT("PARENT_NOT_SCENE_COMPONENT"), FString::Printf(TEXT("Parent is not a SceneComponent: %s"), *ParentName));
	}

	const FString OldParent = ChildNode->ParentComponentOrVariableName.ToString();
	ChildNode->SetParent(NewParentNode);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FBlueprintEditorUtils::RefreshAllNodes(BP);

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
	R.Result->SetStringField(TEXT("component_name"), ChildNode->GetVariableName().ToString());
	R.Result->SetStringField(TEXT("old_parent"), OldParent);
	R.Result->SetStringField(TEXT("new_parent"), NewParentNode->GetVariableName().ToString());
	R.Result->SetBoolField(TEXT("success"), true);
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::Reorder(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
	}

	if (BlueprintName.TrimStartAndEnd().IsEmpty())
	{
		return MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("reorder requires blueprint_name"));
	}

	FSOTS_BPGenBridgeComponentOpResult Err;
	FString Normalized;
	UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
	if (!BP)
	{
		return Err;
	}

	// NOTE: Reordering SCS nodes robustly is non-trivial and upstream VibeUE marks this as not fully implemented.
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("blueprint_name"), Normalized);
	R.Result->SetBoolField(TEXT("success"), true);
	R.Warnings.Add(TEXT("Component reorder is not currently implemented; no changes applied."));
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::GetPropertyMetadata(const TSharedPtr<FJsonObject>& Params)
{
	FString ComponentType;
	FString PropertyName;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("component_type"), ComponentType);
		Params->TryGetStringField(TEXT("property_name"), PropertyName);
	}

	if (ComponentType.TrimStartAndEnd().IsEmpty() || PropertyName.TrimStartAndEnd().IsEmpty())
	{
		return MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("get_property_metadata requires component_type and property_name"));
	}

	UClass* ComponentClass = nullptr;
	if (!ResolveComponentClassByName(ComponentType, true, true, ComponentClass) || !ComponentClass)
	{
		return MakeComponentOpError(TEXT("COMPONENT_TYPE_INVALID"), FString::Printf(TEXT("Invalid component_type: %s"), *ComponentType));
	}

	FProperty* Prop = FindFProperty<FProperty>(ComponentClass, *PropertyName);
	if (!Prop)
	{
		return MakeComponentOpError(TEXT("PROPERTY_NOT_FOUND"), FString::Printf(TEXT("Property not found: %s"), *PropertyName));
	}

	TArray<TSharedPtr<FJsonValue>> Meta;
	AddPropertyMetadataEntry(Meta, Prop);

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = MakeShared<FJsonObject>();
	R.Result->SetStringField(TEXT("component_type"), ComponentClass->GetName());
	R.Result->SetStringField(TEXT("property_name"), Prop->GetName());
	R.Result->SetArrayField(TEXT("metadata"), Meta);
	return R;
}

FSOTS_BPGenBridgeComponentOpResult SOTS_BPGenBridgeComponentOps::GetInfo(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	FString ComponentType;
	FString ComponentName;
	bool bIncludePropertyValues = false;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
		Params->TryGetStringField(TEXT("component_type"), ComponentType);
		Params->TryGetStringField(TEXT("component_name"), ComponentName);
		Params->TryGetBoolField(TEXT("include_property_values"), bIncludePropertyValues);
	}

	if (ComponentType.TrimStartAndEnd().IsEmpty())
	{
		return MakeComponentOpError(TEXT("ERR_INVALID_PARAMS"), TEXT("get_info requires component_type"));
	}

	UClass* ComponentClass = nullptr;
	if (!ResolveComponentClassByName(ComponentType, true, true, ComponentClass) || !ComponentClass)
	{
		return MakeComponentOpError(TEXT("COMPONENT_TYPE_INVALID"), FString::Printf(TEXT("Invalid component_type: %s"), *ComponentType));
	}

	TArray<TSharedPtr<FJsonValue>> PropertyMeta;
	for (TFieldIterator<FProperty> It(ComponentClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		AddPropertyMetadataEntry(PropertyMeta, *It);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("component_type"), ComponentClass->GetName());
	ResultObj->SetStringField(TEXT("class_path"), ComponentClass->GetPathName());
	ResultObj->SetBoolField(TEXT("is_scene_component"), ComponentClass->IsChildOf<USceneComponent>());
	ResultObj->SetArrayField(TEXT("properties"), PropertyMeta);
	ResultObj->SetNumberField(TEXT("property_count"), PropertyMeta.Num());

	if (bIncludePropertyValues && !BlueprintName.TrimStartAndEnd().IsEmpty() && !ComponentName.TrimStartAndEnd().IsEmpty())
	{
		FSOTS_BPGenBridgeComponentOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (BP && BP->SimpleConstructionScript)
		{
			if (USCS_Node* Node = FindNodeByComponentName(BP->SimpleConstructionScript, ComponentName))
			{
				if (Node->ComponentTemplate)
				{
					TSharedPtr<FJsonObject> Values = MakeShared<FJsonObject>();
					int32 Count = 0;
					for (TFieldIterator<FProperty> It(Node->ComponentTemplate->GetClass(), EFieldIteratorFlags::IncludeSuper); It; ++It)
					{
						FProperty* Prop = *It;
						if (!Prop || !Prop->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible | CPF_BlueprintReadOnly))
						{
							continue;
						}
						const uint8* PropData = Prop->ContainerPtrToValuePtr<uint8>(Node->ComponentTemplate);
						TSharedPtr<FJsonValue> JsonValue = FJsonObjectConverter::UPropertyToJsonValue(Prop, PropData);
						if (JsonValue.IsValid())
						{
							Values->SetField(Prop->GetName(), JsonValue);
						}
						Count++;
					}
					ResultObj->SetObjectField(TEXT("property_values"), Values);
					ResultObj->SetNumberField(TEXT("property_values_count"), Count);
					ResultObj->SetStringField(TEXT("blueprint_name"), Normalized);
					ResultObj->SetStringField(TEXT("component_name"), Node->GetVariableName().ToString());
				}
			}
		}
	}

	FSOTS_BPGenBridgeComponentOpResult R;
	R.bOk = true;
	R.Result = ResultObj;
	if (bIncludePropertyValues)
	{
		R.Warnings.Add(TEXT("Property values are filtered to editable/blueprint-visible properties for performance"));
	}
	return R;
}
