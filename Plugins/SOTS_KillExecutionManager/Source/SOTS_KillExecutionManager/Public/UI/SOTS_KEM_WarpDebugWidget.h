#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_KEM_WarpDebugWidget.generated.h"

class ASOTS_ExecutionHelperActor;

/**
 * Optional Advanced-mode widget that visualises warp / spawn information
 * for SpawnActor executions. It reads data from an ASOTS_ExecutionHelperActor
 * instance and exposes it to Blueprint.
 */
UCLASS(Abstract, Blueprintable)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_WarpDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Helper actor to inspect.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="KEM|Debug", meta=(ExposeOnSpawn="true"))
    ASOTS_ExecutionHelperActor* ObservedHelper = nullptr;

protected:
    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    FSOTS_KEM_OmniTraceWarpResult CachedWarpResult;

    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    FTransform CachedSpawnTransform;

    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    ESOTSStealthDebugMode CurrentDebugMode;

    UFUNCTION(BlueprintImplementableEvent, Category="KEM|Debug")
    void OnWarpDebugUpdated();
};
