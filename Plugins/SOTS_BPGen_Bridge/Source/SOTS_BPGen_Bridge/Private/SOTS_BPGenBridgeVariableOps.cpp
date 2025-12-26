#include "SOTS_BPGenBridgeVariableOps.h"

#include "SOTS_BPGenBridgeServer.h"

#include "SOTS_BPGenEnsure.h"
#include "SOTS_BPGenTypes.h"

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

namespace
{
	static FSOTS_BPGenBridgeVariableOpResult MakeVariableOpError(const FString& Code, const FString& Message)
	{
		FSOTS_BPGenBridgeVariableOpResult Err;
		Err.bOk = false;
		Err.ErrorCode = Code;
		Err.Errors.Add(Message);
		return Err;
	}

	static UBlueprint* LoadBlueprintByPath(const FString& BlueprintName, FString& OutNormalizedObjectPath, FSOTS_BPGenBridgeVariableOpResult& OutError)
	{
		if (BlueprintName.IsEmpty())
		{
			OutError = MakeVariableOpError(TEXT("ERR_INVALID_INPUT"), TEXT("blueprint_name was empty"));
			return nullptr;
		}

		FString Normalized = BlueprintName;
		if (!Normalized.Contains(TEXT(".")))
		{
			// Allow passing just "/Game/Path/BP_Name" and normalize to object path "/Game/Path/BP_Name.BP_Name"
			const FString ObjectName = FPackageName::GetShortName(Normalized);
			Normalized = FString::Printf(TEXT("%s.%s"), *Normalized, *ObjectName);
		}

		UBlueprint* BP = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *Normalized));
		if (!BP)
		{
			OutError = MakeVariableOpError(TEXT("ERR_TARGET_NOT_FOUND"), FString::Printf(TEXT("Failed to load Blueprint '%s'"), *Normalized));
			return nullptr;
		}

		OutNormalizedObjectPath = Normalized;
		return BP;
	}

	static FString JsonValueToDefaultString(const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid() || Value->IsNull())
		{
			return FString();
		}

		switch (Value->Type)
		{
		case EJson::String:
			return Value->AsString();
		case EJson::Number:
			return FString::SanitizeFloat(Value->AsNumber());
		case EJson::Boolean:
			return Value->AsBool() ? TEXT("true") : TEXT("false");
		default:
			break;
		}

		// Fallback: serialize JSON to a compact string
		FString Out;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Value.ToSharedRef(), TEXT(""), Writer);
		Writer->Close();
		return Out;
	}

	static void FillMetadataMap(const FBPVariableDescription& Var, TSharedPtr<FJsonObject>& OutObj)
	{
		TSharedPtr<FJsonObject> MetaObj = MakeShared<FJsonObject>();
		for (const FBPVariableMetaDataEntry& Entry : Var.MetaDataArray)
		{
			MetaObj->SetStringField(Entry.DataKey.ToString(), Entry.DataValue);
		}
		OutObj->SetObjectField(TEXT("metadata"), MetaObj);
	}

	static void FillVarInfo(const FBPVariableDescription& Var, TSharedPtr<FJsonObject>& OutObj, bool bIncludeMetadata)
	{
		OutObj->SetStringField(TEXT("name"), Var.VarName.ToString());
		OutObj->SetStringField(TEXT("category"), Var.Category.ToString());
		OutObj->SetStringField(TEXT("tooltip"), Var.FriendlyName);
		OutObj->SetStringField(TEXT("default_value"), Var.DefaultValue);

		OutObj->SetStringField(TEXT("pin_category"), Var.VarType.PinCategory.ToString());
		OutObj->SetStringField(TEXT("pin_sub_category"), Var.VarType.PinSubCategory.ToString());
		OutObj->SetStringField(TEXT("container"), StaticEnum<EPinContainerType>()->GetNameStringByValue((int64)Var.VarType.ContainerType));
		OutObj->SetStringField(TEXT("subobject_path"), Var.VarType.PinSubCategoryObject.IsValid() ? Var.VarType.PinSubCategoryObject->GetPathName() : TEXT(""));

		OutObj->SetBoolField(TEXT("is_blueprint_readonly"), (Var.PropertyFlags & CPF_BlueprintReadOnly) != 0);
		OutObj->SetBoolField(TEXT("is_editable"), (Var.PropertyFlags & CPF_DisableEditOnInstance) == 0);

		if (bIncludeMetadata)
		{
			FillMetadataMap(Var, OutObj);
		}
	}

	static bool TryGetVar(UBlueprint* BP, const FName& VarName, FBPVariableDescription*& OutVar)
	{
		OutVar = nullptr;
		if (!BP)
		{
			return false;
		}

		for (FBPVariableDescription& V : BP->NewVariables)
		{
			if (V.VarName == VarName)
			{
				OutVar = &V;
				return true;
			}
		}

		return false;
	}

	static bool TryMapTypePathToPin(const FString& TypePath, FSOTS_BPGenPin& OutPin)
	{
		OutPin = FSOTS_BPGenPin();
		OutPin.Category = NAME_None;
		OutPin.ContainerType = ESOTS_BPGenContainerType::None;

		if (TypePath.IsEmpty())
		{
			return false;
		}

		// Primitives (canonical VibeUE type_path)
		if (TypePath.Equals(TEXT("/Script/CoreUObject.FloatProperty"), ESearchCase::IgnoreCase))
		{
			OutPin.Category = TEXT("float");
			return true;
		}
		if (TypePath.Equals(TEXT("/Script/CoreUObject.IntProperty"), ESearchCase::IgnoreCase))
		{
			OutPin.Category = TEXT("int");
			return true;
		}
		if (TypePath.Equals(TEXT("/Script/CoreUObject.BoolProperty"), ESearchCase::IgnoreCase))
		{
			OutPin.Category = TEXT("bool");
			return true;
		}
		if (TypePath.Equals(TEXT("/Script/CoreUObject.DoubleProperty"), ESearchCase::IgnoreCase))
		{
			OutPin.Category = TEXT("double");
			return true;
		}
		if (TypePath.Equals(TEXT("/Script/CoreUObject.StrProperty"), ESearchCase::IgnoreCase))
		{
			OutPin.Category = TEXT("string");
			return true;
		}
		if (TypePath.Equals(TEXT("/Script/CoreUObject.NameProperty"), ESearchCase::IgnoreCase))
		{
			OutPin.Category = TEXT("name");
			return true;
		}
		if (TypePath.Equals(TEXT("/Script/CoreUObject.ByteProperty"), ESearchCase::IgnoreCase))
		{
			OutPin.Category = TEXT("byte");
			return true;
		}
		if (TypePath.Equals(TEXT("/Script/CoreUObject.TextProperty"), ESearchCase::IgnoreCase))
		{
			OutPin.Category = TEXT("text");
			return true;
		}

		// Classes/structs/enums: accept "/Script/Module.Type" or asset class paths
		if (TypePath.StartsWith(TEXT("/Script/"), ESearchCase::IgnoreCase) || TypePath.StartsWith(TEXT("/Game/"), ESearchCase::IgnoreCase))
		{
			OutPin.Category = TEXT("object");
			OutPin.SubObjectPath = TypePath;
			return true;
		}

		return false;
	}

	static TSharedPtr<FJsonValue> JsonObjectOrNull(const TSharedPtr<FJsonObject>& Obj)
	{
		if (Obj.IsValid())
		{
			return MakeShared<FJsonValueObject>(Obj);
		}
		return MakeShared<FJsonValueNull>();
	}
}

