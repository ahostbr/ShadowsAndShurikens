#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SOTS_AIPerceptionTypes.h"
#include "SOTS_GAS_DebugLibrary.h"
#include "SOTS_EnemyPerceptionDebugWidget.generated.h"

class USOTS_AIPerceptionComponent;

/**
 * Base widget for per-enemy perception debug. WBP_SOTS_EnemyPerceptionDebug
 * should subclass this and render the cached values.
 */
UCLASS(Abstract, Blueprintable)
class SOTS_GAS_PLUGIN_API USOTS_EnemyPerceptionDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Actor this widget should inspect for perception state. Typically set
    // from owning pawn or a helper component in Blueprint.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perception", meta=(ExposeOnSpawn="true"))
    AActor* ObservedActor = nullptr;

protected:
    UPROPERTY(BlueprintReadOnly, Category="Perception")
    ESOTS_PerceptionState CachedPerceptionState = ESOTS_PerceptionState::Unaware;

    UPROPERTY(BlueprintReadOnly, Category="Perception")
    float CachedSuspicion01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Perception")
    FSOTS_PerceivedTargetState CachedTargetState;

    UPROPERTY(BlueprintReadOnly, Category="Perception")
    float CachedDistanceToPlayer = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Perception")
    bool bCachedHasLineOfSight = false;

    UPROPERTY(BlueprintReadOnly, Category="Perception")
    ESOTSStealthDebugMode CurrentDebugMode = ESOTSStealthDebugMode::Off;

    UFUNCTION(BlueprintImplementableEvent, Category="Perception")
    void OnPerceptionDebugDataUpdated();
};

