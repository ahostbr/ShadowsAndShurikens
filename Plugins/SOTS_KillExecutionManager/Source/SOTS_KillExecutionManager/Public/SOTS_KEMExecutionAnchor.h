#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_KEMExecutionAnchor.generated.h"

class UArrowComponent;
class USceneComponent;
class USOTS_KEM_ExecutionDefinition;

/**
 * DevTools: Python anchor coverage tools scan maps for ASOTS_KEMExecutionAnchor
 * instances to correlate with ExecutionDefinitions and PositionTags.
 */
UCLASS(BlueprintType)
class SOTS_KILLEXECUTIONMANAGER_API ASOTS_KEMExecutionAnchor : public AActor
{
    GENERATED_BODY()

public:
    ASOTS_KEMExecutionAnchor();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Anchor")
    USceneComponent* AnchorRoot = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Anchor")
    UArrowComponent* ArrowComponent = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Anchor", meta=(ToolTip="Match one of the canonical SOTS.KEM.Position.* tags to describe how an execution should approach this spot."))
    FGameplayTag PositionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Anchor", meta=(ToolTip="Select the family that best represents this anchor (GroundRear ↔ PositionTag Ground.Rear, VerticalAbove ↔ Vertical.Above, etc.)."))
    ESOTS_KEM_ExecutionFamily ExecutionFamily = ESOTS_KEM_ExecutionFamily::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Anchor", meta=(ToolTip="Optional preferred executions that should run when the player uses this anchor area."))
    TArray<TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>> PreferredExecutions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Anchor", meta=(ToolTip="Radius within which this anchor is considered valid by supporting systems."))
    float UseRadius = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|KEM|Anchor", meta=(ToolTip="Extra tags that describe the surrounding environment (ledge, railing, window, etc.)."))
    TArray<FGameplayTag> EnvContextTags;

public:
    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Anchor")
    const FGameplayTag& GetPositionTag() const { return PositionTag; }

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Anchor")
    ESOTS_KEM_ExecutionFamily GetExecutionFamily() const { return ExecutionFamily; }

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Anchor")
    const TArray<TSoftObjectPtr<USOTS_KEM_ExecutionDefinition>>& GetPreferredExecutions() const { return PreferredExecutions; }

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Anchor")
    float GetUseRadius() const { return UseRadius; }

    UFUNCTION(BlueprintCallable, Category="SOTS|KEM|Anchor")
    const TArray<FGameplayTag>& GetEnvContextTags() const { return EnvContextTags; }
};
