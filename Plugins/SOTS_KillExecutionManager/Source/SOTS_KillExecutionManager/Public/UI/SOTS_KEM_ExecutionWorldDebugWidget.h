#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_KEM_Types.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_KEM_ExecutionWorldDebugWidget.generated.h"

/**
 * World-space widget that visualises the current KEM execution state
 * for a given actor (instigator or target). Blueprint widgets like
 * WBP_SOTS_KEM_ExecutionWorldDebug should subclass this and render
 * CachedRecord appropriately.
 */
UCLASS(Abstract, Blueprintable)
class SOTS_KILLEXECUTIONMANAGER_API USOTS_KEM_ExecutionWorldDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Actor this widget is attached to (instigator or target).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="KEM|Debug", meta=(ExposeOnSpawn="true"))
    AActor* ObservedActor = nullptr;

protected:
    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    FSOTS_KEMDebugRecord CachedRecord;

    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    bool bHasValidRecord = false;

    UPROPERTY(BlueprintReadOnly, Category="KEM|Debug")
    ESOTSStealthDebugMode CurrentDebugMode;

    UFUNCTION(BlueprintImplementableEvent, Category="KEM|Debug")
    void OnExecutionWorldDebugUpdated();
};
