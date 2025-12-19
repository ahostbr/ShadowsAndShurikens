#include "SOTS_BPGenAPI.h"

#include "JsonObjectConverter.h"
#include "Misc/ScopeExit.h"
#include "SOTS_BPGenBuilder.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenDebug.h"

namespace
{
struct FSOTS_BPGenAPIEnvelope
{
    bool bOk = false;
    FString Action;
    FString RequestId;
    FString Error;
    TArray<FString> Errors;
    TArray<FString> Warnings;
    TSharedPtr<FJsonObject> ResultObject;
};

template <typename TStruct>
bool DeserializeJson(const FString& Json, TStruct& OutStruct, FString& OutError)
{
    if (Json.IsEmpty())
    {
        OutError = TEXT("ParamsJson empty");
        return false;
    }

    if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &OutStruct, 0, 0))
    {
        OutError = TEXT("Failed to parse ParamsJson for struct");
        return false;
    }
    return true;
}

template <typename TStruct>
TSharedPtr<FJsonObject> StructToJsonObject(const TStruct& Struct)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    FJsonObjectConverter::UStructToJsonObject(TStruct::StaticStruct(), &Struct, JsonObject.ToSharedRef(), 0, 0);
    return JsonObject;
}

bool IsApplyAction(const FString& Action)
{
    const FString Lower = Action.ToLower();
    return Lower.Contains(TEXT("apply")) || Lower.Contains(TEXT("delete")) || Lower.Contains(TEXT("replace")) || Lower.Contains(TEXT("create_"));
}

bool EnvAllowsApply()
{
    const FString Allow = FPlatformMisc::GetEnvironmentVariable(TEXT("SOTS_ALLOW_APPLY"));
    return Allow.Equals(TEXT("1"), ESearchCase::IgnoreCase) || Allow.Equals(TEXT("true"), ESearchCase::IgnoreCase);
}

bool SerializeEnvelope(const FSOTS_BPGenAPIEnvelope& Env, FString& OutJson)
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("action"), Env.Action);
    if (!Env.RequestId.IsEmpty())
    {
        Root->SetStringField(TEXT("request_id"), Env.RequestId);
    }
    Root->SetBoolField(TEXT("ok"), Env.bOk);
    if (!Env.Error.IsEmpty())
    {
        Root->SetStringField(TEXT("error"), Env.Error);
    }
    if (Env.Errors.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ErrorVals;
        for (const FString& Err : Env.Errors)
        {
            ErrorVals.Add(MakeShared<FJsonValueString>(Err));
        }
        Root->SetArrayField(TEXT("errors"), ErrorVals);
    }
    if (Env.Warnings.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> WarnVals;
        for (const FString& Warn : Env.Warnings)
        {
            WarnVals.Add(MakeShared<FJsonValueString>(Warn));
        }
        Root->SetArrayField(TEXT("warnings"), WarnVals);
    }
    if (Env.ResultObject.IsValid())
    {
        Root->SetObjectField(TEXT("result"), Env.ResultObject);
    }

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    return FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
}

void AddError(FSOTS_BPGenAPIEnvelope& Env, const FString& Err)
{
    Env.Errors.Add(Err);
    if (Env.Error.IsEmpty())
    {
        Env.Error = Err;
    }
}