namespace SOTS_BPGenBridgeVariableOps
{
	FSOTS_BPGenBridgeVariableOpResult SearchTypes(const TSharedPtr<FJsonObject>& Params)
	{
		FString SearchText;
		if (Params.IsValid() && Params->HasTypedField<EJson::Object>(TEXT("search_criteria")))
		{
			const TSharedPtr<FJsonObject> Criteria = Params->GetObjectField(TEXT("search_criteria"));
			if (Criteria.IsValid())
			{
				Criteria->TryGetStringField(TEXT("search_text"), SearchText);
			}
		}
		if (SearchText.IsEmpty() && Params.IsValid())
		{
			Params->TryGetStringField(TEXT("search_text"), SearchText);
		}

		TArray<TSharedPtr<FJsonValue>> Types;

		// Always include primitive canonical type paths
		auto AddPrimitive = [&Types](const TCHAR* DisplayName, const TCHAR* TypePath)
		{
			TSharedPtr<FJsonObject> T = MakeShared<FJsonObject>();
			T->SetStringField(TEXT("display_name"), DisplayName);
			T->SetStringField(TEXT("type_path"), TypePath);
			T->SetStringField(TEXT("kind"), TEXT("Primitive"));
			Types.Add(MakeShared<FJsonValueObject>(T));
		};

		AddPrimitive(TEXT("Float"), TEXT("/Script/CoreUObject.FloatProperty"));
		AddPrimitive(TEXT("Int"), TEXT("/Script/CoreUObject.IntProperty"));
		AddPrimitive(TEXT("Bool"), TEXT("/Script/CoreUObject.BoolProperty"));
		AddPrimitive(TEXT("Double"), TEXT("/Script/CoreUObject.DoubleProperty"));
		AddPrimitive(TEXT("String"), TEXT("/Script/CoreUObject.StrProperty"));
		AddPrimitive(TEXT("Name"), TEXT("/Script/CoreUObject.NameProperty"));
		AddPrimitive(TEXT("Byte"), TEXT("/Script/CoreUObject.ByteProperty"));
		AddPrimitive(TEXT("Text"), TEXT("/Script/CoreUObject.TextProperty"));

		// Scan loaded UClasses for "/Script/Module.Class" paths
		const FString SearchLower = SearchText.ToLower();
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Cls = *It;
			if (!IsValid(Cls) || Cls->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
			{
				continue;
			}

			const FString Path = Cls->GetPathName();
			if (!Path.StartsWith(TEXT("/Script/")))
			{
				continue;
			}

			if (!SearchLower.IsEmpty())
			{
				const FString NameLower = Cls->GetName().ToLower();
				if (!NameLower.Contains(SearchLower) && !Path.ToLower().Contains(SearchLower))
				{
					continue;
				}
			}

			TSharedPtr<FJsonObject> T = MakeShared<FJsonObject>();
			T->SetStringField(TEXT("display_name"), Cls->GetName());
			T->SetStringField(TEXT("type_path"), Path);
			T->SetStringField(TEXT("kind"), TEXT("Class"));
			Types.Add(MakeShared<FJsonValueObject>(T));
			if (Types.Num() >= 200)
			{
				break;
			}
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetArrayField(TEXT("types"), Types);
		ResultObj->SetNumberField(TEXT("count"), Types.Num());

		FSOTS_BPGenBridgeVariableOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeVariableOpResult Create(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString VariableName;
		TSharedPtr<FJsonObject> VariableConfig;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("variable_name"), VariableName);
			if (Params->HasTypedField<EJson::Object>(TEXT("variable_config")))
			{
				VariableConfig = Params->GetObjectField(TEXT("variable_config"));
			}
		}

		if (BlueprintName.IsEmpty() || VariableName.IsEmpty() || !VariableConfig.IsValid())
		{
			return MakeVariableOpError(TEXT("ERR_INVALID_INPUT"), TEXT("create requires blueprint_name, variable_name, and variable_config"));
		}

		FString TypePath;
		VariableConfig->TryGetStringField(TEXT("type_path"), TypePath);
		if (TypePath.IsEmpty())
		{
			return MakeVariableOpError(TEXT("TYPE_PATH_REQUIRED"), TEXT("variable_config.type_path is required"));
		}

		FSOTS_BPGenPin Pin;
		if (!TryMapTypePathToPin(TypePath, Pin))
		{
			return MakeVariableOpError(TEXT("ERR_TYPE_INVALID"), FString::Printf(TEXT("Unsupported type_path: %s"), *TypePath));
		}

		FString Category;
		FString Tooltip;
		VariableConfig->TryGetStringField(TEXT("category"), Category);
		VariableConfig->TryGetStringField(TEXT("tooltip"), Tooltip);

		bool bEditable = true;
		bool bBlueprintReadonly = false;
		bool bExposeOnSpawn = false;
		VariableConfig->TryGetBoolField(TEXT("is_editable"), bEditable);
		VariableConfig->TryGetBoolField(TEXT("is_blueprint_readonly"), bBlueprintReadonly);
		VariableConfig->TryGetBoolField(TEXT("is_expose_on_spawn"), bExposeOnSpawn);

		FString DefaultValueStr;
		if (VariableConfig->HasField(TEXT("default_value")))
		{
			DefaultValueStr = JsonValueToDefaultString(VariableConfig->TryGetField(TEXT("default_value")));
		}

		FSOTS_BPGenBridgeVariableOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		// Create or update the variable using existing BPGen ensure path (handles compilation + type conversion)
		const FSOTS_BPGenEnsureResult EnsureRes = USOTS_BPGenEnsure::EnsureVariable(
			nullptr,
			Normalized,
			VariableName,
			Pin,
			DefaultValueStr,
			bEditable,
			bExposeOnSpawn,
			/*bCreateIfMissing*/true,
			/*bUpdateIfExists*/true);

		if (!EnsureRes.bSuccess)
		{
			FSOTS_BPGenBridgeVariableOpResult R;
			R.bOk = false;
			R.ErrorCode = EnsureRes.ErrorCode;
			if (!EnsureRes.ErrorMessage.IsEmpty())
			{
				R.Errors.Add(EnsureRes.ErrorMessage);
			}
			R.Warnings.Append(EnsureRes.Warnings);
			return R;
		}

		// Apply additional metadata not covered by EnsureVariable
		FBPVariableDescription* Var = nullptr;
		if (TryGetVar(BP, FName(*VariableName), Var) && Var)
		{
			if (!Category.IsEmpty())
			{
				Var->Category = FText::FromString(Category);
			}
			if (!Tooltip.IsEmpty())
			{
				Var->FriendlyName = Tooltip;
			}

			if (bBlueprintReadonly)
			{
				Var->PropertyFlags |= CPF_BlueprintReadOnly;
			}
			else
			{
				Var->PropertyFlags &= ~CPF_BlueprintReadOnly;
			}

			if (bEditable)
			{
				Var->PropertyFlags &= ~CPF_DisableEditOnInstance;
			}
			else
			{
				Var->PropertyFlags |= CPF_DisableEditOnInstance;
			}

			// If a metadata map was provided, apply it
			if (VariableConfig->HasTypedField<EJson::Object>(TEXT("metadata")))
			{
				const TSharedPtr<FJsonObject> MetaObj = VariableConfig->GetObjectField(TEXT("metadata"));
				if (MetaObj.IsValid())
				{
					Var->MetaDataArray.Reset();
					for (const auto& Kvp : MetaObj->Values)
					{
						Var->SetMetaData(FName(*Kvp.Key), JsonValueToDefaultString(Kvp.Value));
					}
				}
			}

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
			FKismetEditorUtilities::CompileBlueprint(BP);
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetStringField(TEXT("variable_name"), VariableName);
		ResultObj->SetStringField(TEXT("type_path"), TypePath);

		FSOTS_BPGenBridgeVariableOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		R.Warnings.Append(EnsureRes.Warnings);
		return R;
	}

	FSOTS_BPGenBridgeVariableOpResult Delete(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString VariableName;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("variable_name"), VariableName);
		}

