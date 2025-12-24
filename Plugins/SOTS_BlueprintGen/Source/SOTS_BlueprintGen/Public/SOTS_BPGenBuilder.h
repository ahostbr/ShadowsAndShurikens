#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenBuilder.generated.h"

UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenAssetResult CreateStructAssetFromDef(const UObject* WorldContextObject, const FSOTS_BPGenStructDef& StructDef);

    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenAssetResult CreateEnumAssetFromDef(const UObject* WorldContextObject, const FSOTS_BPGenEnumDef& EnumDef);

    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenAssetResult CreateDataAssetFromDef(const UObject* WorldContextObject, const FSOTS_BPGenDataAssetDef& AssetDef);

    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenApplyResult ApplyFunctionSkeleton(const UObject* WorldContextObject, const FSOTS_BPGenFunctionDef& FunctionDef);

    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenApplyResult ApplyGraphSpecToFunction(const UObject* WorldContextObject, const FSOTS_BPGenFunctionDef& FunctionDef, const FSOTS_BPGenGraphSpec& GraphSpec);

    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenApplyResult ApplyGraphSpecToTarget(const UObject* WorldContextObject, const FSOTS_BPGenGraphSpec& GraphSpec);

    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenCanonicalizeResult CanonicalizeGraphSpec(const FSOTS_BPGenGraphSpec& GraphSpec, const FSOTS_BPGenSpecCanonicalizeOptions& Options);

    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenGraphEditResult DeleteNodeById(const UObject* WorldContextObject, const FSOTS_BPGenDeleteNodeRequest& Request);

    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenGraphEditResult DeleteLink(const UObject* WorldContextObject, const FSOTS_BPGenDeleteLinkRequest& Request);

    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen")
    static FSOTS_BPGenGraphEditResult ReplaceNodePreserveId(const UObject* WorldContextObject, const FSOTS_BPGenReplaceNodeRequest& Request);

    /**
     * Debug/test helper: builds Test_AllNodesGraph on BP_PrintHello using BPGen specs.
     */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "SOTS|BPGen|Test")
    static FSOTS_BPGenApplyResult BuildTestAllNodesGraphForBPPrintHello();
};
