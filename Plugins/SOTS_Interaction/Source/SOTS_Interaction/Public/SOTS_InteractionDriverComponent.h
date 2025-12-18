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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FSOTS_OnDriverFocusChanged,
    AActor*, OldActor,
    AActor*, NewActor
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FSOTS_OnDriverOptionsChanged,
    AActor*, FocusedActor,
    const TArray<FSOTS_InteractionOption>&, Options
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FSOTS_OnDriverInteractRequested,
    AActor*, FocusedActor,
    FGameplayTag, OptionTag
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

    /** Fired when focus target changes (including cleared). */
    UPROPERTY(BlueprintAssignable, Category="SOTS|Interaction|Driver")
    FSOTS_OnDriverFocusChanged OnFocusChanged;

    /** Fired when available options for the focused target change. */
    UPROPERTY(BlueprintAssignable, Category="SOTS|Interaction|Driver")
    FSOTS_OnDriverOptionsChanged OnOptionsChanged;

    /** Fired when the driver requests an interaction. */
    UPROPERTY(BlueprintAssignable, Category="SOTS|Interaction|Driver")
    FSOTS_OnDriverInteractRequested OnInteractRequested;

    /** Call from input binding (Interact). */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    FSOTS_InteractionResult Driver_RequestInteract();

    /** Call from UI choice: execute selected option. */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    FSOTS_InteractionResult Driver_ExecuteOption(FGameplayTag OptionTag);

    /** Force update now. */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    void Driver_UpdateNow();

    /** Blueprint-friendly alias for Driver_UpdateNow. */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    void ForceRefreshFocus();

    /** Current focused actor (if any). */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    AActor* GetFocusedActor() const;

    /** Score of current focus (0 if none). */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    float GetFocusedScore() const;

    /** Get cached options for current focus. */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    bool GetFocusedOptions(TArray<FSOTS_InteractionOption>& OutOptions) const;

    /** Request interaction with optional verb/option tag. Returns success flag and fills result/option. */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction|Driver")
    bool TryInteract(FGameplayTag OptionTag, FSOTS_InteractionResult& OutResult, FSOTS_InteractionOption& OutChosenOption);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<USOTS_InteractionSubsystem> Subsystem;

    UPROPERTY(Transient)
    TWeakObjectPtr<APlayerController> CachedPC;

    UPROPERTY(Transient)
    FSOTS_InteractionData CachedData;

    FTimerHandle AutoUpdateTimer;

    void BindSubsystemEvents();
    void UnbindSubsystemEvents();

    UFUNCTION()
    void HandleSubsystemUIIntentPayload(APlayerController* PlayerController, const FSOTS_InteractionUIIntentPayload& Payload);

    UFUNCTION()
    void HandleSubsystemCandidateChanged(APlayerController* PlayerController, const FSOTS_InteractionContext& NewCandidate);

    void RefreshCachedData();

    void BroadcastOptionChangeIfNeeded(const TArray<FSOTS_InteractionOption>& OldOptions, const TArray<FSOTS_InteractionOption>& NewOptions);

    void AutoUpdateTick();
};
