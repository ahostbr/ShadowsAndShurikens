#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "SOTS_InteractionTypes.generated.h"

class AActor;
class APawn;
class APlayerController;

/**
 * A single selectable interaction option for a target (e.g., "Open", "Loot", "Drag Body").
 */
USTRUCT(BlueprintType)
struct FSOTS_InteractionOption
{
    GENERATED_BODY()

    /** Option identifier (stable tag, drives behavior). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTag OptionTag;

    /** Player-facing label to display in UI (UI is routed elsewhere). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FText DisplayText;

    /** Optional: reason this option is blocked (empty = available). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTag BlockedReasonTag;

    /** Optional: arbitrary extra tags describing this option (e.g. "RequiresKey"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTagContainer MetaTags;

    FSOTS_InteractionOption()
        : OptionTag()
        , DisplayText(FText::GetEmpty())
        , BlockedReasonTag()
        , MetaTags()
    {}
};

/**
 * The context passed around for interaction validation and execution.
 * This is the canonical payload type; UI can be driven from this.
 */
USTRUCT(BlueprintType)
struct FSOTS_InteractionContext
{
    GENERATED_BODY()

    /** The controller initiating the interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    TWeakObjectPtr<APlayerController> PlayerController;

    /** The pawn initiating the interaction (usually the player's pawn). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    TWeakObjectPtr<APawn> PlayerPawn;

    /** The target actor being interacted with. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    TWeakObjectPtr<AActor> TargetActor;

    /** The primary interaction type tag for this target (e.g., Pickup/Door/Talk). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FGameplayTag InteractionTypeTag;

    /** Distance from player to target at query time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    float Distance = 0.f;

    /** Trace result used to resolve the candidate (if any). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SOTS|Interaction")
    FHitResult HitResult;

    FSOTS_InteractionContext()
        : PlayerController(nullptr)
        , PlayerPawn(nullptr)
        , TargetActor(nullptr)
        , InteractionTypeTag()
        , Distance(0.f)
        , HitResult()
    {}
};