		FSOTS_BPGenBridgeVariableOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		FBlueprintEditorUtils::RemoveMemberVariable(BP, FName(*VariableName));
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetStringField(TEXT("variable_name"), VariableName);

		FSOTS_BPGenBridgeVariableOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeVariableOpResult List(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		TSharedPtr<FJsonObject> Criteria;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			if (Params->HasTypedField<EJson::Object>(TEXT("list_criteria")))
			{
				Criteria = Params->GetObjectField(TEXT("list_criteria"));
			}
		}

		FSOTS_BPGenBridgeVariableOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		FString CategoryFilter;
		FString NameContains;
		bool bIncludeMetadata = false;
		if (Criteria.IsValid())
		{
			Criteria->TryGetStringField(TEXT("category"), CategoryFilter);
			Criteria->TryGetStringField(TEXT("name_contains"), NameContains);
			Criteria->TryGetBoolField(TEXT("include_metadata"), bIncludeMetadata);
		}

		const FString NameContainsLower = NameContains.ToLower();
		const FString CategoryLower = CategoryFilter.ToLower();

		TArray<TSharedPtr<FJsonValue>> Vars;
		for (const FBPVariableDescription& Var : BP->NewVariables)
		{
			if (!NameContainsLower.IsEmpty() && !Var.VarName.ToString().ToLower().Contains(NameContainsLower))
			{
				continue;
			}

			if (!CategoryLower.IsEmpty() && !Var.Category.ToString().ToLower().Contains(CategoryLower))
			{
				continue;
			}

			TSharedPtr<FJsonObject> V = MakeShared<FJsonObject>();
			FillVarInfo(Var, V, bIncludeMetadata);
			Vars.Add(MakeShared<FJsonValueObject>(V));
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetArrayField(TEXT("variables"), Vars);
		ResultObj->SetNumberField(TEXT("count"), Vars.Num());

		FSOTS_BPGenBridgeVariableOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeVariableOpResult GetInfo(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString VariableName;
		bool bIncludeMetadata = true;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("variable_name"), VariableName);
			if (Params->HasTypedField<EJson::Object>(TEXT("info_options")))
			{
				const TSharedPtr<FJsonObject> Info = Params->GetObjectField(TEXT("info_options"));
				if (Info.IsValid())
				{
					Info->TryGetBoolField(TEXT("include_metadata"), bIncludeMetadata);
				}
			}
		}

