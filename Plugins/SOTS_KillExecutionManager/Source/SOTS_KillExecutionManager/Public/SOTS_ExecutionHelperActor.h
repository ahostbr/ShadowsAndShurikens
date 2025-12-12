#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_ExecutionHelperActor.generated.h"

class USOTS_SpawnExecutionData;
class USOTS_KEM_ExecutionDefinition;

/**
 * Helper actor spawned by KEM for SpawnActor backend executions.
 * It plays the configured montages on instigator and target, then self-destructs.
 */
UCLASS()
class SOTS_KILLEXECUTIONMANAGER_API ASOTS_ExecutionHelperActor : public AActor
{
    GENERATED_BODY()

public:
    ASOTS_ExecutionHelperActor();

    // Initialize this helper with the resolved execution context and the spawn data.
    // ExecutionDefinition and SpawnData together describe the authored warp points
    // and candidate warp names that OmniTrace or other logic can evaluate.
    void Initialize(const FSOTS_ExecutionContext& InContext,
                    const USOTS_SpawnExecutionData* InData,
                    const FTransform& InSpawnTransform,
                    const USOTS_KEM_ExecutionDefinition* InExecutionDefinition,
                    const FSOTS_KEM_OmniTraceWarpResult& InWarpResult);

    // Blueprint extension point: runs before any montages are played.
    // The default implementation applies OmniTrace or authored warp point data
    // before the montages start; override in Blueprint only when you need
    // custom spatial logic beyond the builtin behavior.
    // Blueprint authors: call the parent implementation first to ensure the
    // default warp placement is applied before adding per-execution tweaks.
    UFUNCTION(BlueprintNativeEvent, Category="KEM|Spawn")
    void PrePlaySpawnMontages();
    virtual void PrePlaySpawnMontages_Implementation();

    // Signals to KEM that this execution has fully ended. Intended to be
    // called from Blueprint (e.g., via montage notifies) when the helper
    // has finished its work.
    UFUNCTION(BlueprintCallable, Category="SOTS|KEM")
    void NotifyExecutionEnded(bool bWasSuccessful);

    // Debug-only accessors used by warp visualisation widgets.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|KEM|Debug")
    const FSOTS_KEM_OmniTraceWarpResult& GetWarpResult() const { return WarpResult; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|KEM|Debug")
    const FTransform& GetSpawnTransform() const { return SpawnTransform; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(BlueprintReadOnly, Category="KEM", meta=(AllowPrivateAccess="true"))
    FSOTS_ExecutionContext Context;

    UPROPERTY()
    const USOTS_SpawnExecutionData* SpawnData = nullptr;

    // Owning execution definition for this spawn-based execution.
    UPROPERTY(BlueprintReadOnly, Category="KEM", meta=(AllowPrivateAccess="true"))
    const USOTS_KEM_ExecutionDefinition* ExecutionDefinition = nullptr;

    // OmniTrace warp result cached for animation / movement logic.
    UPROPERTY(BlueprintReadOnly, Category="KEM|Warp", meta=(AllowPrivateAccess="true"))
    FSOTS_KEM_OmniTraceWarpResult WarpResult;

    // Initial spawn transform chosen by KEM before any warp refinement.
    UPROPERTY(BlueprintReadOnly, Category="KEM", meta=(AllowPrivateAccess="true"))
    FTransform SpawnTransform;

    void PlayMontages();
};
