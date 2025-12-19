#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_BPGenTypes.h"
#include "SOTS_BPGenExamples.generated.h"

UCLASS()
class SOTS_BLUEPRINTGEN_API USOTS_BPGenExamples : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Example: apply a simple function graph that calls PrintString using a spawner key. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "SOTS|BPGen|Examples")
	static FSOTS_BPGenApplyResult Example_ApplyPrintStringGraph(const FString& TargetBlueprintPath, FName FunctionName, const FString& MessageText = TEXT("Hello from BPGen"));
};
