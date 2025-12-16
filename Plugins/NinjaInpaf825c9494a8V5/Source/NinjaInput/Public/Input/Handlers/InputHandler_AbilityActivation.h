// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputHandler.h"
#include "UObject/Object.h"
#include "InputHandler_AbilityActivation.generated.h"

class UGameplayAbility;
class UNinjaInputAbilityActivationCheck;

/**
 * Base input handler for ability activations. 
 */
UCLASS(Abstract)
class NINJAINPUT_API UInputHandler_AbilityActivation : public UNinjaInputHandler
{
    
    GENERATED_BODY()

public:
    
    UInputHandler_AbilityActivation();

protected:

    /**
     * If set to true, this handler will send a Gameplay Event on additional inputs, made while the ability is active.
     *
     * Activation must be determined by child classes that will have their own activation strategies (i.e. Class, Tags
     * or Input ID) and must use these same strategies to determine if an ability is active or not. 
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Activation")
    bool bSendEventIfActive;

    /**
     * If set to true, the event will be triggered on locally.
     *
     * If the "Trigger Event On Server" flag is enabled and the Input Owner is both locally controlled and authoritative,
     * the event is guaranteed to not be triggered twice.
     */    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Activation", meta = (EditCondition = "bSendEventIfActive", EditConditionHides))
    bool bTriggerEventLocally;

    /**
     * If set to true, the event will be triggered on the server (authoritative) version.
     *
     * If the "Trigger Event Locally" flag is enabled and the Input Owner is both locally controlled and authoritative,
     * the event is guaranteed to not be triggered twice.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Activation", meta = (EditCondition = "bSendEventIfActive", EditConditionHides))
    bool bTriggerEventOnServer;
    
    /**
     * Gameplay Tag used to trigger a Gameplay Event, if there is an activation attempt, while the ability is active.
     *
     * This Gameplay Event can be tracked by the active ability or any other ability that is already active or can
     * be activated by a Gameplay State. A common use-case for this is a Combo Attack.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Activation", meta = (EditCondition = "bSendEventIfActive", EditConditionHides))
    FGameplayTag ActiveEventTag;

	/**
	 * Provides additional flexibility while evaluating Input Action values.
	 *
	 * For common triggers such as Pressed or Released, which will return a proper boolean false, you probably
	 * won't need to use this. However, for other triggers such as "Tap" and "Double Tap" which won't provide
	 * such boolean values - meaning their values will be *false* once they *trigger*, you may want to use the
	 * provided (default) UNinjaInputAbilityActivationCheck, which can handle such cases.
	 *
	 * You can also extend the base class and add any other necessary checks for activation.
	 *
	 * This value is set as the default class, which handles both cases and tries to find an outcome as fast
	 * as possible. However, if you are looking for extreme optimizations and are NOT using triggers such as
	 * the ones mentioned above, you may want to clear this value.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Activation")
	TSubclassOf<UNinjaInputAbilityActivationCheck> AbilityActivationCheckClass;
	
    // -- Begin Input Handler implementation
	virtual void HandleStartedEvent_Implementation(UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction) const override;
	
    virtual void HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager,
        const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const override;

    virtual void HandleCompletedEvent_Implementation(UNinjaInputManagerComponent* Manager,
        const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const override;

    virtual void HandleCancelledEvent_Implementation(UNinjaInputManagerComponent* Manager,
        const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const override;
    // -- End Input Handler implementation

	/**
	 * Basic logic for activations.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ninja Input|Ability Activation")
	virtual void HandleActivation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
		const UInputAction* InputAction, float ElapsedTime) const;

	/**
	 * Centralized logic for what should happen when the ability has a momentary activation ended.
	 *
	 * Usually, this would mean the ability being cancelled. But you might want to have the ability
	 * canceling or confirming ongoing targets if necessary.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ninja Input|Ability Activation")
	virtual void HandleMomentaryDeactivation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
		const UInputAction* InputAction, float ElapsedTime) const;
	
    /**
     * Allows proper handling of abilities that are already active, based on the current setup.
     *
     * A reason for you to override this method is to potentially avoid iterating over the abilities
     * if there's no reason to do so. For example, by default, both "Event" and Toggle "checks are
     * taken into consideration. If that scenario can never happen in conjunction for you system,
     * then it may be worth it to adjust this functionality to fit your needs.
     *
     * @param Manager           Input Manager that has invoked this handler. Must be valid.
     * @param Value             Original value received from the Enhanced Input System.
     * @param InputAction       Original Action that is triggering this activation.
     * @return                  True if this method handled the ability. If false, then handling is pending.
     */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ninja Input|Ability Activation")
    virtual bool TryHandleActiveAbility(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
        const UInputAction* InputAction) const;

