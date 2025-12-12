#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_KEMAuthoringLibrary.generated.h"

UCLASS()
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEMAuthoringLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Authoring")
    static void GetAllExecutionDefinitions(TArray<USOTS_KEM_ExecutionDefinition*>& OutDefs);

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Authoring")
    static void FilterExecutionDefinitionsByFamily(
        const TArray<USOTS_KEM_ExecutionDefinition*>& InDefs,
        FGameplayTag FamilyTag,
        TArray<USOTS_KEM_ExecutionDefinition*>& OutDefs
    );

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Authoring")
    static void FilterExecutionDefinitionsByPosition(
        const TArray<USOTS_KEM_ExecutionDefinition*>& InDefs,
        FGameplayTag PositionTag,
        TArray<USOTS_KEM_ExecutionDefinition*>& OutDefs
    );

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Authoring")
    static void FilterExecutionDefinitionsByTargetKind(
        const TArray<USOTS_KEM_ExecutionDefinition*>& InDefs,
        ESOTS_KEMTargetKind TargetKind,
        TArray<USOTS_KEM_ExecutionDefinition*>& OutDefs
    );

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Coverage")
    static void BuildCoverageMatrix(
        const TArray<USOTS_KEM_ExecutionDefinition*>& InDefs,
        TArray<FSOTS_KEMCoverageCell>& OutCells
    );

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Coverage")
    static void LogCoverageMatrix(const TArray<FSOTS_KEMCoverageCell>& Cells);
};
