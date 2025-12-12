#if WITH_EDITOR

#include "SOTS_GenericJsonImporter.h"

#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

bool USOTS_GenericJsonImporter::ImportJson(UDataAsset* TargetAsset, const FString& JsonFilePath, FName ArrayPropertyName, bool bMarkDirty)
{
    if (!TargetAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("GenericJsonImporter: No TargetAsset set."));
        return false;
    }

    if (JsonFilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("GenericJsonImporter: JsonFilePath is empty."));
        return false;
    }

    const FString ResolvedPath = FPaths::ConvertRelativePathToFull(JsonFilePath);

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *ResolvedPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("GenericJsonImporter: Failed to load JSON file: %s"), *ResolvedPath);
        return false;
    }

    TSharedPtr<FJsonValue> RootValue;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("GenericJsonImporter: Failed to parse JSON: %s"), *ResolvedPath);
        return false;
    }

    TSharedPtr<FJsonObject> ObjectToApply;

    if (RootValue->Type == EJson::Object)
    {
        ObjectToApply = RootValue->AsObject();
    }
    else if (RootValue->Type == EJson::Array)
    {
        // Determine which array property should receive the root array.
        FName TargetArrayProp = ArrayPropertyName;
        if (TargetArrayProp.IsNone())
        {
            int32 ArrayCount = 0;
            FName FoundArrayProp = NAME_None;
            for (TFieldIterator<FProperty> It(TargetAsset->GetClass()); It; ++It)
            {
                if (CastField<FArrayProperty>(*It))
                {
                    ++ArrayCount;
                    FoundArrayProp = It->GetFName();
                    if (FoundArrayProp == FName(TEXT("Actions")))
                    {
                        TargetArrayProp = FoundArrayProp;
                        break;
                    }
                }
            }

            if (TargetArrayProp.IsNone() && ArrayCount == 1)
            {
                TargetArrayProp = FoundArrayProp;
            }
        }

        if (TargetArrayProp.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("GenericJsonImporter: Root is an array but no ArrayPropertyName was provided and no suitable array property was found on %s."), *TargetAsset->GetName());
            return false;
        }

        ObjectToApply = MakeShared<FJsonObject>();
        ObjectToApply->SetArrayField(TargetArrayProp.ToString(), RootValue->AsArray());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GenericJsonImporter: Root JSON must be an object or an array: %s"), *ResolvedPath);
        return false;
    }

    TargetAsset->Modify();

    const bool bConverted = FJsonObjectConverter::JsonObjectToUStruct(ObjectToApply.ToSharedRef(), TargetAsset->GetClass(), TargetAsset, 0, 0);
    if (!bConverted)
    {
        UE_LOG(LogTemp, Warning, TEXT("GenericJsonImporter: Failed to map JSON into asset %s"), *TargetAsset->GetPathName());
        return false;
    }

    if (bMarkDirty)
    {
        TargetAsset->MarkPackageDirty();
        TargetAsset->PostEditChange();
    }

    UE_LOG(LogTemp, Log, TEXT("GenericJsonImporter: Imported %s into %s"), *ResolvedPath, *TargetAsset->GetPathName());
    return true;
}

#endif // WITH_EDITOR
