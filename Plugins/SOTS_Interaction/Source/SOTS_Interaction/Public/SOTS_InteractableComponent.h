#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SOTS_InteractionTypes.h"
#include "SOTS_InteractableComponent.generated.h"

/**
 * Lightweight data/config component marking an actor as interactable.
 * Logic is resolved by the subsystem and/or ISOTS_InteractableInterface.
 */
UCLASS(ClassGroup=(SOTS), meta=(BlueprintSpawnableComponent))
class USOTS_InteractableComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USOTS_InteractableComponent();

    /** Primary interaction type for this actor (Pickup/Door/Talk/etc). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction")
    FGameplayTag InteractionTypeTag;

    /** Player-facing display name (used by UI via routed intent; no UI here). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction")
    FText DisplayName;

    /** Maximum interaction distance. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction", meta=(ClampMin="0.0"))
    float MaxDistance;

    /** Whether LOS is required to interact. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction")
    bool bRequiresLineOfSight;

    /** Tags required on the player to interact (evaluated by subsystem later). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction")
    FGameplayTagContainer RequiredPlayerTags;

    /** Tags that block interaction if present on the player (evaluated by subsystem later). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction")
    FGameplayTagContainer BlockedPlayerTags;

    /** Tags required on the target (self) for an option to be available. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction")
    FGameplayTagContainer RequiredTargetTags;

    /** Tags that block interaction if present on the target (self). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction")
    FGameplayTagContainer BlockedTargetTags;

    /** Optional: extra tags describing this interactable (category, rarity, etc). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction")
    FGameplayTagContainer MetaTags;

    /** Optional options defined on the component. Component options take priority over interface options. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Interaction")
    TArray<FSOTS_InteractionOption> Options;

    /** Convenience accessor: does owner implement ISOTS_InteractableInterface? */
    UFUNCTION(BlueprintCallable, Category="SOTS|Interaction")
    bool OwnerImplementsInteractableInterface() const;

protected:
    virtual void BeginPlay() override;
};