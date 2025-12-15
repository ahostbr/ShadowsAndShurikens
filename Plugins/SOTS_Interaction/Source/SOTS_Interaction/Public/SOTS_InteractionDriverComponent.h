#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SOTS_InteractionTypes.h"
#include "SOTS_InteractionUIIntentPayload.h"
#include "SOTS_InteractionDriverComponent.generated.h"

class USOTS_InteractionSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSOTS_OnDriverUIIntentPayload,
    const FSOTS_InteractionUIIntentPayload&, Payload
);

UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class USOTS_InteractionDriverComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_InteractionDriverComponent();

    /** If true, the driver periodically updates interaction candidate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Driver")
    bool bAutoUpdate;

    /** How often to call UpdateCandidateThrottled. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction|Driver", meta=(ClampMin="0.01"))
    float AutoUpdateIntervalSeconds;

    /** Fired whenever subsystem emits a UI intent; forward to SOTS_UI router in BP. */
    UPROPERTY(BlueprintAssignable, Category="SOTS|Interaction|Driver")
    FSOTS_OnDriverUIIntentPayload OnUIIntentForwarded;

    /** Call from input binding (Interact). */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    FSOTS_InteractionResult Driver_RequestInteract();

    /** Call from UI choice: execute selected option. */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    FSOTS_InteractionResult Driver_ExecuteOption(FGameplayTag OptionTag);

    /** Force update now. */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    void Driver_UpdateNow();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<USOTS_InteractionSubsystem> Subsystem;

    UPROPERTY(Transient)
    TWeakObjectPtr<APlayerController> CachedPC;

    FTimerHandle AutoUpdateTimer;

    void BindSubsystemEvents();
    void UnbindSubsystemEvents();

    UFUNCTION()
    void HandleSubsystemUIIntentPayload(APlayerController* PlayerController, const FSOTS_InteractionUIIntentPayload& Payload);

    void AutoUpdateTick();
};
