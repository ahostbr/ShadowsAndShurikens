#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenInspector.generated.h"

UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenInspector : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** List nodes in a function graph with optional pin metadata. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|Inspect")
    static FSOTS_BPGenNodeListResult ListFunctionGraphNodes(const UObject* WorldContextObject, const FString& BlueprintPath, FName FunctionName, bool bIncludePins = false);

    /** Describe a single node by BPGen NodeId (falls back to NodeGuid lookup). */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|Inspect")
    static FSOTS_BPGenNodeDescribeResult DescribeNodeById(const UObject* WorldContextObject, const FString& BlueprintPath, FName FunctionName, const FString& NodeId, bool bIncludePins = true);

    /** Compile a Blueprint asset. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|Workflow")
    static FSOTS_BPGenMaintenanceResult CompileBlueprintAsset(const UObject* WorldContextObject, const FString& BlueprintPath);

    /** Save a Blueprint asset. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|Workflow")
    static FSOTS_BPGenMaintenanceResult SaveBlueprintAsset(const UObject* WorldContextObject, const FString& BlueprintPath);

    /** Reconstruct all nodes in a function graph, then compile. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|Workflow")
    static FSOTS_BPGenMaintenanceResult RefreshAllNodesInFunction(const UObject* WorldContextObject, const FString& BlueprintPath, FName FunctionName, bool bIncludePins = false);
};
