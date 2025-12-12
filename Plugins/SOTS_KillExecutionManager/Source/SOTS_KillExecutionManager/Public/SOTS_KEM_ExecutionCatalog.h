#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_KEM_ExecutionCatalog.generated.h"

class USOTS_KEM_ExecutionDefinition;

/**
 * Catalog of execution definitions that represent the vertical slice.
 *
 * DevTools: Python KEM coverage tools read this catalog (via its asset path)
 * to know which ExecutionDefinitions to report on. All vertical-slice
 * executions should be registered here.
 */
UCLASS(BlueprintType)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_ExecutionCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    // All execution definitions used in this catalog (e.g. vertical slice)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM")
    TArray<TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>> ExecutionDefinitions;
};
