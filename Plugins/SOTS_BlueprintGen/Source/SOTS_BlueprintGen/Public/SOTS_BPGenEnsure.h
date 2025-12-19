#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenEnsure.generated.h"

UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenEnsure : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Ensure a function exists with the given signature (idempotent). */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|Ensure")
    static FSOTS_BPGenEnsureResult EnsureFunction(const UObject* WorldContextObject, const FString& BlueprintAssetPath, const FString& FunctionName, const FSOTS_BPGenFunctionSignature& Signature, bool bCreateIfMissing = true, bool bUpdateIfExists = true);

    /** Ensure a variable exists with the given type/default (idempotent). */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|Ensure")
    static FSOTS_BPGenEnsureResult EnsureVariable(const UObject* WorldContextObject, const FString& BlueprintAssetPath, const FString& VarName, const FSOTS_BPGenPin& VarType, const FString& DefaultValue, bool bInstanceEditable = false, bool bExposeOnSpawn = false, bool bCreateIfMissing = true, bool bUpdateIfExists = true);

    /** Ensure a widget exists in a widget blueprint's tree (idempotent). */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|UMG")
    static FSOTS_BPGenWidgetEnsureResult EnsureWidgetComponent(const UObject* WorldContextObject, const FSOTS_BPGenWidgetSpec& Request);

    /** Apply property updates to an existing widget (slot or widget properties). */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|UMG")
    static FSOTS_BPGenWidgetPropertyResult SetWidgetProperties(const UObject* WorldContextObject, const FSOTS_BPGenWidgetPropertyRequest& Request);

    /** Ensure a widget property binding exists and optionally apply graph content to its function. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|UMG")
    static FSOTS_BPGenBindingEnsureResult EnsureWidgetBinding(const UObject* WorldContextObject, const FSOTS_BPGenBindingRequest& Request);
};
