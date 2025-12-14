#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Data/SOTS_AbilityTypes.h"
#include "Interfaces/SOTS_AbilityInterfaces.h"
#include "SOTS_AbilityComponent.generated.h"

class USOTS_AbilityRegistrySubsystem;
class USOTS_AbilityBase;
class USOTS_AbilitySubsystem;
class UInvSP_InventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_AbilitySimpleSignature, FGameplayTag, AbilityTag, F_SOTS_AbilityHandle, Handle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_AbilityFailedSignature, FGameplayTag, AbilityTag, E_SOTS_AbilityActivationResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSOTS_AbilityActivatedWithContextSignature, FGameplayTag, AbilityTag, F_SOTS_AbilityHandle, Handle, const F_SOTS_AbilityActivationContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_AbilityListChangedSignature, UAC_SOTS_Abilitys*, AbilityComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSOTS_AbilityStateChangedSignature, FGameplayTag, AbilityTag, F_SOTS_AbilityStateSnapshot, NewState);

/**
 * Main, Blueprint-facing ability component for SOTS.
 *
 * High-level structure (v1.0 pattern):
 * - Static data: abilities are authored as F_SOTS_AbilityDefinition structs, usually
 *   inside USOTS_AbilityDefinitionDA assets. These are registered with the
 *   USOTS_AbilityRegistrySubsystem at startup.
 * - Global lookup: USOTS_AbilityRegistrySubsystem owns the map of AbilityTag ->
 *   F_SOTS_AbilityDefinition and exposes helper functions to fetch definitions
 *   and evaluate FSOTS_AbilityRequirements via USOTS_GAS_AbilityRequirementLibrary.
 * - Per-actor runtime state: UAC_SOTS_Abilitys lives on the player (or any actor)
 *   and tracks which ability tags are known plus per-ability cooldown / charges
 *   state in RuntimeStates.
 * - Implementation: gameplay for each ability lives in Blueprint subclasses of
 *   USOTS_AbilityBase. This component only manages lifecycle, gating, and events.
 */

UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class SOTS_GAS_PLUGIN_API UAC_SOTS_Abilitys : public UActorComponent
{
    GENERATED_BODY()

public:
    UAC_SOTS_Abilitys();

    UPROPERTY(BlueprintAssignable, Category="SOTS Ability|Events")
    FSOTS_AbilitySimpleSignature OnAbilityActivated;

    UPROPERTY(BlueprintAssignable, Category="SOTS Ability|Events")
    FSOTS_AbilitySimpleSignature OnAbilityEnded;

    UPROPERTY(BlueprintAssignable, Category="SOTS Ability|Events")
    FSOTS_AbilityFailedSignature OnAbilityFailed;

    // Extended activation event that also exposes the activation context for
    // UI or higher-level systems that want to inspect targets / locations.
    UPROPERTY(BlueprintAssignable, Category="SOTS Ability|Events")
    FSOTS_AbilityActivatedWithContextSignature OnAbilityActivatedWithContext;

    // Broadcast whenever the set of known abilities on this component changes.
    UPROPERTY(BlueprintAssignable, Category="SOTS Ability|Events")
    FSOTS_AbilityListChangedSignature OnAbilitiesChanged;

    // Broadcast whenever an individual ability's runtime state (charges, active
    // flag, cooldown) changes.
    UPROPERTY(BlueprintAssignable, Category="SOTS Ability|Events")
    FSOTS_AbilityStateChangedSignature OnAbilityStateChanged;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability")
    bool GrantAbility(FGameplayTag AbilityTag, const F_SOTS_AbilityGrantOptions& Options, F_SOTS_AbilityHandle& OutHandle);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability")
    bool RevokeAbilityByTag(FGameplayTag AbilityTag);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability")
    bool TryActivateAbilityByTag(FGameplayTag AbilityTag, const F_SOTS_AbilityActivationContext& Context, E_SOTS_AbilityActivationResult& OutResult);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability")
    void CancelAllAbilities();

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Query")
    void GetAbilityCharges(FGameplayTag AbilityTag, int32& OutCurrentCharges, int32& OutMaxCharges) const;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Query")
    void IsAbilityOnCooldown(FGameplayTag AbilityTag, bool& bOutIsOnCooldown, float& OutRemainingTime) const;

    // ---------- High-level convenience API ----------

    UFUNCTION(BlueprintCallable, Category="SOTS Ability")
    bool RemoveAbility(FGameplayTag AbilityTag);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Query")
    bool HasAbility(FGameplayTag AbilityTag) const;

    // Returns all ability tags that currently have runtime state on this
    // component (i.e., granted or previously activated abilities).
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Query")
    void GetKnownAbilities(TArray<FGameplayTag>& OutAbilityTags) const;

    // Pure requirement / state check without actually triggering activation.
    // Uses FSOTS_AbilityRequirements via the requirement library plus local
    // cooldown / charge / inventory gates.
    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Query")
    bool CanActivateAbility(FGameplayTag AbilityTag, FText& OutFailureReason) const;

    // High-level activation helper that combines CanActivateAbility + the
    // existing TryActivateAbilityByTag flow and returns a failure reason for UI.
    UFUNCTION(BlueprintCallable, Category="SOTS Ability")
    bool ActivateAbility(FGameplayTag AbilityTag, const F_SOTS_AbilityActivationContext& Context, FText& OutFailureReason);

    // Grant/remove without skill gating. Intended for systems such as SkillTree
    // or MissionDirector that need to override normal gating rules.
    UFUNCTION(BlueprintCallable, Category="SOTS Ability")
    bool ForceGrantAbility(FGameplayTag AbilityTag, const F_SOTS_AbilityGrantOptions& Options, F_SOTS_AbilityHandle& OutHandle);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability")
    bool ForceRemoveAbility(FGameplayTag AbilityTag);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Profile")
    void PushProfileStateToSubsystem();

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Profile")
    void PullProfileStateFromSubsystem();

    // Serialize / restore the component's runtime state for save systems.
    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Save")
    void GetSerializedState(F_SOTS_AbilityComponentSaveData& OutData) const;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Save")
    void ApplySerializedState(const F_SOTS_AbilityComponentSaveData& InData);

protected:
    virtual void BeginPlay() override;

    USOTS_AbilityRegistrySubsystem* GetRegistry() const;

    bool InternalGetDefinition(FGameplayTag AbilityTag, F_SOTS_AbilityDefinition& OutDef) const;

    bool PassesOwnerTagGate(const F_SOTS_AbilityDefinition& Def) const;
    bool PassesSkillGate(const F_SOTS_AbilityDefinition& Def) const;
    bool PassesInventoryGate(const F_SOTS_AbilityDefinition& Def) const;

    bool HasSufficientCharges(const F_SOTS_AbilityDefinition& Def) const;
    void ConsumeChargesOnActivation(const F_SOTS_AbilityDefinition& Def);

    int32 QueryInventoryItemCount(const FGameplayTagContainer& Tags) const;
    bool ConsumeInventoryItems(const FGameplayTagContainer& Tags, int32 Count) const;

    float GetWorldTime() const;

    // Evaluate FSOTS_AbilityRequirements from the ability definition (if any)
    // using the shared requirement library.
    bool EvaluateAbilityRequirementsForDefinition(const F_SOTS_AbilityDefinition& Def, FText* OutFailureReason) const;

    // Helper used by UI/events to expose a stable, Blueprint-safe snapshot of
    // an ability's current runtime state.
    F_SOTS_AbilityStateSnapshot BuildStateSnapshot(FGameplayTag AbilityTag) const;

    void BroadcastAbilityListChanged();
    void BroadcastAbilityStateChanged(FGameplayTag AbilityTag);

    UInvSP_InventoryComponent* ResolveInventoryComponent() const;

protected:
    UPROPERTY()
    mutable TWeakObjectPtr<UInvSP_InventoryComponent> InventoryComponent;

    UPROPERTY()
    USOTS_AbilitySubsystem* AbilitySubsystem = nullptr;

    UPROPERTY()
    TMap<FGameplayTag, F_SOTS_AbilityRuntimeState> RuntimeStates;

    UPROPERTY()
    TMap<FGameplayTag, USOTS_AbilityBase*> AbilityInstances;

    UPROPERTY()
    int32 NextInternalHandleId = 1;
};
