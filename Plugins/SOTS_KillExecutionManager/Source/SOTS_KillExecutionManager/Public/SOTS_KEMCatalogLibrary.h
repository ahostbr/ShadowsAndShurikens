#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_KEMCatalogLibrary.generated.h"

class USOTS_KEM_ExecutionCatalog;
class UObject;

UCLASS()
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEMCatalogLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="SOTS|KEM", meta=(WorldContext="WorldContextObject"))
    static USOTS_KEM_ExecutionCatalog* GetExecutionCatalog(const UObject* WorldContextObject);
};