    /**
     * Concrete implementation that can check if an ability is active based on a certain criteria.
     *
     * Each subclass must implement this method properly, without calling the base implementation (super).
     * 
     * @param Manager           Input Manager that has invoked this handler. Must be valid.
     * @return                  True if an Ability is active.
     */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|Ability Activation")
    virtual bool HasActiveAbility(const UNinjaInputManagerComponent* Manager) const;

	/**
	 * Provides asset tags related to the Ability related to this handler.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|Ability Activation")
	FGameplayTagContainer GetAbilityTags(const UNinjaInputManagerComponent* Manager) const;
	
	/**
	 * Checks if the active ability related to this input handler has a momentary activation.
	 *
	 * Momentary activation means that the ability will be cancelled (or similar) when the
	 * input system receives a "false" trigger (i.e. "release").
	 *
	 * The ability is considered "momentary" if it has the "Input.Ability.Momentary" tag
	 * added to its asset tags.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|Ability Activation")
	virtual bool IsMomentaryAbility(const UNinjaInputManagerComponent* Manager) const;
	
	/**
	 * Checks if the active ability related to this input handler has a toggled activation.
	 *
	 * Toggled activation means that the ability will be cancelled (or similar) when the
	 * input system receives a secondary "true" trigger (i.e. "pressed").
	 *
	 * The ability is considered "toggled" if it has the "Input.Ability.Toggled" tag
	 * added to its asset tags.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|Ability Activation")
	virtual bool IsToggledAbility(const UNinjaInputManagerComponent* Manager) const;	
	
    /**
     * Concrete implementation that will activate an ability using a proper activation mode.
     *
     * Each subclass must implement this method properly, without calling the base implementation (super).
     * 
     * @param Manager           Input Manager that has invoked this handler. Must be valid.
     * @param Value             Original value received from the Enhanced Input System.
     * @param InputAction       Original Action that is triggering this activation.
     */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ninja Input|Ability Activation")
    virtual bool ActivateAbility(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
        const UInputAction* InputAction) const;

    /**
     * Concrete implementation that will cancel an ability using a proper cancellation mode.
     * 
     * @param Manager           Input Manager that has invoked this handler. Must be valid.
     * @param Value             Original value received from the Enhanced Input System.
     * @param InputAction       Original Action that is triggering this activation.
     */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ninja Input|Ability Activation")
	virtual bool CancelAbility(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
        const UInputAction* InputAction) const;

	/**
	 * Provides the active Ability Instance related to this input, if any.
	 */
	virtual UGameplayAbility* GetAbilityInstance(const UNinjaInputManagerComponent* Manager) const;
	
    /**
     * Sends a gameplay event for the provided tag.
     *
     * It will send the Input Action as an optional object and the value as Magnitude.
     *
     * @param Manager           Input Manager that has invoked this handler. Must be valid.
     * @param GameplayEventTag  Gameplay Event Tag that identifies the Event to trigger. Must be valid.
     * @param Value             Input value that generated the input activation.
     * @param InputAction       Specific Input Action that was activated.
     * @return                  Ability activations generated by this event. 
     */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ninja Input|Ability Activation")
    virtual int32 SendGameplayEvent(const UNinjaInputManagerComponent* Manager, FGameplayTag GameplayEventTag,
        const FInputActionValue& Value, const UInputAction* InputAction) const;

    /**
     * Checks the custom activation class in case we can define the activation right away.
     */
	bool EvaluateAbilityActivation(const FInputActionValue& Value, const UInputAction* InputAction) const;

};

