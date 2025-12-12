#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_AbilityTypes.generated.h"

class AActor;
class USOTS_AbilityBase;

UENUM(BlueprintType)
enum class E_SOTS_AbilityActivationResult : uint8
{
    Success,
    Failed_Cooldown,
    Failed_ChgDepleted,
    Failed_InventoryGate,
    Failed_OwnerTags,
    Failed_SkillGate,
    Failed_CustomCondition
};

UENUM(BlueprintType)
enum class E_SOTS_AbilityInventoryMode : uint8
{
    None,
    RequireAndConsume,
    RequireOnly
};

UENUM(BlueprintType)
enum class E_SOTS_AbilityChargeMode : uint8
{
    InternalOnly,
    InventoryLinked,
    Hybrid
};

UENUM(BlueprintType)
enum class E_SOTS_AbilitySkillGateMode : uint8
{
    None,
    RequireForGrant,
    RequireForActivate,
    RequireForBoth
};

UENUM(BlueprintType)
enum class E_SOTS_AbilityActivationPolicy : uint8
{
    OnInputPressed,
    OnInputReleased,
    OnOwnerEvent,
    Passive
};

USTRUCT(BlueprintType)
struct SOTS_GAS_PLUGIN_API F_SOTS_AbilityHandle
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InternalId = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag AbilityTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsValid = false;
};

USTRUCT(BlueprintType)
struct SOTS_GAS_PLUGIN_API F_SOTS_AbilityGrantOptions
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InitialCharges = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bStartOnCooldown = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float OverrideCooldownSeconds = -1.0f;
};

USTRUCT(BlueprintType)
struct SOTS_GAS_PLUGIN_API F_SOTS_AbilityActivationContext
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* Instigator = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* TargetActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer ActivationTags;
};

// Generic, reusable description of what an ability requires in order
// to be considered usable. This does not perform activation logic;
// it simply describes the gates that other systems should check.
USTRUCT(BlueprintType)
struct SOTS_GAS_PLUGIN_API FSOTS_AbilityRequirements
{
    GENERATED_BODY()

    // Optional identifier; in libraries you typically index by AbilityTag.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    FGameplayTag AbilityTag;

    // Skill tags that must be present (from the skill tree subsystem).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    FGameplayTagContainer RequiredSkillTags;

    // Generic tags required on the player/profile (e.g., Player.State.HasDragon).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    FGameplayTagContainer RequiredPlayerTags;

    // If >= 0, the global stealth tier must be >= this (cast from ESOTS_StealthTier).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    int32 MinStealthTier = -1;

    // If >= 0, the global stealth tier must be <= this.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    int32 MaxStealthTier = -1;

    // If true, disallow when the tier is "Compromised" (or higher, if extended later).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    bool bDisallowWhenCompromised = false;

    // Optional soft cap on overall stealth score: ability is disabled if score exceeds this.
    // < 0.0 = ignore.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaxStealthScore01 = -1.0f;
};

USTRUCT(BlueprintType)
struct SOTS_GAS_PLUGIN_API F_SOTS_AbilityDefinition
{
    GENERATED_BODY()

public:
    // Stable identifier for this ability (used everywhere as the primary key).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    FGameplayTag AbilityTag;

    // Optional high-level category (e.g. Ability.Dragon, Ability.Item, Ability.Utility).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    FGameplayTag AbilityCategoryTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    TSubclassOf<USOTS_AbilityBase> AbilityClass;

    // Simple, generic cooldown time in seconds. Content may also implement
    // more complex cooldown rules in Blueprint, but this field is the shared
    // baseline used by the ability component.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    float CooldownSeconds = 0.0f;

    // Generic finite-uses budget for this ability. If <= 0, the ability is
    // treated as having infinite internal charges.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    int32 MaxCharges = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    E_SOTS_AbilityChargeMode ChargeMode = E_SOTS_AbilityChargeMode::InternalOnly;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    E_SOTS_AbilityInventoryMode InventoryMode = E_SOTS_AbilityInventoryMode::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    E_SOTS_AbilitySkillGateMode SkillGateMode = E_SOTS_AbilitySkillGateMode::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    FGameplayTagContainer RequiredOwnerTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    FGameplayTagContainer BlockedOwnerTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    FGameplayTagContainer RequiredInventoryTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    TArray<FGameplayTag> RequiredSkillTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    FGameplayTagContainer GrantedTagsWhileActive;

    // Optional, authored requirements that describe what must be true in
    // order for this ability to be considered usable. These are evaluated
    // via USOTS_GAS_AbilityRequirementLibrary and are intentionally kept
    // data-only so they can be reused by systems such as KEM, UI, and
    // MissionDirector.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
    FSOTS_AbilityRequirements AbilityRequirements;

    // Optional FX tags that are fired via the global FX manager when this
    // ability successfully activates or fails due to requirements.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|FX")
    FGameplayTag FXTag_OnActivate;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|FX")
    FGameplayTag FXTag_OnFailRequirements;
};

USTRUCT()
struct F_SOTS_AbilityRuntimeState
{
    GENERATED_BODY()

public:
    UPROPERTY()
    F_SOTS_AbilityHandle Handle;

    UPROPERTY()
    bool bIsActive = false;

    UPROPERTY()
    int32 CurrentCharges = 0;

    UPROPERTY()
    float CooldownEndTime = 0.0f;
};

// Lightweight snapshot of per-ability runtime state that is safe to expose
// to Blueprint UI and save systems. This mirrors the internal runtime state
// but avoids leaking implementation details such as absolute world times.
USTRUCT(BlueprintType)
struct SOTS_GAS_PLUGIN_API F_SOTS_AbilityStateSnapshot
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    FGameplayTag AbilityTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    bool bIsActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    int32 CurrentCharges = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    bool bIsOnCooldown = false;

    // Remaining cooldown time in seconds at the moment the snapshot was taken.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|GAS|Ability")
    float RemainingCooldown = 0.0f;
};

// Serialized representation of an ability component's state. This is intended
// for profile / mission saves and can be round-tripped back into the ability
// component without requiring any level-specific wiring.
USTRUCT(BlueprintType)
struct SOTS_GAS_PLUGIN_API F_SOTS_AbilityComponentSaveData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|GAS|Ability")
    TArray<F_SOTS_AbilityStateSnapshot> SavedAbilities;
};
