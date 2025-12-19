#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_BPGenAPI.generated.h"

/**
 * Canonical entry surface for BPGen (One True Entry).
 * All external transports (bridge, MCP, inbox jobs) should funnel through these JSON helpers.
 */
UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenAPI : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Execute a single BPGen action.
     * @param WorldContextObject World context for asset resolution.
     * @param Action Stable action name (e.g., apply_graph_spec, canonicalize_graph_spec, delete_node, delete_link, replace_node, apply_function_skeleton, create_struct_asset, create_enum_asset).
     * @param ParamsJson JSON payload matching the target action's UStruct schema.
     * @param OutResponseJson JSON response envelope containing ok/action/errors/warnings/result.
     * @return true if the action dispatched; ok field in the response reflects business success.
     */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|API", meta = (WorldContext = "WorldContextObject"))
    static bool ExecuteAction(const UObject* WorldContextObject, const FString& Action, const FString& ParamsJson, FString& OutResponseJson);

    /**
     * Execute a batch of actions. BatchJson is an array of {"action": "...", "params": {...}, "request_id": "optional"}.
     * Responses are returned as an array of envelopes preserving order.
     */
    UFUNCTION(BlueprintCallable, Category = "SOTS|BPGen|API", meta = (WorldContext = "WorldContextObject"))
    static bool ExecuteBatch(const UObject* WorldContextObject, const FString& BatchJson, FString& OutResponseJson);
};
