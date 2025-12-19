#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_BPGenDebug.generated.h"

/** Debug/visualization helpers for BPGen operations. Editor-only. */
UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenDebug : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Annotate nodes in a function graph with a debug comment prefix. */
	UFUNCTION(BlueprintCallable, Category="SOTS|BPGen|Debug", meta=(WorldContext="WorldContextObject"))
	static bool AnnotateNodes(const UObject* WorldContextObject, const FString& BlueprintAssetPath, FName FunctionName, const TArray<FString>& NodeIds, const FString& AnnotationText = TEXT("[BPGEN]"));

	/** Clear BPGen annotations from a function graph (nodes with comments starting with [BPGEN]). */
	UFUNCTION(BlueprintCallable, Category="SOTS|BPGen|Debug", meta=(WorldContext="WorldContextObject"))
	static bool ClearAnnotations(const UObject* WorldContextObject, const FString& BlueprintAssetPath, FName FunctionName);

	/** Focus the Blueprint editor on a specific node id (best effort). */
	UFUNCTION(BlueprintCallable, Category="SOTS|BPGen|Debug", meta=(WorldContext="WorldContextObject"))
	static bool FocusNodeById(const UObject* WorldContextObject, const FString& BlueprintAssetPath, FName FunctionName, const FString& NodeId);
};
