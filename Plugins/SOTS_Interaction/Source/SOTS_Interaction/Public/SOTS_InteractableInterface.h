#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "SOTS_InteractionTypes.h"
#include "SOTS_InteractableInterface.generated.h"

UINTERFACE(BlueprintType)
class USOTS_InteractableInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Optional "smart" interface for actors that want custom interaction logic.
 * If an actor has USOTS_InteractableComponent but also implements this interface,
 * subsystem/component may query these hooks.
 */
class ISOTS_InteractableInterface
{
    GENERATED_BODY()

public:
    /**
     * Return whether the actor can be interacted with right now.
     * OutFailReasonTag should be set when returning false (optional).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Interaction")
    bool CanInteract(const FSOTS_InteractionContext& Context, FGameplayTag& OutFailReasonTag) const;

    /**
     * Provide all options for this interaction. If empty, the system can fallback to a default option.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Interaction")
    void GetInteractionOptions(const FSOTS_InteractionContext& Context, TArray<FSOTS_InteractionOption>& OutOptions) const;

    /**
     * Execute a chosen option. OptionTag identifies which option was selected.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="SOTS|Interaction")
    void ExecuteInteraction(const FSOTS_InteractionContext& Context, FGameplayTag OptionTag);
};