		FSOTS_BPGenBridgeVariableOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		FBPVariableDescription* Var = nullptr;
		if (!TryGetVar(BP, FName(*VariableName), Var) || !Var)
		{
			return MakeVariableOpError(TEXT("VARIABLE_NOT_FOUND"), FString::Printf(TEXT("Variable not found: %s"), *VariableName));
		}

		TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
		FillVarInfo(*Var, VarObj, bIncludeMetadata);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetObjectField(TEXT("variable"), VarObj);

		FSOTS_BPGenBridgeVariableOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeVariableOpResult GetProperty(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString VariableName;
		FString PropertyPath;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("variable_name"), VariableName);
			Params->TryGetStringField(TEXT("property_path"), PropertyPath);
			if (PropertyPath.IsEmpty())
			{
				Params->TryGetStringField(TEXT("path"), PropertyPath);
			}
		}

		FSOTS_BPGenBridgeVariableOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		FBPVariableDescription* Var = nullptr;
		if (!TryGetVar(BP, FName(*VariableName), Var) || !Var)
		{
			return MakeVariableOpError(TEXT("VARIABLE_NOT_FOUND"), FString::Printf(TEXT("Variable not found: %s"), *VariableName));
		}

		FString OutValue;
		if (PropertyPath.Equals(TEXT("category"), ESearchCase::IgnoreCase))
		{
			OutValue = Var->Category.ToString();
		}
		else if (PropertyPath.Equals(TEXT("tooltip"), ESearchCase::IgnoreCase))
		{
			OutValue = Var->FriendlyName;
		}
		else if (PropertyPath.Equals(TEXT("default_value"), ESearchCase::IgnoreCase))
		{
			OutValue = Var->DefaultValue;
		}
		else if (PropertyPath.Equals(TEXT("is_blueprint_readonly"), ESearchCase::IgnoreCase))
		{
			OutValue = ((Var->PropertyFlags & CPF_BlueprintReadOnly) != 0) ? TEXT("true") : TEXT("false");
		}
		else if (PropertyPath.Equals(TEXT("is_editable"), ESearchCase::IgnoreCase))
		{
			OutValue = ((Var->PropertyFlags & CPF_DisableEditOnInstance) == 0) ? TEXT("true") : TEXT("false");
		}
		else
		{
			// Try metadata
			for (const FBPVariableMetaDataEntry& Entry : Var->MetaDataArray)
			{
				if (Entry.DataKey.ToString().Equals(PropertyPath, ESearchCase::IgnoreCase))
				{
					OutValue = Entry.DataValue;
					break;
				}
			}
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetStringField(TEXT("variable_name"), VariableName);
		ResultObj->SetStringField(TEXT("property_path"), PropertyPath);
		ResultObj->SetStringField(TEXT("value"), OutValue);

		FSOTS_BPGenBridgeVariableOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}

	FSOTS_BPGenBridgeVariableOpResult SetProperty(const TSharedPtr<FJsonObject>& Params)
	{
		FString BlueprintName;
		FString VariableName;
		FString PropertyPath;
		TSharedPtr<FJsonValue> Value;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName);
			Params->TryGetStringField(TEXT("variable_name"), VariableName);
			Params->TryGetStringField(TEXT("property_path"), PropertyPath);
			if (PropertyPath.IsEmpty())
			{
				Params->TryGetStringField(TEXT("path"), PropertyPath);
			}
			Value = Params->TryGetField(TEXT("value"));
		}

		FSOTS_BPGenBridgeVariableOpResult Err;
		FString Normalized;
		UBlueprint* BP = LoadBlueprintByPath(BlueprintName, Normalized, Err);
		if (!BP)
		{
			return Err;
		}

		FBPVariableDescription* Var = nullptr;
		if (!TryGetVar(BP, FName(*VariableName), Var) || !Var)
		{
			return MakeVariableOpError(TEXT("VARIABLE_NOT_FOUND"), FString::Printf(TEXT("Variable not found: %s"), *VariableName));
		}

		const FString ValueStr = JsonValueToDefaultString(Value);

		if (PropertyPath.Equals(TEXT("category"), ESearchCase::IgnoreCase))
		{
			Var->Category = FText::FromString(ValueStr);
		}
		else if (PropertyPath.Equals(TEXT("tooltip"), ESearchCase::IgnoreCase))
		{
			Var->FriendlyName = ValueStr;
		}
		else if (PropertyPath.Equals(TEXT("default_value"), ESearchCase::IgnoreCase))
		{
			Var->DefaultValue = ValueStr;
			Var->SetMetaData(TEXT("DefaultValue"), ValueStr);
		}
		else if (PropertyPath.Equals(TEXT("is_blueprint_readonly"), ESearchCase::IgnoreCase))
		{
			const bool bTrue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
			if (bTrue)
			{
				Var->PropertyFlags |= CPF_BlueprintReadOnly;
			}
			else
			{
				Var->PropertyFlags &= ~CPF_BlueprintReadOnly;
			}
		}
		else if (PropertyPath.Equals(TEXT("is_editable"), ESearchCase::IgnoreCase))
		{
			const bool bTrue = ValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ValueStr.Equals(TEXT("1"));
			if (bTrue)
			{
				Var->PropertyFlags &= ~CPF_DisableEditOnInstance;
			}
			else
			{
				Var->PropertyFlags |= CPF_DisableEditOnInstance;
			}
		}
		else
		{
			Var->SetMetaData(FName(*PropertyPath), ValueStr);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		FKismetEditorUtilities::CompileBlueprint(BP);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
		ResultObj->SetStringField(TEXT("variable_name"), VariableName);
		ResultObj->SetStringField(TEXT("property_path"), PropertyPath);
		ResultObj->SetStringField(TEXT("value"), ValueStr);

		FSOTS_BPGenBridgeVariableOpResult R;
		R.bOk = true;
		R.Result = ResultObj;
		return R;
	}
}
