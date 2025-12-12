#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_GenericJsonImporter.generated.h"

/**
 * Generic editor utility that maps JSON onto any DataAsset using reflection.
 * Supports object roots and array roots (array roots target a chosen array property).
 */
UCLASS()
class SOTS_EDUTIL_API USOTS_GenericJsonImporter : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Import JSON into the target asset. If the root is an array, it is applied to the provided array property name (or "Actions" / only array property fallback). */
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "JSON")
    static bool ImportJson(UDataAsset* TargetAsset, const FString& JsonFilePath, FName ArrayPropertyName = NAME_None, bool bMarkDirty = true);
};

#endif // WITH_EDITOR