FSOTS_BPGenAPIEnvelope ExecuteSingle(const UObject* WorldContextObject, const FString& ActionIn, const FString& ParamsJson)
{
    FSOTS_BPGenAPIEnvelope Env;
    Env.Action = ActionIn;

    const FString Action = ActionIn.ToLower();
    const bool bApplyAllowed = EnvAllowsApply();

    if (IsApplyAction(Action) && !bApplyAllowed)
    {
        AddError(Env, TEXT("APPLY blocked: set SOTS_ALLOW_APPLY=1 to enable mutations"));
        return Env;
    }

    const auto ToResultJson = [&Env](const auto& ResultStruct)
    {
        Env.ResultObject = StructToJsonObject(ResultStruct);
        Env.bOk = ResultStruct.bSuccess;
        if (!ResultStruct.ErrorMessage.IsEmpty())
        {
            AddError(Env, ResultStruct.ErrorMessage);
        }
        if (ResultStruct.Errors.Num() > 0)
        {
            for (const FString& Err : ResultStruct.Errors)
            {
                AddError(Env, Err);
            }
        }
        Env.Warnings.Append(ResultStruct.Warnings);
    };

    FString ParseError;

    if (Action == TEXT("apply_graph_spec"))
    {
        FSOTS_BPGenGraphSpec Spec;
        if (!DeserializeJson(ParamsJson, Spec, ParseError))
        {
            AddError(Env, ParseError);
            return Env;
        }
        const FSOTS_BPGenApplyResult Result = USOTS_BPGenBuilder::ApplyGraphSpecToTarget(WorldContextObject, Spec);
        ToResultJson(Result);
        return Env;
    }

    if (Action == TEXT("canonicalize_graph_spec"))
    {
        TSharedPtr<FJsonObject> RootObj;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ParamsJson);
        if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
        {
            AddError(Env, TEXT("Failed to parse ParamsJson"));
            return Env;
        }
        FSOTS_BPGenGraphSpec Spec;
        FSOTS_BPGenSpecCanonicalizeOptions Options;
        if (!FJsonObjectConverter::JsonObjectToUStruct(RootObj.ToSharedRef(), &Spec, 0, 0))
        {
            AddError(Env, TEXT("Failed to parse graph_spec"));
            return Env;
        }
        if (TSharedPtr<FJsonObject>* OptionsObj = RootObj->Values.Find(TEXT("options")) ? RootObj->TryGetObjectField(TEXT("options")) : nullptr)
        {
            FJsonObjectConverter::JsonObjectToUStruct(*OptionsObj, &Options, 0, 0);
        }
        const FSOTS_BPGenCanonicalizeResult Result = USOTS_BPGenBuilder::CanonicalizeGraphSpec(Spec, Options);
        ToResultJson(Result);
        return Env;
    }

    if (Action == TEXT("apply_function_skeleton"))
    {
        FSOTS_BPGenFunctionDef Def;
        if (!DeserializeJson(ParamsJson, Def, ParseError))
        {
            AddError(Env, ParseError);
            return Env;
        }
        const FSOTS_BPGenApplyResult Result = USOTS_BPGenBuilder::ApplyFunctionSkeleton(WorldContextObject, Def);
        ToResultJson(Result);
        return Env;
    }

    if (Action == TEXT("delete_node"))
    {
        FSOTS_BPGenDeleteNodeRequest Request;
        if (!DeserializeJson(ParamsJson, Request, ParseError))
        {
            AddError(Env, ParseError);
            return Env;
        }
        const FSOTS_BPGenGraphEditResult Result = USOTS_BPGenBuilder::DeleteNodeById(WorldContextObject, Request);
        ToResultJson(Result);
        return Env;
    }

    if (Action == TEXT("delete_link"))
    {
        FSOTS_BPGenDeleteLinkRequest Request;
        if (!DeserializeJson(ParamsJson, Request, ParseError))
        {
            AddError(Env, ParseError);
            return Env;
        }
        const FSOTS_BPGenGraphEditResult Result = USOTS_BPGenBuilder::DeleteLink(WorldContextObject, Request);
        ToResultJson(Result);
        return Env;
    }

    if (Action == TEXT("replace_node"))
    {
        FSOTS_BPGenReplaceNodeRequest Request;
        if (!DeserializeJson(ParamsJson, Request, ParseError))
        {
            AddError(Env, ParseError);
            return Env;
        }
        const FSOTS_BPGenGraphEditResult Result = USOTS_BPGenBuilder::ReplaceNodePreserveId(WorldContextObject, Request);
        ToResultJson(Result);
        return Env;
    }

    if (Action == TEXT("create_struct_asset"))
    {
        FSOTS_BPGenStructDef Def;
        if (!DeserializeJson(ParamsJson, Def, ParseError))
        {
            AddError(Env, ParseError);
            return Env;
        }
        const FSOTS_BPGenAssetResult Result = USOTS_BPGenBuilder::CreateStructAssetFromDef(WorldContextObject, Def);
        ToResultJson(Result);
        return Env;
    }

    if (Action == TEXT("create_enum_asset"))
    {
        FSOTS_BPGenEnumDef Def;
        if (!DeserializeJson(ParamsJson, Def, ParseError))
        {
            AddError(Env, ParseError);
            return Env;
        }
        const FSOTS_BPGenAssetResult Result = USOTS_BPGenBuilder::CreateEnumAssetFromDef(WorldContextObject, Def);
        ToResultJson(Result);
        return Env;
    }

    if (Action == TEXT("debug_annotate"))
    {
        TSharedPtr<FJsonObject> Root;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ParamsJson);
        if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        {
            AddError(Env, TEXT("Failed to parse ParamsJson"));
            return Env;
        }

        const FString BlueprintPath = Root->GetStringField(TEXT("blueprint_asset_path"));
        const FString FunctionNameStr = Root->GetStringField(TEXT("function_name"));
        const FString Annotation = Root->GetStringField(TEXT("annotation"));

        TArray<FString> NodeIds;
        if (const TArray<TSharedPtr<FJsonValue>>* IdArray = nullptr; Root->TryGetArrayField(TEXT("node_ids"), IdArray) && IdArray)
        {
            for (const TSharedPtr<FJsonValue>& Val : *IdArray)
            {
                FString Id;
                if (Val.IsValid() && Val->TryGetString(Id))
                {
                    NodeIds.Add(Id);
                }
            }
        }

        const bool bOk = USOTS_BPGenDebug::AnnotateNodes(WorldContextObject, BlueprintPath, FName(*FunctionNameStr), NodeIds, Annotation);
        Env.bOk = bOk;
        Env.ResultObject = MakeShared<FJsonObject>();
        Env.ResultObject->SetBoolField(TEXT("ok"), bOk);
        if (!bOk)
        {
            AddError(Env, TEXT("Annotate failed"));
        }
        return Env;
    }

    if (Action == TEXT("clear_annotations"))
    {
        TSharedPtr<FJsonObject> Root;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ParamsJson);
        if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        {
            AddError(Env, TEXT("Failed to parse ParamsJson"));
            return Env;
        }

        const FString BlueprintPath = Root->GetStringField(TEXT("blueprint_asset_path"));
        const FString FunctionNameStr = Root->GetStringField(TEXT("function_name"));
        const bool bOk = USOTS_BPGenDebug::ClearAnnotations(WorldContextObject, BlueprintPath, FName(*FunctionNameStr));
        Env.bOk = bOk;
        Env.ResultObject = MakeShared<FJsonObject>();
        Env.ResultObject->SetBoolField(TEXT("ok"), bOk);
        if (!bOk)
        {
            AddError(Env, TEXT("Clear annotations failed"));
        }
        return Env;
    }

    if (Action == TEXT("focus_node"))
    {
        TSharedPtr<FJsonObject> Root;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ParamsJson);
        if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        {
            AddError(Env, TEXT("Failed to parse ParamsJson"));
            return Env;
        }

        const FString BlueprintPath = Root->GetStringField(TEXT("blueprint_asset_path"));
        const FString FunctionNameStr = Root->GetStringField(TEXT("function_name"));
        const FString NodeId = Root->GetStringField(TEXT("node_id"));
        const bool bOk = USOTS_BPGenDebug::FocusNodeById(WorldContextObject, BlueprintPath, FName(*FunctionNameStr), NodeId);
        Env.bOk = bOk;
        Env.ResultObject = MakeShared<FJsonObject>();
        Env.ResultObject->SetBoolField(TEXT("ok"), bOk);
        if (!bOk)
        {
            AddError(Env, TEXT("Focus node failed"));
        }
        return Env;
    }

    AddError(Env, FString::Printf(TEXT("Unknown action: %s"), *ActionIn));
    return Env;
}
} // namespace

bool USOTS_BPGenAPI::ExecuteAction(const UObject* WorldContextObject, const FString& Action, const FString& ParamsJson, FString& OutResponseJson)
{
    const FSOTS_BPGenAPIEnvelope Env = ExecuteSingle(WorldContextObject, Action, ParamsJson);
    return SerializeEnvelope(Env, OutResponseJson);
}

bool USOTS_BPGenAPI::ExecuteBatch(const UObject* WorldContextObject, const FString& BatchJson, FString& OutResponseJson)
{
    TArray<TSharedPtr<FJsonValue>> Items;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BatchJson);
    if (!FJsonSerializer::Deserialize(Reader, Items))
    {
        return false;
    }

    TArray<TSharedPtr<FJsonValue>> Results;
    for (const TSharedPtr<FJsonValue>& Val : Items)
    {
        if (!Val.IsValid() || Val->Type != EJson::Object)
        {
            continue;
        }
        const TSharedPtr<FJsonObject> Obj = Val->AsObject();
        if (!Obj.IsValid())
        {
            continue;
        }

        const FString Action = Obj->GetStringField(TEXT("action"));
        FString RequestId;
        Obj->TryGetStringField(TEXT("request_id"), RequestId);

        FString ParamsJson;
        if (const TSharedPtr<FJsonObject>* ParamsObj = Obj->Values.Find(TEXT("params")) ? Obj->TryGetObjectField(TEXT("params")) : nullptr)
        {
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsJson);
            FJsonSerializer::Serialize((*ParamsObj).ToSharedRef(), Writer);
        }
        else
        {
            ParamsJson = TEXT("{}");
        }

        FSOTS_BPGenAPIEnvelope Env = ExecuteSingle(WorldContextObject, Action, ParamsJson);
        Env.RequestId = RequestId;

        FString EnvJson;
        SerializeEnvelope(Env, EnvJson);

        TSharedPtr<FJsonObject> EnvObj;
        TSharedRef<TJsonReader<>> EnvReader = TJsonReaderFactory<>::Create(EnvJson);
        FJsonSerializer::Deserialize(EnvReader, EnvObj);
        Results.Add(MakeShared<FJsonValueObject>(EnvObj));
    }

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetArrayField(TEXT("results"), Results);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutResponseJson);
    return FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
}
